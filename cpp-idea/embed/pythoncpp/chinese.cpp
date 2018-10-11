#include "stdafx.h"
#include "chinese.h"
#include "handwritingData.h"
#include "SigUtilities.h"
#include "bch.h"
#include "logSigLength.h"
#include "Representations.h"

#define CALC_STATS //just so we can see the compilation time hit of doing this here
#ifdef CALC_STATS
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#include <boost/accumulators/statistics/kurtosis.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#endif

//undefine this to enable getRNNinput2 where the source of each example is remembered 
//#define REMEMBER_WHICH_CHAR

using std::ostream;

//StrokeDistorter is the worker for
// api fn getRNNInput which generates 
// 8000 of (a representation of each stroke of a distorted training character)
// and 225 of (a representation of each stroke of a test character)
//max #strokes in a char is 26.
//under 2000 have more than 16 strokes - lets ignore them
//  select count(*) from (select * from (select count(*) as s from strokes group by trainxn) where s>16)
//Let the worker do 1/25th of this work (320,9)

//Change it to 128*100=12.8k trains = 25*512
// and 1000 tests = 25*40

//If you want repeatable random numbers, stop setting the seed and use a single thread.

void initialiseRNNInput(RNNInput&r, bool batch){
    r.testX.resize(batch ? nTest_batch_RNN : nBatches_RNN*nTest_batch_RNN);
    r.testY.resize(batch ? nTest_batch_RNN : nBatches_RNN*nTest_batch_RNN);
    r.trainX.resize(batch ? nTrain_batch_RNN : nBatches_RNN*nTrain_batch_RNN);
    r.trainY.resize(batch ? nTrain_batch_RNN : nBatches_RNN*nTrain_batch_RNN);
#ifdef REMEMBER_WHICH_CHAR
    r.testChar.resize(batch ? nTest_batch_RNN : nBatches_RNN*nTest_batch_RNN);
    r.trainChar.resize(batch ? nTrain_batch_RNN : nBatches_RNN*nTrain_batch_RNN);
#endif
}

static double clamp(double x){
  if (x<-1) return -1;
  if (x>1) return 1;
  return x;
}

//?good idea to include initial direction of stroke?
//USER
const int RNN_strokeLevel = 3;

const bool renormalize = true;//true usual: whether to move the character to the bounding box after distortion
const bool rescale = true;//whether to centre and sphere the repn based on a presample of training data
const bool padToCentre = true; //false should be good for RNN - pad the start of character. true good for CNN - pad middle

const int x_breakpoints = 10;
const int y_breakpoints = 10;

//various classes to represent strokes
template<int strokeLevel>
struct LogSigBaseRepn{
  static const int strokeRepnLength = SigLength<2,strokeLevel>::value;
  static void strokeRepn(const Stroke& s, vector<float>& out){
        FLogSignature<2,strokeLevel> o{};
	for(size_t i=1; i!=s.size(); ++i)
	{
	        FSegment<2> displacement;
		displacement[0]=s[i][0]-s[i-1][0];
		displacement[1]=s[i][1]-s[i-1][1];
		joinSegmentToSignatureInPlace<2,strokeLevel>(o,displacement);
	}
	for(int i=0; i<strokeRepnLength; ++i)
	  out[i]=o[i];
  }
  static void summary(ostream& o){o<<"logsig_"<<strokeLevel;}
};

template<int strokeLevel>
struct LogSigFromSigBaseRepn{
  static const int strokeRepnLength = LogSigLength::countNecklacesUptoLengthM(2,strokeLevel);
  static void strokeRepn(const Stroke& s, vector<float>& out){
    auto sig = calcPathSig<strokeLevel>(s);
    auto l = getLogSigFromSignature(sig);
    for(int i=0; i<strokeRepnLength; ++i)
      out[i]=l[i];
  }
  static void summary(ostream& o){o<<"logsigfromsig_"<<strokeLevel;}
  static bool alreadyNormalised(int i){return false;}
};
template<int strokeLevel>
struct SigBaseRepn{
  static const int strokeRepnLength = Signature<2,strokeLevel>::totalLength;
  static void strokeRepn(const Stroke& s, vector<float>& out){
    auto sig = calcPathSig<strokeLevel>(s);
    size_t i=0;
    sig.iterateOver([&](float f){out[i++]=f;});
  }
  static void summary(ostream& o){o<<"sig_"<<strokeLevel;}
  static bool alreadyNormalised(int i){return false;}
};
//typedef LogSigBaseRepn BaseRepn; //USER CHOOSE BASE HERE
//typedef SigBaseRepn BaseRepn;

/*
#ifdef JUSTSIG
const int strokeRepnLength = baseStrokeRepnLength;
  static vector<float> strokeRepn(const Stroke& s){
  #ifdef EASY
    return vector<float>(strokeRepnLength,0);
  #else
    auto sig = calcPathSig<strokeLevel>(s);
    return representSignature(sig);
  #endif
  }
#endif
*/
//Include location as midpoint
template<class BaseRepn>
struct MidpointLabel{
  static const int strokeRepnLength = BaseRepn::strokeRepnLength+2;
  static void strokeRepn(const Stroke& s, vector<float>& out){
  #ifdef EASY
    return;
  #else
    BaseRepn::strokeRepn(s,out);
    size_t i = BaseRepn::strokeRepnLength;
    out[i++]=s[0][0]+s.back()[0];//Missing multiplication by 0.5 is irrelevant as we get scaled later anyway
    out[i++]=s[0][1]+s.back()[1];
  #endif
  }
};
//Include location as midpoint
template<class BaseRepn>
struct MidpointOneHotLabel{
  static const int strokeRepnLength = BaseRepn::strokeRepnLength + x_breakpoints + y_breakpoints;
  static void strokeRepn(const Stroke& s, vector<float>& out){
  #ifdef EASY
    return;
  #else
    BaseRepn::strokeRepn(s,out);
    size_t i = BaseRepn::strokeRepnLength;
    const float x=0.5*(s[0][0]+s.back()[0]);
    const float y=0.5*(s[0][1]+s.back()[1]);
    for (int b=1; b<=x_breakpoints; ++b)
      out[i++]= x>(b*(2.0f/(1+x_breakpoints))-1)? 1 : 0;
    for (int b=1; b<=y_breakpoints; ++b)
      out[i++]= y>(b*(2.0f/(1+y_breakpoints))-1)? 1 : 0;
    //cout<<"\n\n";
    //for(float f:out)
    //  cout<<f<<"\n";
    //std::exit(1);
  #endif
  }
static void summary(ostream& o){BaseRepn::summary(o); o<<"_Location_"<<x_breakpoints<<"_"<<y_breakpoints;}
};
template<class BaseRepn>
struct ProperMidpointOneHotLabel{
  static const int eee=8;
  static const int strokeRepnLength = BaseRepn::strokeRepnLength + 2*eee;
  static void strokeRepn(const Stroke& s, vector<float>& out){
    //cout<<"dkfs;a"<<endl;
    BaseRepn::strokeRepn(s,out);
    float mean0, mean1;
    getFixedLength(s,0,mean0,mean1);
    for(int i=0; i<eee; ++i){
      double comp = 1+2*i;
      int offset = BaseRepn::strokeRepnLength + 2*i;
      out[offset]  =clamp(8*(1+mean0)-comp);
      out[offset+1]=clamp(8*(1+mean1)-comp);
    }
  }
  static void summary(ostream& o){BaseRepn::summary(o); o<<"_LocationM_"<<eee;}
};

struct Match_BG{
  static const int flattenLength = 15;
  static const int eee=8;
  static const int strokeRepnLength=(flattenLength-1)*2 + 2*eee;
  static void strokeRepn(const Stroke& s, vector<float>& out){
    float mean0, mean1;
    Stroke flat = getFixedLength(s,flattenLength,mean0,mean1);
    for(int i=0; i<flattenLength-1; ++i){
      out[2*i]  =3*(flat[i+1][0]-flat[i][0]);
      out[2*i+1]=3*(flat[i+1][1]-flat[i][1]);
    }
    for(int i=0; i<eee; ++i){
      double comp = 1+2*i;
      int offset = (flattenLength-1)*2 + 2*i;
      out[offset]  =clamp(8*(1+mean0)-comp);
      out[offset+1]=clamp(8*(1+mean1)-comp);
    }
  }
  static void summary(ostream& o){o<<"MATCH"<<flattenLength<<"_"<<eee;}
  static bool alreadyNormalised(int i) {return true;}
};

template<class BaseRepn>
struct Match_BG_plus_Base{
  static const int flattenLength = 15;
  static const int eee=8;
  static const int strokeRepnLength=BaseRepn::strokeRepnLength+(flattenLength-1)*2 + 2*eee;
  static void strokeRepn(const Stroke& s, vector<float>& out){
    BaseRepn::strokeRepn(s,out);
    float mean0, mean1;
    Stroke flat = getFixedLength(s,flattenLength,mean0,mean1);
    for(int i=0; i<flattenLength-1; ++i){
      int offset = BaseRepn::strokeRepnLength;
      out[offset+2*i]  =3*(flat[i+1][0]-flat[i][0]);
      out[offset+2*i+1]=3*(flat[i+1][1]-flat[i][1]);
    }
    for(int i=0; i<eee; ++i){
      double comp = 1+2*i;
      int offset = BaseRepn::strokeRepnLength+(flattenLength-1)*2 + 2*i;
      out[offset]  =clamp(8*(1+mean0)-comp);
      out[offset+1]=clamp(8*(1+mean1)-comp);
    }
  }
  static void summary(ostream& o){o<<"MATCH"<<flattenLength<<"_"<<eee<<"+";BaseRepn::summary(o);}
  static bool alreadyNormalised(int i){
    if(i<BaseRepn::strokeRepnLength)
      return BaseRepn::alreadyNormalised(i);
    return true;
  }
};

template<class Orig, int Size=9>
struct Clamped
{
  static const int strokeRepnLength = Orig::strokeRepnLength;
  static void strokeRepn(const Stroke& s, vector<float>& out){
    Orig::strokeRepn(s,out);
    for(float& f : out){
      if(f> Size) f=Size;
      if(f<-Size) f=-Size;
    }
  }
  static void summary(ostream& o){Orig::summary(o); o<<"_clam"<<Size;}
  static bool alreadyNormalised(int i) {return Orig::alreadyNormalised(i);}
};

//typedef Match_BG Repn; //USER
//typedef Match_BG_plus_Base<LogSigBaseRepn> Repn;
//typedef Match_BG_plus_Base<LogSigFromSigBaseRepn<RNN_strokeLevel>> Repn;
typedef Match_BG_plus_Base<SigBaseRepn<RNN_strokeLevel>> Repn;
//typedef Clamped<Match_BG_plus_Base<SigBaseRepn<RNN_strokeLevel>>,25> Repn;

static const int strokeRepnLength = Repn::strokeRepnLength;
LUAFUNC int getStrokeRepnLength(){return strokeRepnLength;}
std::string getStrokeRepnName(){
  std::ostringstream oss;
  Repn::summary(oss);
  if(renormalize)
    oss<<'n';
  if(padToCentre)
    oss<<"_ctrPad";
  if(!rescale)
    oss<<"_NoScale";
  return oss.str();
}

DataCopy g_dataCopy;

static void normalizeCharacter(Character& x){
  //  std::array<float,2> mx{-100,-100}, mn{100,100}; //clang hates this, it's a bug
  std::array<float,2> mx, mn; 
  mx[0]=mx[1]=-100; mn[0]=mn[1]=100;
  for(const auto& s :x)
    for (const auto& p : s)
      for(int i=0; i<2; ++i){
	if(p[i]<mn[i])
	  mn[i]=p[i];
	if(p[i]>mx[i])
	  mx[i]=p[i];
      }
  using std::min;
  using std::max;
  float shrinkF = min(10.0f,1.0f/max(mx[0]-mn[0],mx[1]-mn[1]));
  for(auto& s : x)
    for(auto& p : s)
      for(int i=0; i<2; ++i)
	p[i]=(2*p[i]-mx[i]-mn[i])*shrinkF;
  bool validate = false;
  if(validate){
    float mn = 100, mx = -100;
    for(const auto& s :x)
      for (const auto& p : s)
	for(int i=0; i<2; ++i){
	  if(p[i]<mn)
	    mn=p[i];
	  if(p[i]>mx)
	    mx=p[i];
	}
    cout<<"Checkme: "<<mn<<","<<mx<<endl;
  }

}

class StrokeDistorter
{
  std::random_device m_rd;
  std::mt19937 m_rnd;
  struct Scaler{
    vector<double> m_means;
    vector<double> m_inv_sds;
  };
  static Scaler g_scaler;

  static void charToRepns(const Character& x, vector<float>& o){
    o.assign(RNN_maxTime*strokeRepnLength,0);
    auto outIt = o.rbegin();
    if(padToCentre){
      int nBlanks = RNN_maxTime-x.size();
      outIt += strokeRepnLength * (nBlanks/2);
    }
    for(auto i=x.size(); i>0; --i){
      const Stroke& s = x[i-1];
      vector<float> r(Repn::strokeRepnLength);
      Repn::strokeRepn(s,r);//share this?
      for(int i=0; i!=strokeRepnLength; ++i){
	if(rescale)
	  *(outIt++)=(r[i]-g_scaler.m_means[i])*g_scaler.m_inv_sds[i];
	else
	  *(outIt++)=r[i];
      }
    }
  }
public:
	StrokeDistorter():m_rnd(m_rd())
	{
	}
	typedef RNNInput result_type;
	result_type operator()()
	{
	        result_type out;
		initialiseRNNInput(out,true);
		std::uniform_int_distribution<int> uid_train(1,g_dataCopy.m_trainX.size());
		std::uniform_int_distribution<int> uid_test(1,g_dataCopy.m_testX.size());
		for(int i=0; i<nTrain_batch_RNN; ++i){
		  Character x;
		  int index;
		  do{
		    index = uid_train(m_rnd);
		    x = g_dataCopy.m_trainX[index-1];
		  }
		  while(x.size()>RNN_maxTime);
		  distortCharacter(x, m_rnd);
		  if(renormalize)
		    normalizeCharacter(x);
		  vector<float>& o = out.trainX[i];
		  out.trainY[i]=g_dataCopy.m_trainY[index-1];
#ifdef REMEMBER_WHICH_CHAR
		  out.trainChar[i]=index-1;
#endif
		  charToRepns(x,o);
#ifdef CHECK_FOR_LARGE_FEATURE_VALUES
		  static bool printed = false;
		  if(!printed)
		  {
		    const float toobig = 30;
		    bool p  = false;
		    for(float f:o)
		      if (f>toobig)
			p=printed=true;
		    if(p){
		      std::cout<<"Whoops: "<<index<<"\n";
		      for(const Stroke& s: x){
			vector<float> v(Repn::strokeRepnLength);
			Repn::strokeRepn(s,v);
			auto v2=v;
			int i=0;
			bool p2=false;
			if(rescale)
			  for(float& ff :v2){
			    ff=(ff-g_scaler.m_means[i])*g_scaler.m_inv_sds[i];
			    if(ff>toobig)
			      p2=true;
			    ++i;
			  }
			if(p2){
			  for(float ff : v)
			    cout<<ff<<",";
			  cout<<"\n";
			  for(float ff : v2)
			    cout<<ff<<",";
			  cout<<"\n";
			}
		      }
		      cout<<endl;
		    }
		  }
#endif		  
		}		  
		for(int i=0; i<nTest_batch_RNN; ++i){
		  Character x;
		  int index;
		  do{
		    index = uid_test(m_rnd);
		    x = g_dataCopy.m_testX[index-1];
		  }
		  while(x.size()>RNN_maxTime);
		  vector<float>& o = out.testX[i];
		  out.testY[i]=g_dataCopy.m_testY[index-1];
#ifdef REMEMBER_WHICH_CHAR
		  out.testChar[i]=index-1;
#endif
		  charToRepns(x,o);
		}		  
		return out;
	}
  static void makeScalings(){
#ifdef CALC_STATS
    using namespace boost::accumulators;
    int sampleSize = 10000;
    std::mt19937 mt;
    std::uniform_int_distribution<int> uid_train(1,g_dataCopy.m_trainX.size());
    //    std::vector<accumulator_set<double,stats<tag::mean,tag::variance,tag::min,tag::max,tag::skewness,tag::kurtosis>>>  ss(strokeRepnLength);
    std::vector<accumulator_set<double,stats<tag::mean,tag::variance>>>  ss(strokeRepnLength);
    for(int i=0;i<=sampleSize; ++i){
		  Character x;
		  int index;
		  do{
		    index = uid_train(mt);
		    x = g_dataCopy.m_trainX[index-1];
		  }
		  while(x.size()>RNN_maxTime);
		  distortCharacter(x, mt);
		  if(renormalize)
		    normalizeCharacter(x);
		  for(const Stroke& s: x){
		    vector<float> v(Repn::strokeRepnLength);
                    Repn::strokeRepn(s, v);
		    for(int j=0; j!=strokeRepnLength; ++j)
		      ss[j](v[j]);
		  }
    }
    g_scaler.m_means.assign(strokeRepnLength,0.0);
    g_scaler.m_inv_sds.assign(strokeRepnLength,1.0);
    for(int i=0; i!=strokeRepnLength; ++i){
      if(!Repn::alreadyNormalised(i)){
	g_scaler.m_means[i]=mean(ss[i]);
	g_scaler.m_inv_sds[i]=1.0/sqrt(variance(ss[i]));
      }
//cout<<g_scaler.m_means[i]<<" "<<1/g_scaler.m_inv_sds[i]<<" "<<max(ss[i])<<" "<<min(ss[i])<<std::endl; 
//      cout<<kurtosis(ss[i])<<" "<<skewness(ss[i])<<std::endl; 
    }
#endif
  }
  //just represent a character and throw it away.
  //This means that, if libalgebra is involved in the representation, it will have calculated any static information it needs,
  //making it safe to compile libalgebra without any mutexes
  static void mainThreadCalcOne(){
    for(const Stroke& s : g_dataCopy.m_trainX[0]){
      vector<float> v(Repn::strokeRepnLength);
      Repn::strokeRepn(s,v);
    }
  }
};

StrokeDistorter::Scaler StrokeDistorter::g_scaler;

std::thread g_setup;
void makeAllScalings(); //def'd below
static void checkScalings();

LUAFUNC void setDataCopyAsync(const char* data){
  std::string d = data;
  //  g_data_waiter = std::async(std::launch::async,[d](){
  g_setup=std::thread([d](){
      const char* s = d.c_str();
      bool abbreviated = (*s == '?');
      if(abbreviated){
	++s;
      }
      makeDataCopy(s, g_dataCopy, abbreviated);
      makeAllScalings();
  });
  //checkScalings();
}
LUAFUNC void waitForOpenDataCopy(){
  //  g_data_waiter.get();
  g_setup.join();
}

RNNInput getRNNinput_readyForPython(){
  //static AlternatingAsyncs<StrokeDistorter,2,30> g_strokeDistorter;
  static AlternatingAsyncs<StrokeDistorter,2,4> g_strokeDistorter;
  //static StrokeDistorter g_strokeDistorter;
  RNNInput all;
  initialiseRNNInput(all,false);
  for (int batch =0; batch<nBatches_RNN; ++batch){
    auto inp = g_strokeDistorter();
    std::move(inp.trainX.begin(), inp.trainX.end(), all.trainX.begin()+batch*nTrain_batch_RNN);
    std::move(inp.testX.begin(), inp.testX.end(), all.testX.begin()+batch*nTest_batch_RNN);
    std::move(inp.trainY.begin(), inp.trainY.end(), all.trainY.begin()+batch*nTrain_batch_RNN);
    std::move(inp.testY.begin(), inp.testY.end(), all.testY.begin()+batch*nTest_batch_RNN);
#ifdef REMEMBER_WHICH_CHAR
    std::move(inp.trainChar.begin(), inp.trainChar.end(), all.trainChar.begin()+batch*nTrain_batch_RNN);
    std::move(inp.testChar.begin(), inp.testChar.end(), all.testChar.begin()+batch*nTest_batch_RNN);
#endif
  }
  return all;
}

//not currently one-hot
//length assumed correct
LUAFUNC void getRNN_input(float* trainX, int* trainY,
			  float* testX, int* testY){ //short should do for the y's
      static AlternatingAsyncs<StrokeDistorter,2,30> g_strokeDistorter;
  for (int batch =0; batch<nBatches_RNN; ++batch){
    auto inp = g_strokeDistorter();
    for(int sample = 0; sample<nTrain_batch_RNN; ++sample){
      std::move(inp.trainX[sample].begin(), inp.trainX[sample].end(), trainX+(batch*nTrain_batch_RNN+sample)*RNN_maxTime*strokeRepnLength);
    }
    for(int sample = 0; sample<nTest_batch_RNN; ++sample)
      std::move(inp.testX [sample].begin(), inp.testX [sample].end(), testX +(batch*nTest_batch_RNN +sample)*RNN_maxTime*strokeRepnLength);
    std::move(inp.trainY.begin(), inp.trainY.end(), trainY+batch*nTrain_batch_RNN);
    std::move(inp.testY.begin(), inp.testY.end(), testY+batch*nTest_batch_RNN);
  }
}

LUAFUNC int get_nTrain(){return nTrain_batch_RNN*nBatches_RNN;}
LUAFUNC int get_nTest(){return nTest_batch_RNN*nBatches_RNN;}
LUAFUNC int get_maxTime(){return RNN_maxTime;}

// - VARIABLE LENGTH STUFF -everything again, different structures
//no scaling yet

namespace VariableLength{
  struct FixedDistanceEachStroke{
    constexpr static double distance = 0.3;
    static const int length = 2;
    static void r(vector<float>& o, Character& in){ //try this
      float ignore1, ignore2;
      for(const Stroke s : in){
	auto s2 = getFixedDistance(s,distance);
	for(Point2d i : s2){
	  o.push_back(i[0]);
	  o.push_back(i[1]);
	}
      }
    }
    static void str(std::ostream& o){o<<"FixedDistanceEachStroke"<<distance;}
  };
  struct FixedDistanceEverything{
    constexpr static double distance = 0.3;
    static const int length = 2;
    static void r(vector<float>& o, Character& in){ //try this
      float ignore1, ignore2;
      Stroke s = concat(in);
      auto s2 = getFixedDistance(s,distance);
      for(Point2d i : s2){
	o.push_back(i[0]);
	o.push_back(i[1]);
      }
    }
    static void str(std::ostream& o){o<<"FixedDistanceEverything"<<distance;}
  };
  struct FixedDistanceEverythingPenDim{
    constexpr static double distance = 0.3;
    static const int length = 3;
    static void r(vector<float>& o, Character& in){ //try this
      float ignore1, ignore2;
      Stroke3d s = concatWithPenDimension(in);
      auto s2 = getFixedDistanceUsing2d(s,distance,nullptr);
      for(Point3d i : s2){
	o.push_back(i[0]);
	o.push_back(i[1]);
	o.push_back(i[2]);
      }
    }
    static void str(std::ostream& o){o<<"FixedDistanceEverythingPenDim"<<distance;}
  };
  struct FixedDistancePenDir{
    constexpr static double distance = 0.3;
    static const int length = 5;
    static void r(vector<float>& o, Character& in){ //try this
      float ignore1, ignore2;
      Stroke3d s = concatWithPenDimension(in);
      vector<float> angles;
      auto s2 = getFixedDistanceUsing2d(s,distance,&angles);
      auto a = angles.begin();
      for(Point3d i : s2){
	o.push_back(i[0]);
	o.push_back(i[1]);
	o.push_back(i[2]);
	o.push_back(sin(*a));
	o.push_back(cos(*a++));
      }
    }
    static void str(std::ostream& o){o<<"FixedDistancePenDir"<<distance;}
  };

  struct PenChunkSig{
    constexpr static double distance = 0.4;
    constexpr static int level = 2;
    static const int length = 3+Signature<3,level>::totalLength;
    static void r(vector<float>& o, Character& in){ //try this
      Stroke3d s = concatWithPenDimension(in);
      auto chunks = chunkUsing2d(s, distance);
      for(const auto& chunk : chunks){
	auto& i = chunk[0];
	o.push_back(i[0]);
	o.push_back(i[1]);
	o.push_back(i[2]);
	auto sig = calcPathSig<level>(chunk);
	sig.iterateOver([&](float f){o.push_back(f);});
      }
    }
    static void str(std::ostream& o){o<<"PenChunkSig"<<distance<<"_"<<level;}
  };


  //typedef FixedDistanceEverythingPenDim R;
  typedef FixedDistancePenDir R;
  //typedef PenChunkSig R;
};


LUAFUNC int getVariableLengthStrokeRepnLength(){return VariableLength::R::length;}
std::string getVariableLengthRepnName(){
  std::ostringstream oss;
  VariableLength::R::str(oss);
  return oss.str();
}

RNNInput getRNNinput_variableLength_readyForPython(){
  RNNInput out;
  initialiseRNNInput(out,false);
  static std::mt19937 rnd;
  std::uniform_int_distribution<int> uid_train(1,g_dataCopy.m_trainX.size());
  std::uniform_int_distribution<int> uid_test(1,g_dataCopy.m_testX.size());
  for(int i=0; i<nTrain_batch_RNN*nBatches_RNN; ++i){
    Character x;
    int index;
    do{
      index = uid_train(rnd);
      x = g_dataCopy.m_trainX[index-1];
    }
    while(x.size()>RNN_maxTime);
    distortCharacter(x, rnd);
    if(renormalize)
      normalizeCharacter(x);
    out.trainY[i]=g_dataCopy.m_trainY[index-1];
#ifdef REMEMBER_WHICH_CHAR
    out.trainChar[i]=index-1;
#endif
    VariableLength::R::r(out.trainX[i],x);
  }		  
  for(int i=0; i<nTest_batch_RNN*nBatches_RNN; ++i){
    Character x;
    int index;
    do{
      index = uid_test(rnd);
      x = g_dataCopy.m_testX[index-1];
    }
    while(x.size()>RNN_maxTime);
    vector<float>& o = out.testX[i];
    out.testY[i]=g_dataCopy.m_testY[index-1];
#ifdef REMEMBER_WHICH_CHAR
    out.testChar[i]=index-1;
#endif
    VariableLength::R::r(out.testX[i],x);
  }
  return out;
}

//The following substitute for the above function just returns right-sized junk
RNNInput getRNNinput_variableLength_readyForPython2(){
  RNNInput out;
  initialiseRNNInput(out,false);
  static std::mt19937 rnd;
  std::uniform_int_distribution<int> label(0,3754);
  std::uniform_real_distribution<float> data(0,1);
  std::bernoulli_distribution bother(0.6);
  size_t timeLength = 120;
  bool bothering = false;//bother(rnd);
  for(int i=0; i<nTrain_batch_RNN*nBatches_RNN; ++i){
    out.trainX[i].resize(timeLength*VariableLength::R::length);
    //    for(float& f : out.trainX[i])
    //f=data(rnd);
    if(bothering)
      for(int j=0; j<80; ++j)
	out.trainX[i].at(timeLength-1-j) = data(rnd);
    out.trainY[i]=label(rnd);
  }
  for(int i=0; i<nTest_batch_RNN*nBatches_RNN; ++i){
    out.testX[i].resize(timeLength*VariableLength::R::length);
    for(float& f : out.testX[i])
      f=data(rnd);
    out.testY[i]=label(rnd);
  }
  return out;
}




//call in python eg with import ctypes; ctypes.cdll.LoadLibrary("embed.so").ttt()
/*
LUAFUNC void ttt(){
  Stroke s(3);
  s[0][0]=s[0][1]=1;
  s[1][0]=2; s[1][1]=1;
  s[2][0]=s[2][1]=3;
  float m0, m1;
  for(const auto& a : getFixedLength(s,14,m0,m1)){
    cout<<a[0]<<","<<a[1]<<"\n";
  }
  cout<<"("<<m0<<","<<m1<<")\n";
  cout<<endl;
  
}
*/

//---PYRAMID

const int nLayers = 2;
LUAFUNC int numLayers(){return nLayers;}
const int layerMinibatchSize_ = 125;
LUAFUNC int layerMinibatchSize(){return layerMinibatchSize_;}

using Layer1Repn = SigBaseRepn<3>;
using Layer0Repn = Match_BG_plus_Base<SigBaseRepn<2>>;
const int layer01 = 3; //merge how many

LUAFUNC int layerTimePoolingAfter(int){return layer01;}
LUAFUNC int layerRepnLength(int layer){
  if(layer == 0)
    return Layer0Repn::strokeRepnLength;
  return Layer1Repn::strokeRepnLength;
}
LUAFUNC int layerTimeLength(int layer){
  if(layer == 0)
    return RNN_maxTime;
  int prevLayerLength = layerTimeLength(layer-1);
  int filter = layerTimePoolingAfter(layer-1), stride = filter;
  int possiblePoolings = prevLayerLength + 1 - filter;
  return (possiblePoolings + stride - 1) / stride;
}

constexpr bool layerIsUnpooled(int layer){
  return layer==0;
}

constexpr int poolLength(int layer){
  return (layerIsUnpooled(layer)) ? 1 :
    layerTimePoolingAfter(layer-1)*poolLength(layer-1);
}

using std::tuple;
//using Layers = tuple<Layer0,Layer1>;

class PyramidWorker
{
  std::random_device m_rd;
  std::mt19937 m_rnd;
  static void charToRepns(const Character& x, vector<vector<float>>& o){
    static bool check = true;
    o.resize(nLayers);
    for(int layer = 0; layer<nLayers; ++layer){
      o[layer].assign(layerTimeLength(layer)*layerRepnLength(layer),0);
      auto outIt = o[layer].rbegin();
      if(layerIsUnpooled(layer)){
	for(auto i=x.size(); i>0; --i){
	  const Stroke& s = x[i-1];
	  vector<float> r(Layer0Repn::strokeRepnLength);
	  Layer0Repn::strokeRepn(s, r);//share this?
	  for(int i=0; i!=Layer0Repn::strokeRepnLength; ++i){
	    if(rescale)
	      *(outIt++)=345634;//(r[i]-g_scaler.m_means[i])*g_scaler.m_inv_sds[i];
	    else
	      *(outIt++)=r[i];
	  }
	}
      }
      else{
	const int thisPoolLength = poolLength(layer);
	size_t iStroke = 0;
	while(iStroke<x.size()){
	  std::cout<<"\n";
	  std::cout<<iStroke<<",";
	  Stroke pooled = x[iStroke++];
	  for(size_t i=1; i<thisPoolLength && iStroke<x.size(); ++i, ++iStroke){
	    pooled.insert(pooled.end(),x[iStroke].begin(),x[iStroke].end());
	    if(check)
	      std::cout<<iStroke<<",";
	  }
	  vector<float> r(Layer1Repn::strokeRepnLength);
	  Layer1Repn::strokeRepn(pooled, r);
	  for(int i=0; i!=Layer1Repn::strokeRepnLength; ++i){
	    if(rescale)
	      *(outIt++)=345634;//(r[i]-g_scaler.m_means[i])*g_scaler.m_inv_sds[i];
	    else
	      *(outIt++)=r[i];
	  }
	}
      }
    }
    check = false;
  }
public:
  PyramidWorker():m_rnd(m_rd())
  {
  }
  static void resizeLayerInput(LayerInput& l, size_t r){
    l.testX.resize(r);
    l.trainX.resize(r);
    l.testY.resize(r);
    l.trainY.resize(r);
  }

  typedef LayerInput result_type;
  result_type operator()()
  {
    result_type out;
    resizeLayerInput(out,layerMinibatchSize_);
    std::uniform_int_distribution<int> uid_train(1,g_dataCopy.m_trainX.size());
    std::uniform_int_distribution<int> uid_test(1,g_dataCopy.m_testX.size());
    for(int i=0; i<layerMinibatchSize_; ++i){
      Character x;
      int index;
      do{
	index = uid_train(m_rnd);
	x = g_dataCopy.m_trainX[index-1];
      }
      while(x.size()>RNN_maxTime);
      distortCharacter(x, m_rnd);
      if(renormalize)
	normalizeCharacter(x);
      auto& o = out.trainX[i];
      out.trainY[i]=g_dataCopy.m_trainY[index-1];
      charToRepns(x,o);
    }		  
    for(int i=0; i<layerMinibatchSize_; ++i){
      Character x;
      int index;
      do{
	index = uid_test(m_rnd);
	x = g_dataCopy.m_testX[index-1];
      }
      while(x.size()>RNN_maxTime);
      auto& o = out.testX[i];
      out.testY[i]=g_dataCopy.m_testY[index-1];
      charToRepns(x,o);
    }		  
    return out;
  }
  static void makeScalings(){
#if 0
#ifdef CALC_STATS
    using namespace boost::accumulators;
    int sampleSize = 10000;
    std::mt19937 mt;
    std::uniform_int_distribution<int> uid_train(1,g_dataCopy.m_trainX.size());
    std::vector<accumulator_set<double,stats<tag::mean,tag::variance>>>  ss(strokeRepnLength);
    for(int i=0;i<=sampleSize; ++i){
      Character x;
      int index;
      do{
	index = uid_train(mt);
	x = g_dataCopy.m_trainX[index-1];
      }
      while(x.size()>RNN_maxTime);
      distortCharacter(x, mt);
      if(renormalize)
	normalizeCharacter(x);
      for(const Stroke& s: x){
	vector<float> v(Repn::strokeRepnLength);
	Repn::strokeRepn(s, v);
	for(int j=0; j!=strokeRepnLength; ++j)
	  ss[j](v[j]);
      }
    }
    g_scaler.m_means.assign(strokeRepnLength,0.0);
    g_scaler.m_inv_sds.assign(strokeRepnLength,0.0);
    for(int i=0; i!=strokeRepnLength; ++i){
      g_scaler.m_means[i]=mean(ss[i]);
      g_scaler.m_inv_sds[i]=1.0/sqrt(variance(ss[i]));
      //cout<<g_scaler.m_means[i]<<" "<<g_scaler.m_inv_sds[i]<<std::endl; 
    }
#endif
#endif
  }
};

LayerInput getLayerInput_readyForPython(){
  static PyramidWorker worker;
  return worker();
}
//LUAFUNC void getRNN_input(int layer, float* trainX, int* trainY,
//			  float* testX, int* testY){

LUAFUNC void getLayerData(int layer, float* dest){
  static PyramidWorker worker;
  static PyramidWorker::result_type res;
  if(layer==0){
    res=worker();
    
  }


}

//LUAFUNC string f1(){return "sadfh";}
LUAFUNC const char * f2(){return "sadfh";}
std::array<double,3> food {{3.4,543.3,23.3}};
LUAFUNC double* f3(){return food.data();}



void makeAllScalings(){
  if(rescale){
    StrokeDistorter::makeScalings();
  }
  else
    StrokeDistorter::mainThreadCalcOne();
  if(rescale)
    PyramidWorker::makeScalings();
}

static void checkScalings(){
#ifdef CALC_STATS
    using namespace boost::accumulators;
    int sampleSize = 100000, length = 600;
    std::mt19937 mt;
    std::uniform_real_distribution<float> rea(-1,1);
    using R = SigBaseRepn<4>;
    std::vector<accumulator_set<double,stats<tag::mean,tag::variance,tag::min,tag::max,tag::skewness,tag::kurtosis>>>  ss(R::strokeRepnLength);
    for(int i=0;i<=sampleSize; ++i){
      Stroke s;
      for(int j=1; j<=length; ++j){
	Point2d p;
	p[0]=rea(mt); p[1] = rea(mt);
	s.push_back(p);
      }
      vector<float> v(R::strokeRepnLength);
      R::strokeRepn(s, v);
      for(int j=0; j!=R::strokeRepnLength; ++j)
	ss[j](v[j]);
    }
    for(int i=0; i!=R::strokeRepnLength; ++i){
      cout<<mean(ss[i])<<" "<<variance(ss[i])<<" "<<skewness(ss[i])<<" "<<kurtosis(ss[i])<<std::endl; 
      cout<<min(ss[i])<<" "<<max(ss[i])<<std::endl; 
    }
#endif
  }

static void checkScalings2(){
  //BM
#ifdef CALC_STATS
    using namespace boost::accumulators;
    int sampleSize = 1000000, length = 600;
    std::mt19937 mt;
    std::normal_distribution<float> rea(0,0.1);
    using R = SigBaseRepn<6>;
    std::vector<accumulator_set<double,stats<tag::mean,tag::variance,tag::min,tag::max,tag::skewness,tag::kurtosis>>>  ss(R::strokeRepnLength);
    for(int i=0;i<=sampleSize; ++i){
      Stroke s;
      s.reserve(1+length);
      Point2d last{};
      s.push_back(last);
      for(int j=1; j<=length; ++j){
	last[0]+=rea(mt); last[1]+=rea(mt);
	s.push_back(last);
      }
      vector<float> v(R::strokeRepnLength);
      R::strokeRepn(s, v);
      for(int j=0; j!=R::strokeRepnLength; ++j)
	ss[j](v[j]);
    }
    for(int i=0; i!=R::strokeRepnLength; ++i){
      cout<<i<<" "<<mean(ss[i])<<" "<<variance(ss[i])<<" "<<skewness(ss[i])<<" "<<kurtosis(ss[i])<<std::endl; 
      cout<<min(ss[i])<<" "<<max(ss[i])<<std::endl; 
    }
#endif
  }

static void writeRandomSample(){
  int sampleSize = 100000, length = 60;
  sampleSize=2;
  std::mt19937 mt;
  std::uniform_real_distribution<float> rea(-1,1);
  using R = SigBaseRepn<4>;
  //auto db = JRSqlite::Connection("foo.sqlite");
  auto db = JRSqlite::Connection::openNewCarefree("foo.sqlite");
  JRSqlite::SimpleStmt(db,"create table A(sample int, pos int, value real)").execute();
  JRSqlite::Sink2IntDoubleStmt put(db,"insert into A values (?,?,?)");
  for(int i=0;i<=sampleSize; ++i){
      Stroke s;
      for(int j=1; j<=length; ++j){
	Point2d p;
	p[0]=rea(mt); p[1] = rea(mt);
	s.push_back(p);
      }
      vector<float> v(R::strokeRepnLength);
      R::strokeRepn(s, v);
      for(int j=0; j!=R::strokeRepnLength; ++j)
	put.execute(i,j,v[j]);
    }
}

