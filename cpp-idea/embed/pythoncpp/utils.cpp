#include "stdafx.h"
#include "SigUtilities.h"
#include "calcSignature.h"

#ifdef LINUX
 #define USE_BCH
#endif

#ifdef USE_BCH
 #include "bch.h"
#endif

#include "jrutils/checkAndCall.h"
#include "jrutils/doParallel.h"


#ifdef LINUX
//on Linux, it is easiest only to include this in one translation unit
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
//#include "numpy_boost.hpp"
#include "numpy_boost_python.hpp"
#endif

#include "utils.h"

//namespace u
static Stroke pathToStroke(numpy_boost<float,2> path)
{
  Stroke s;
  auto sh=path.shape();
  if (sh[1]!=2)
    throw std::runtime_error("A numpy array with two columns to describe a 2d path was expected");
  s.reserve(sh[0]);
  for(int i=0; i<sh[0];++i)
    {
      Point2d p = {{path[i][0],path[i][1]}};
      s.emplace_back(p);
    }
  return s;
}

static Stroke3d pathToStroke3d(numpy_boost<float,2> path)
{
  Stroke3d s;
  auto sh=path.shape();
  if (sh[1]!=3)
    throw std::runtime_error("A numpy array with three columns to describe a 3d path was expected");
  s.reserve(sh[0]);
  for(int i=0; i<sh[0];++i)
    {
      Point3d p = {{path[i][0],path[i][1],path[i][2]}};
      s.emplace_back(p);
    }
  return s;
}

template<typename T>
static numpy_boost<float,1> toNumpy(const T& v)
{
  size_t dims[] = {v.size()};
  numpy_boost<float,1> o (dims);
  int i=0;
  for(float d :v)
    o[i++] = d;
  return o; 
}

template<int M>
struct Siginplace{
  static void go(const Stroke& in, vector<float>& out)
  {
    auto sig = calcPathSig<M>(in);
    out.assign(sig.totalLength,0);
    int i=0;
    sig.iterateOver([&](float d){out[i++]=d;});
  }
};

template<int M>
struct SiginplaceWithoutFirstOfEachLevel{
  static void go(const Stroke& in, vector<float>& out)
  {
    auto sig = calcPathSig<M>(in);
    out.assign(sig.totalLength-M,0);
    int i=0,inlevel=0;
    sig.iterateOverWithSignal([&](float d){if(inlevel++>0)out[i++]=d;},[&]{inlevel=0;});
  }
};

template<int M>
struct SiginplaceRescaled{
  static void go(const Stroke& in, vector<float>& out)
  {
    auto sig = calcPathSig<M>(in);
    out.assign(sig.totalLength,0);
    int i=0,level=1;
    sig.iterateOverWithSignal([&](float d){out[i++]=std::copysign(std::pow(std::abs(d),1.0/level),d);},[&]{++level;});
  }
};

template<int M, int m>
struct ForgetfulSiginplace{
  static void go(const Stroke& in, vector<float>& out)
  {
    auto sig = calcPathSig<M>(in);
    forgetLevels<2,M,m>(sig);
    out.assign(sig.totalLength,0);
    int i=0;
    sig.iterateOver([&](float d){out[i++]=d;});
  }
};

//This function just calculates the signature of a path of any level up to any dimension
//without the dimension or level being in mind at compile time, and consequently without
//trying to store intermediate data on the stack.
numpy_boost<float,1> sig_notemplates_(numpy_boost<float,2> path, int level)
{
  auto sh = path.shape();
  int lengthOfPath = sh[0];
  const int d = sh[1];
  CalculatedSignature s1, s2;
  vector<float> displacement(d);
  for(int i=1; i<lengthOfPath; ++i){
    for(int j=0;j<d; ++j)
      displacement[j]=path[i][j]-path[i-1][j];
    s1.sigOfSegment(d,level,&displacement[0]);
    if(i==1)
      s2.swap(s1);
    else
      s2.concatenateWith(d,level,s1);
  }
  
  size_t dims[] = {(size_t) calcSigTotalLength(d,level)};
  numpy_boost<float,1> o (dims);
  s2.writeOut(o.data());
  
  return o;
}


template<int D, int M>
void sig_general_worker(numpy_boost<float,2> path, int lengthOfPath, vector<float>& out)
{
	Signature<D,M> o;
	for(size_t i=1; i<lengthOfPath; ++i)
	{
		std::array<float,D> displacement;
		for(int j=0; j<D; ++j)
		  displacement[j]=path[i][j]-path[i-1][j];
		o=Signature<D,M>(o,Signature<D,M>(displacement));
	}
	out.assign(o.totalLength,0);
	int i=0;
	o.iterateOver([&](float d){out[i++]=d;});
}

//This calculates the signature of the input for certain 
numpy_boost<float,1> sig_general_(numpy_boost<float,2> path, int level){
  auto sh = path.shape();
  int lengthOfPath = sh[0];
  const int d = sh[1];
  vector<float> out;
  /*
  if(d==5 && level == 5)
    sig_general_worker<5,5>(path,lengthOfPath,out);
  else if(d==10 && level == 4)
    sig_general_worker<10,4>(path,lengthOfPath,out);
  else if(d==3 && level == 10)
    sig_general_worker<3,10>(path,lengthOfPath,out);
  else
  */
    throw std::runtime_error("unexpected constants in sig_general");
  return toNumpy(out);
}

//a function like this could be mostly parallelised by chunking the paths
//but it's more efficient to run this function in parallel
numpy_boost<float,1> sig_(numpy_boost<float,2> path, int level)
{
  vector<float> out;
  const auto stroke = pathToStroke(path);
  checkAndCall<10,Siginplace>::go(level,"signature",std::cref(stroke),std::ref(out));
  return toNumpy(out);
}


//parallel the above
boost::python::list sigs_(boost::python::list paths, int level)
{
  vector<Stroke> strokes;
  const int nToDo = len(paths);
  strokes.reserve(nToDo);
  for(int i=0; i<nToDo; ++i)
    strokes.push_back(pathToStroke(boost::python::extract<numpy_boost<float,2>>(paths[i])));
  vector<vector<float>> answers(nToDo);

  auto fnToDo = [&](int n){
    checkAndCall<8,Siginplace>::go(level,"signature",std::cref(strokes[n]),std::ref(answers[n]));
  };

  doParallel(fnToDo, nToDo);
  boost::python::list o;
  for(const auto& a : answers)
    o.append(toNumpy(a));
  return o;
}

//as sigs, without the first value of each level
boost::python::list sigsWithoutFirsts_(boost::python::list paths, int level)
{
  vector<Stroke> strokes;
  const int nToDo = len(paths);
  strokes.reserve(nToDo);
  for(int i=0; i<nToDo; ++i)
    strokes.push_back(pathToStroke(boost::python::extract<numpy_boost<float,2>>(paths[i])));
  vector<vector<float>> answers(nToDo);

  auto fnToDo = [&](int n){
    checkAndCall<8,SiginplaceWithoutFirstOfEachLevel>::go(level,"signature",std::cref(strokes[n]),std::ref(answers[n]));
  };

  doParallel(fnToDo, nToDo);
  boost::python::list o;
  for(const auto& a : answers)
    o.append(toNumpy(a));
  return o;
}

//as sigs, with level2 sqrted, level 3 cuberooted etc
boost::python::list sigsRescaled_(boost::python::list paths, int level)
{
  vector<Stroke> strokes;
  const int nToDo = len(paths);
  strokes.reserve(nToDo);
  for(int i=0; i<nToDo; ++i)
    strokes.push_back(pathToStroke(boost::python::extract<numpy_boost<float,2>>(paths[i])));
  vector<vector<float>> answers(nToDo);

  auto fnToDo = [&](int n){
    checkAndCall<8,SiginplaceRescaled>::go(level,"signature",std::cref(strokes[n]),std::ref(answers[n]));
  };

  doParallel(fnToDo, nToDo);
  boost::python::list o;
  for(const auto& a : answers)
    o.append(toNumpy(a));
  return o;
}

//like sig_ and sigs_ but omit info in the signature beyond infolevel
numpy_boost<float,1> sigupto_(numpy_boost<float,2> path, int level, int infolevel)
{
  vector<float> out;
  const auto stroke = pathToStroke(path);
  checkAndCall2<8,8,ForgetfulSiginplace>::go(level,infolevel,"signature with forgetfulness",std::cref(stroke),std::ref(out));
  return toNumpy(out);
}


boost::python::list sigsupto_(boost::python::list paths, int level, int infolevel)
{
  vector<Stroke> strokes;
  const int nToDo = len(paths);
  strokes.reserve(nToDo);
  for(int i=0; i<nToDo; ++i)
    strokes.push_back(pathToStroke(boost::python::extract<numpy_boost<float,2>>(paths[i])));
  vector<vector<float>> answers(nToDo);

  auto fnToDo = [&](int n){
    checkAndCall2<8,8,ForgetfulSiginplace>::go(level, infolevel,"signature with forgetfulness",std::cref(strokes[n]),std::ref(answers[n]));
  };

  doParallel(fnToDo, nToDo);
  boost::python::list o;
  for(const auto& a : answers)
    o.append(toNumpy(a));
  return o;
}

vector<float> calcLogSigBCHf(const Stroke& in,int level, bool rescale);

//return level 5 logsignature
numpy_boost<float,1> logsig_(numpy_boost<float,2> path, int level)
{
  auto sh = path.shape();
  //cout<<sh[0]<<","<<sh[1]<<endl;
  auto sig = calcLogSigBCHf(pathToStroke(path),level,false);
  return toNumpy(sig);
}

#ifdef USE_BCH
template<int D, int M>
void logsig_general_worker(numpy_boost<float,2>& in, int lengthOfPath, vector<float>& out)
{
        FLogSignature<D,M> o{};
	for(size_t i=1; i!=in.size(); ++i)
	{
	        FSegment<D> displacement;
		for(int j = 0; j<D; ++j)
		  displacement[j]=in[i][j]-in[i-1][j];
		joinSegmentToSignatureInPlace<D,M>(o,displacement);
	}
	out.assign(o.begin(),o.end());
}
#endif

numpy_boost<float,1> logsig_general_(numpy_boost<float,2> path, int level){
  auto sh = path.shape();
  int lengthOfPath = sh[0];
  const int d = sh[1];
  vector<float> out;
  /*
  if(d==5 && level == 5)
    logsig_general_worker<5,5>(path,lengthOfPath,out);
  else if(d==10 && level == 4)
    logsig_general_worker<10,4>(path,lengthOfPath,out);
  else if(d==3 && level == 10)
    //logsig_general_worker<3,10>(path,lengthOfPath,out);
    std::cout<<"rashdglkj"<<std::endl;
  else
  */
    throw std::runtime_error("unexpected constants in logsig_general");
  return toNumpy(out);
}

template<int D, int M>
void logsigfromsig_general_worker(numpy_boost<float,2> path, int lengthOfPath, vector<float>& out)
{
	Signature<D,M> o;
	for(size_t i=1; i<lengthOfPath; ++i)
	{
		std::array<float,D> displacement;
		for(int j=0; j<D; ++j)
		  displacement[j]=path[i][j]-path[i-1][j];
		o=Signature<D,M>(o,Signature<D,M>(displacement));
	}
	out = getLogSigFromSignature(o);
}

numpy_boost<float,1> logsigfromsig_(numpy_boost<float,2> path, int level){
  auto sh = path.shape();
  int lengthOfPath = sh[0];
  const int d = sh[1];
  vector<float> out;
  /*
  if(d==5 && level == 5)
    logsigfromsig_general_worker<5,5>(path,lengthOfPath,out);
  else if(d==10 && level == 4)
    logsigfromsig_general_worker<10,4>(path,lengthOfPath,out);
  else if(d==3 && level == 10)
    logsigfromsig_general_worker<3,10>(path,lengthOfPath,out);
  else if(d==2 && level == 10)
    logsigfromsig_general_worker<2,10>(path,lengthOfPath,out);
  else if(d==2 && level == 6)
    logsigfromsig_general_worker<2,6>(path,lengthOfPath,out);
  else if(d==2 && level == 2)
    logsigfromsig_general_worker<2,2>(path,lengthOfPath,out);
  else
  */
    throw std::runtime_error("unexpected constants in logsigfromsig");
  return toNumpy(out);
}


//parallel the above
boost::python::list logsigs_(boost::python::list paths, int level)
{
  vector<Stroke> strokes;
  const int nToDo = len(paths);
  strokes.reserve(nToDo);
  for(int i=0; i<nToDo; ++i)
    strokes.push_back(pathToStroke(boost::python::extract<numpy_boost<float,2>>(paths[i])));
  vector<vector<float>> answers(nToDo);

  auto fnToDo = [&](int n){
    answers[n] = calcLogSigBCHf(strokes[n],level,false);
  };

  doParallel(fnToDo, nToDo);

  boost::python::list o;
  for(const auto& a : answers)
    o.append(toNumpy(a));
  return o;
}

//with level2 sqrted, level 3 cuberooted etc
boost::python::list logsigsRescaled_(boost::python::list paths, int level)
{
  vector<Stroke> strokes;
  const int nToDo = len(paths);
  strokes.reserve(nToDo);
  for(int i=0; i<nToDo; ++i)
    strokes.push_back(pathToStroke(boost::python::extract<numpy_boost<float,2>>(paths[i])));
  vector<vector<float>> answers(nToDo);

  auto fnToDo = [&](int n){
    answers[n] = calcLogSigBCHf(strokes[n],level,true);
  };

  doParallel(fnToDo, nToDo);

  boost::python::list o;
  for(const auto& a : answers)
    o.append(toNumpy(a));
  return o;
}
