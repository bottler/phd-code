#include "stdafx.h"
#include "SigUtilities.h"
#include "bch.h"

#include "jrutils/checkAndCall.h"

#ifdef LINUX
#include "Eigen/LU" //Jacobi?
#include "Eigen/SVD" //Jacobi?
//#include "Eigen/Dense" 
#endif


template<int M>
LogSignature<2,M> calcPathLogSigBCH(const Stroke& in, bool backwards=false)
{
        LogSignature<2,M> o{};
	for(size_t i=1; i!=in.size(); ++i)
	{
	        Segment<2> displacement;
		displacement[backwards?1:0]=in[i][0]-in[i-1][0];
		displacement[backwards?0:1]=in[i][1]-in[i-1][1];
		//displacement[2]=in[i][2]-in[i-1][2];
		joinSegmentToSignatureInPlace<2,M>(o,displacement);
	}
	return o;
}

template<int M>
FLogSignature<2,M> calcPathLogSigBCHf(const Stroke& in)
{
        FLogSignature<2,M> o{};
	for(size_t i=1; i!=in.size(); ++i)
	{
	        FSegment<2> displacement;
		displacement[0]=in[i][0]-in[i-1][0];
		displacement[1]=in[i][1]-in[i-1][1];
		//displacement[2]=in[i][2]-in[i-1][2];
		joinSegmentToSignatureInPlace<2,M>(o,displacement);
	}
	return o;
}

/*
vector<float> calcLevel5LogSigBCHf(const Stroke& in)
{
  const int M = 5;
  auto a = calcPathLogSigBCHf<M>(in);
  vector<float> out;
  for(auto i : a)
    out.push_back(i);
  return out;
}
*/

//transform a vector of logsignature<2,M> elements so that level 2 gets sqrt, level 3 cuberoot etc
template<int M>
void rescaleLogSig(vector<float>& f)
{
  for(int i=SigLength<2,M>::value-1; i>=SigLength<2,M-1>::value; --i)
    f[i]=std::copysign(std::pow(std::abs(f[i]),1.0/M),f[i]);
  rescaleLogSig<M-1>(f);
}
template<> void rescaleLogSig<1>(vector<float>& f){}


template<int M>
struct LogSigBCHfinplace{
  static void go(const Stroke& in, vector<float>& out, bool rescale)
  {
    auto a = calcPathLogSigBCHf<M>(in);
    for(auto i : a)
      out.push_back(i);
    if(rescale)
      rescaleLogSig<M>(out);
  }
};


vector<float> calcLogSigBCHf(const Stroke& in, int level, bool rescale)
{
  vector<float> out;
  //checkAndCall<6,LogSigBCHfinplace>::go(level,"log signature",std::cref(in),std::ref(out),rescale);
  /*  if(level==10)
    LogSigBCHfinplace<10>::go(in,out,rescale);
  else if(level==6)
    LogSigBCHfinplace<6>::go(in,out,rescale);
  else if(level==2)
    LogSigBCHfinplace<2>::go(in,out,rescale);
    else*/
    throw std::runtime_error("aljkah");
  return out;
}


void bchtime(){
  	std::random_device rndd;
	std::mt19937 rnd(rndd());
	std::uniform_real_distribution<float> urd(-1,1);
	const int repeats = 60;
	const int tries=2311;
	const int M = 6;
	Stroke d;
	cout <<"start"<<std::endl;
	double time;
	{
	  SecondsCounter sc(time);
	  for(int repeat=0; repeat<repeats; ++repeat){
	    d.clear();
	    for(int i=0; i!=12; ++i){
	      Point2d b;
	      b[0]=urd(rnd);
	      b[1]=urd(rnd);
	      //b[2]=urd(rnd);
	      d.push_back(b);
	    }
	    auto s1=calcPathSig<M>(d);
	    auto l1=getLogSigFromSignature(s1);
	  }
	}
	cout<<"old:  "<<time<<endl;
	{/*
	  SecondsCounter sc(time);
	  for(int repeat=0; repeat<repeats; ++repeat){
	    d.clear();
	    for(int i=0; i!=12; ++i){
	      Point2d b;
	      b[0]=urd(rnd);
	      b[1]=urd(rnd);
	      //b[2]=urd(rnd);
	      d.push_back(b);
	    }
	    auto s1=calcPathLogSigBCH<M>(d);
	    }*/
	}
	cout<<"new "<<time<<endl;
      
	
	
}

static double r(double x){
  const double tol = 0.001;
  if(fabs(x)<tol)
    return 0;
  if(fabs(x-1)<tol)
    return 1;
  if(fabs(x+1)<tol)
    return -1;
  return x;
}

static void ro(Eigen::MatrixXd& xs){
  size_t i = 0, nRows = xs.rows(), nCols = xs.cols();
  //cout<<"#####"<<nRows<<" "<<nCols<<"\n\n";
  for (; i < nCols; ++i)
    for(size_t j=0; j<nRows; ++j)
      xs(i,j) = r(xs(i,j));
}

//the two bases are different, even if you reverse the order of the coordinates.
void bchcompareOldNew(){
#if false
	std::random_device rndd;
	std::mt19937 rnd(rndd());
	std::uniform_real_distribution<float> urd(-1,1);
	const int repeats = 991;
	const int M = 6;
	const int dim = SigLength<2,M>::value;
	Stroke d;
	//cout <<"start"<<std::endl;
	Eigen::MatrixXd olds(repeats, dim), news2(repeats, dim);
	vector<Eigen::VectorXd> news;
	news.reserve(dim);
	for(int i=0; i!=dim; ++i)
	{
	  news.push_back(Eigen::VectorXd(repeats));
	}
	bool dorepeats=false;
	for(int repeat=0; repeat<(dorepeats?repeats:1); ++repeat){
	    d.clear();
	    for(int i=0; i!=12; ++i){
	      Point2d b;
	      b[0]=urd(rnd);
	      b[1]=urd(rnd);
	      d.push_back(b);
	    }
	    auto s1=calcPathSig<M>(d);
	    auto l1=getLogSigFromSignature(s1);
	    for(int i=0; i!=dim; ++i)
	      olds(repeat,i)=l1[i];
	    auto l2=calcPathLogSigBCH<M>(d);
	    for(int i=0; i!=dim; ++i){
	      news[i][repeat]=l2[i];
	      news2(repeat,i)=l2[i];
	    }
	    if(repeat==0)
	    for (int i=0; i!=l2.size(); ++i)
	      cout<<i<<" "<<l1[i]-l2[i]<<" "<<l1[i]<<" "<<l2[i]<<"\n";
	    //cout<<"old:  "<<endl;
	}
	if(dorepeats){
	  auto m = olds.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV);
    
	  Eigen::MatrixXd foo = m.solve(news2);
	  ro(foo);
	  Eigen::MatrixXd foo2 = foo.inverse();
	  ro(foo2);
	  cout<<foo<<"\n\n"<<foo2<<endl;
	/*for(int i=0; i!=dim; ++i)
	  {
	    Eigen::VectorXd x = m.solve(news[i]);
	    for(int i=0; i!=dim; ++i)
	      //	    for (auto i : x)
	      
	      cout<<r(x[i])<<",";
	    cout<<endl;
	    }*/
	}
	if(0)
	{	
	  const int MM=7;
	  alg::lie<float,float,2,MM>::basis.growup(MM);
	  auto b=alg::lie<float,float,2,MM>::basis;
		for (auto k = b.begin(); k != b.end(); k = b.nextkey(k))
			cout << b.key2string(k) << '\n';
	}
#endif
}

void bchCompareFloatDouble(){
 	std::random_device rndd;
	std::mt19937 rnd(rndd());
	std::uniform_real_distribution<float> urd(-1,1);
	const int repeats = 2;
	const int tries=2311;
	const int M = 6;//9;
	//const int dim = SigLength<2,M>::value;
	Stroke d;
	cout <<"start"<<std::endl;

        /*
	for(int repeat=0; repeat<repeats; ++repeat){
	    d.clear();
 	    for(int i=0; i!=120; ++i){
	      Point2d b;
	      b[0]=urd(rnd);
	      b[1]=urd(rnd);
	      d.push_back(b);
	    }
	    auto l1=calcPathLogSigBCHf<M>(d);
	    auto l2=calcPathLogSigBCH<M>(d);
	    for (size_t i=0; i!=l2.size(); ++i)
	      cout<<i<<" "<<l1[i]-l2[i]<<" "<<l1[i]<<" "<<l2[i]<<"\n";
	    //cout<<"old:  "<<endl;
	    }*/
}

/*
void bchExpandDimTest()
{
  std::random_device rndd;
  std::mt19937 rnd(rndd());
  std::uniform_real_distribution<float> urd(-1,1);
  Stroke3d foo;
  for(int i=0; i!=120; ++i){
    Point3d b;
    //b[0]=urd(rnd);
    b[1]=urd(rnd);
    b[2]=urd(rnd);
    foo.push_back(b);
  }
  auto s = getLogSig<3,7>(foo);
  for(auto&& i : s)
    cout<<i<<endl;
}*/

void bch(){
  bchcompareOldNew();
}



/*
template<int i>
struct foo1
{
  static void go () {cout<<"hello from number "<< i<<endl;}
};

*/
