// I've disabled LTCG because of all the template stuff

#include "stdafx.h"
#include "SigUtilities.h"
#include "Representations.h"
#include "chinese.h"

#ifdef LINUX
//on Linux, it is easiest only to include this in one translation unit
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy_boost.hpp"
#include "numpy_boost_python.hpp"
#endif

#include "handwritingData.h"


#include "utils.h"

//EASY means the representation is all zeros
//#define EASY 

void bch();

//template<int D, 1>

void sigVec(float x, float y)
{
	Point2d a;
	a[0]=x;
	a[1]=y;
	Signature<2,4> fsoo(a);
	fsoo.print();
	for(auto a : getLogSigFromSignature(fsoo))
		cout<<a<<",";
	cout<<endl;
	for(auto a : getLogSigFromSignature(fsoo.m_parent))
		cout<<a<<",";
	cout<<endl;
	for(auto a : getLogSigFromSignature(fsoo.m_parent.m_parent))
		cout<<a<<",";
	//auto aa=unpermute(fsoo);
	//for(auto a:aa)
	//	cout<<a<<",";
	cout<<endl;
}

void sigVec2(float x, float y, float x1, float y1)
{
	Point2d a, b;
	a[0]=x;
	a[1]=y;
	b[0]=x1;b[1]=y1;
	static const int m=5;
	Signature<2,m> s1(a),s2(b);
	Signature<2,m> fsoo(s1,s2);
	fsoo.print();
	for(auto a : getLogSigFromSignature(fsoo))
		cout<<a<<",";
	cout<<endl;
	for(auto a : getLogSigFromSignature(fsoo.m_parent))
		cout<<a<<",";
	cout<<endl;
	for(auto a : getLogSigFromSignature(fsoo.m_parent.m_parent))
		cout<<a<<",";
	cout<<endl;
	for(auto a : getLogSigFromSignature(fsoo.m_parent.m_parent.m_parent))
		cout<<a<<",";
	//for(auto a:unpermute(fsoo))
	//	cout<<a<<",";
	//cout<<endl;
	//for(auto a:unpermute(fsoo.m_parent))
	//	cout<<a<<",";
	//cout<<endl;
	//for(auto a:unpermute(fsoo.m_parent.m_parent))
	//	cout<<a<<",";
	cout<<endl;
}
void sigVec2L(float x, float y, float x1, float y1)
{
	Point2d o, a, b;
	Stroke str;
	o[0]=o[1]=0;
	a[0]=x;
	a[1]=y;
	b[0]=x1+x;b[1]=y1+y;
	str.push_back(o);str.push_back(a);str.push_back(b);
	auto aa = getLogSig<2,2>(str);
	for(auto f:aa)
		cout<<f<<',';
	cout<<endl;
	auto aa1 = getLogSig<2,3>(str);
	for(auto f:aa1)
		cout<<f<<',';
	cout<<endl;
	auto aa2 = getLogSig<2,4>(str);
	for(auto f:aa2)
		cout<<f<<',';
	cout<<endl;

}

void sigVec1(float x)
{
	std::array<float,1> a;
	a[0]=x;
	Signature<1,4> fsoo(a);
	fsoo.print();
}

void sigVec21(float x, float x1)
{
	std::array<float,1> a, b;
	a[0]=x;
	b[0]=x1;
	static const int m=4;
	Signature<1,m> s1(a),s2(b);
	Signature<1,m> fsoo(s1,s2);
	fsoo.print();
}

void lengths()
{
	cout<<getSigSize<2,2>()<<','<<getLogSigSize<2,2>()<<endl;
	cout<<getSigSize<2,3>()<<','<<getLogSigSize<2,3>()<<endl;
	cout<<getSigSize<2,4>()<<','<<getLogSigSize<2,4>()<<endl;
	cout<<getSigSize<2,5>()<<','<<getLogSigSize<2,5>()<<endl;
	cout<<getSigSize<2,6>()<<','<<getLogSigSize<2,6>()<<endl;
	cout<<getSigSize<2,7>()<<','<<getLogSigSize<2,7>()<<endl;
	cout<<getSigSize<2,8>()<<','<<getLogSigSize<2,8>()<<endl;
	cout<<getSigSize<2,9>()<<','<<getLogSigSize<2,9>()<<endl;
	cout<<'\n'<<getSigSize<3,2>()<<','<<getLogSigSize<3,2>()<<endl;
	cout<<getSigSize<3,3>()<<','<<getLogSigSize<3,3>()<<endl;
	cout<<getSigSize<3,4>()<<','<<getLogSigSize<3,4>()<<endl;
	cout<<getSigSize<3,5>()<<','<<getLogSigSize<3,5>()<<endl;// too slow!

	//auto& a = alg::free_tensor<float,float,2,5>::basis;
	auto& a = alg::lie<float,float,2,5>::basis;
	a.growup(5);
	for(auto i = a.begin(); i!=a.end(); i=a.nextkey(i)) //note: cant use < here
		cout<<a.key2string(i)<<endl;
	cout<<endl<<endl;
}


std::string myF(boost::python::object o)
{
	std::cout << "C++" << std::endl;
	if(o.is_none())
		return "bn";
	else
		return "kljgf2";
}

numpy_boost<float, 2> foo1()
{

//	auto conn = JRSqlite::Connection::openReadOnly("..\\data\\Assamese.sqlite");
	auto conn = JRSqlite::Connection::openReadOnly("..\\data\\pendigits.sqlite");
	JRSqlite::Source1BlobStmt st(conn, "select data from strokes where stroken=1");
	Character vf;
	st.executeWithSize([&vf](const char* x, int n)
		{
			vf.push_back(Stroke());
			
			vf.back().resize(n/8);
			const float* xx = reinterpret_cast<const float*>(x);
			for(size_t i=0; i!=vf.size(); ++i)
			{
				vf.back()[i][0]=xx[i*2];
				vf.back()[i][1]=xx[i*2+1];
			}
		}
	);


	size_t dims[] = { vf[0].size(), 2 };
	numpy_boost<float, 2> array(dims);

	for (size_t i = 0; i < array.shape()[0]; ++i) {
		array[i][0] = vf[0][i][0];
		array[i][1] = vf[0][i][1];
	}
	return array;
}


boost::python::list representPaths(const Character& ch){
	boost::python::list o;
	for(const auto& a: ch){
		size_t dims[] = { a.size(), 2 };
		numpy_boost<float, 2> array(dims);
		for (size_t i = 0; i < array.shape()[0]; ++i) {
			array[i][0] = a[i][0];
			array[i][1] = a[i][1];
		}
		o.append(array);
	}
	return o;
}

boost::python::list vectorToPythonList(const vector<float>& a){
	boost::python::list o;
	size_t dims[] = { a.size() };
	numpy_boost<float, 1> array(dims);
	for (size_t i = 0; i < a.size(); ++i) {
		array[i] = a[i];
	}
	o.append(array);
	return o;
}

numpy_boost<float,2> vectorToNumpy2d(const vector<float>& a, size_t d1, size_t d2){
  size_t dims[] = {d1, d2};
  numpy_boost<float, 2> arr(dims);
  float* d = arr.data();
  for(float f: a)
    *(d++) = f;
  return arr;
}

//this function assumes that every elt of a has length d1*d2
numpy_boost<float,3> vectorsToNumpy3d(const vector<vector<float>>& a, size_t d1, size_t d2){
  size_t dims[] = {a.size(),d1, d2};
  numpy_boost<float, 3> arr(dims);
  float* d = arr.data();
  for(const vector<float>& v : a)
    for(float f: v)
      *(d++) = f;
  return arr;
}

//this function assumes that every elt of a is an n*d2 array and returns the max of the n's
numpy_boost<float,3> vectorsToMaxNumpy3d(const vector<vector<float>>& a, size_t d2){
  size_t maxSize = 0;
  for(const auto& v : a)
    if(maxSize<v.size())
      maxSize=v.size();
  size_t d1 = maxSize / d2;
  size_t dims[] = {a.size(),d1, d2};
  numpy_boost<float, 3> arr(dims);
  float* data = arr.data();
  for(const vector<float>& v : a){
    float* firstDest = data+maxSize-1;
    for(float f: v)
      *(firstDest--) = f;
    data += maxSize;
  }
  return arr;
}

//split layers
//returns o such that o[i][j,k,l] is a[j][i][k*d2+l]
boost::python::list vectorsToListOfNumpy3d(const vector<vector<vector<float>>>& a, size_t d1){
  boost::python::list o;
  size_t nLayers = a.at(0).size();
  for(int layer = 0; layer<nLayers; ++layer){
    size_t d2 = a[0][layer].size()/d1;
    size_t dims[] = {a.size(),d1, d2};
    numpy_boost<float, 3> arr(dims);
    float* d = arr.data();
    for(const vector<vector<float>>& v : a)
      for(float f: v[layer])
	*(d++) = f;
    o.append(arr);
  }
  return o;
}

numpy_boost<int,1> vectorToNumpy1d(const vector<int>& a){
  size_t dims[] = {a.size()};
  numpy_boost<int, 1> arr(dims);
  int* d = arr.data();
  for(int f: a)
    *(d++) = f;
  return arr;
}

numpy_boost<float,2> vectorToOneHot(const vector<int>& a, size_t max){ //a has elts between 0 and max-1
  size_t asize = a.size();
  size_t dims[] = {asize, max};
  numpy_boost<float, 2> arr(dims);//This begins uninitialised!!
  float* d = arr.data();
  for(int i=0; i!=max*asize; ++i)
    d[i]=0.0f;

  for(size_t i=0; i!=asize; ++i)
    d[i*max+a[i]]=1;
  return arr;
}

void setTrainingInterest(boost::python::list l)
{
	vector<int> vi;
	for(int i=0, end=len(l);i<end;++i)
	{
		vi.push_back(boost::python::extract<int>(l[i]));
	}
	setInterest(vi,false);
}
void setTestingInterest(boost::python::list l)
{
	vector<int> vi;
	for(int i=0, end=len(l);i<end;++i)
	{
		vi.push_back(boost::python::extract<int>(l[i]));
	}
	setInterest(vi,true);
}


//CharacterDistorter is the worker for
// api fn getDistortedTrainRepresentations which generates 
// N of (a representation of a distorted character)

class CharacterDistorter
{
	std::random_device m_rd;
	std::mt19937 m_rnd;
        const vector<Character>& m_trainX;
        const vector<int>& m_trainY;
public:
	CharacterDistorter():m_rnd(m_rd())
			    , m_trainX(get_trainX())
			    , m_trainY(get_trainY())
	{
	}
//	typedef boost::array<boost::python::object,3> result_type;
	typedef std::tuple<vector<float>,int,int> result_type;
	result_type operator()()
	{
		//result_type out;
		std::uniform_int_distribution<int> uid(1,m_trainX.size());
		int index = uid(m_rnd);
		//out[2]=boost::python::object(index);
		//out[1]=boost::python::object(g_data.m_trainY[index-1]);
		//auto x = g_data.getStrokes(index);
		auto x = m_trainX[index-1];
		distortCharacter(x, m_rnd); //TODO: move this fn here?
		//out[0]=representChar(x);
		//return out;
		return std::make_tuple(representChar(x),m_trainY[index-1],index);
	}
};


//There's a problem with the dtor of AlternatingAsyncs after a ctrl-c - 
//joinable returns true and yet the worker thread no longer exists.
//Easiest solution is to just never attempt dtor
boost::python::object getDistortedTrainRepns(int N){
	//static AlternatingAsyncs<CharacterDistorter,2,10> g_distorter;
	//static AlternatingAsyncs<CharacterDistorter,2,100>& g_distorter =*new AlternatingAsyncs<CharacterDistorter,2,100>();
	static CharacterDistorter g_distorter;

	boost::python::list o, o2,o3;
	//cout<<"doing"<<endl;
	//static bool started = false;
	//if(!started)
	//{
	//	//for(auto& a : g_distorters)
	//	//	a.start();
	//	started=true;
	//}
	for(int i=0; i!=N; ++i)
	{
		auto oo = g_distorter();
		o3.append(std::get<2>(oo));
		o2.append(std::get<1>(oo));
		o.append(vectorToPythonList(std::get<0>(oo)));
	}
	//cout<<"done"<<endl;
	return boost::python::make_tuple(o,o2,o3);
}


boost::python::object getTrainRepns(){
	boost::python::list o;
	for(const auto& x : get_trainX())
		o.append(vectorToPythonList(representChar(x)));
	return o;
}
boost::python::object getTestRepns(int factor){
	boost::python::list o;
	const auto& testX = get_testX();
	for(const auto& x : testX)
		o.append(vectorToPythonList(representChar(x)));

//hack to ensure output is multiple of factor in length
	auto x = representChar(testX[0]);
	for(size_t i=testX.size(); i%factor; ++i)
		o.append(vectorToPythonList(x));
	return o;
}

boost::python::object getRNNinput(){
  RNNInput all = getRNNinput_readyForPython();
  int strokeRepnLength = getStrokeRepnLength();
  return boost::python::make_tuple(
           boost::python::make_tuple(
				     vectorsToNumpy3d(all.trainX,RNN_maxTime,strokeRepnLength),
				     //vectorToNumpy1d(all.trainY)
				     vectorToOneHot(all.trainY,3755)
				     ),
           boost::python::make_tuple(
				     vectorsToNumpy3d(all.testX,RNN_maxTime,strokeRepnLength),
				     //vectorToNumpy1d(all.testY)
				     vectorToOneHot(all.testY,3755)
				     ));  
}
boost::python::object getRNNinput_variableLength(){
  RNNInput all = getRNNinput_variableLength_readyForPython();
  int repnLength = getVariableLengthStrokeRepnLength();
  return boost::python::make_tuple(
           boost::python::make_tuple(
				     vectorsToMaxNumpy3d(all.trainX,repnLength),
				     //vectorToNumpy1d(all.trainY)
				     vectorToOneHot(all.trainY,3755)
				     ),
           boost::python::make_tuple(
				     vectorsToMaxNumpy3d(all.testX,repnLength),
				     //vectorToNumpy1d(all.testY)
				     vectorToOneHot(all.testY,3755)
				     ));  
}

static
boost::python::object getRNNinput_labelled(){ //only works if enabled in chinese.cpp
  RNNInput all = getRNNinput_readyForPython();
  int strokeRepnLength = getStrokeRepnLength();
  return boost::python::make_tuple(
           boost::python::make_tuple(
				     vectorsToNumpy3d(all.trainX,RNN_maxTime,strokeRepnLength),
				     vectorToNumpy1d(all.trainChar),
				     vectorToOneHot(all.trainY,3755)
				     ),
           boost::python::make_tuple(
				     vectorsToNumpy3d(all.testX,RNN_maxTime,strokeRepnLength),
				     vectorToNumpy1d(all.testChar),
				     vectorToOneHot(all.testY,3755)
				     ));  
}

boost::python::object getLayerInput(){
  LayerInput all = getLayerInput_readyForPython();
  return boost::python::make_tuple(
           boost::python::make_tuple(
				     vectorsToListOfNumpy3d(all.trainX,RNN_maxTime),
				     vectorToOneHot(all.trainY,3755)
				     ),
           boost::python::make_tuple(
				     vectorsToListOfNumpy3d(all.testX,RNN_maxTime),
				     vectorToOneHot(all.testY,3755)
				     ));  
}

void about()
{
	cout<< // "JFR's embedded python/c++ module " 
#ifdef _DEBUG
	" DEBUG build"
#else
	" RELEASE build"
#endif
	" " __DATE__ " " __TIME__ <<endl;
}

//#ifndef LINUX
#ifdef VERIFY
template <typename T, int R, int C>
inline void makeRealZeros(Eigen::Matrix<T,R,C>& xs)
{
  for (size_t i = 0, nRows = xs.rows(), nCols = xs.cols(); i < nCols; ++i)
   for (size_t j = 0; j < nRows; ++j)
   {
        if (fabs(xs(j,i))<0.0001)
          xs(j,i)=0;
   }
}
template <typename T, int R, int C>
inline int nZeros(const Eigen::Matrix<T,R,C>& xs)
{
  int out=0;
  for (size_t i = 0, nRows = xs.rows(), nCols = xs.cols(); i < nCols; ++i)
   for (size_t j = 0; j < nRows; ++j)
   {
        if (fabs(xs(j,i))<0.0001)
          ++out;
   }
   return out;
}
#endif



void deriveIrrotation(){
	std::random_device rndd;
	std::mt19937 rnd(rndd());
	std::uniform_real_distribution<float> urd(-1,1);
	const int repeats = 60;
	const int tries=2311;
	const int M = 4;
	const int dim = Signature<3,M>::length;
	vector<vector<float>> f(tries*repeats);
	for(int repeat=0, row=0; repeat<repeats; ++repeat)
	{
		Stroke3d d;
		for(int i=0; i!=12; ++i){
			Point3d b;
			b[0]=urd(rnd);
			b[1]=urd(rnd);
			b[2]=urd(rnd);
			d.push_back(b);
		}
		auto s1=calcPathSig<M>(d);
		vector<float> base;
		for(int j=0; j!= dim; ++j){
			base.push_back(s1.m_data[j]);
		}
		for(int i=0; i<tries; ++i, ++row)
		{
			float angle = urd(rnd), c=cos(angle), s=sin(angle);
			for(auto &b: d){
				auto x= c*b[0]+s*b[1];
				auto y=-s*b[0]+c*b[1];
				b[0]=x; b[1]=y;
			}
			auto s2=calcPathSig<M>(d);
			f[row].assign(dim,0);
			for(int j=0; j!= dim; ++j)
				f[row][j]=s2.m_data[j]-base[j];
		}
	}
	f.push_back(vector<float>()); //we know 333 is invariant!
	f.back().assign(dim,0);
	f.back().back()=1;
#ifdef VERIFY
	using namespace Eigen;
	MatrixXd m(f.size(),dim);
	for(int i=0; i<(int)f.size(); ++i)
		for(size_t j=0; j!= dim; ++j)
			m(i,j) = f[i][j];
//	FullPivLU<MatrixXd> lu0(m);
//cout << "Here is, up to permutations, its LU decomposition matrix:"
//<< endl << lu0.matrixLU() << endl;
//cout << "Here is the permutation:" << endl;
//cout <<lu0.permutationQ().toDenseMatrix()<< endl;
//	FullPivLU<MatrixXd> lu1(m.transpose());
//cout << "Here is, up to permutations, its LU decomposition matrix:"
//<< endl << lu1.matrixLU().transpose() << endl;
//cout << "Here is the permutation:" << endl;
//cout <<lu1.permutationP().toDenseMatrix()<< endl;


	JacobiSVD<MatrixXd> svd(m,ComputeThinU | ComputeThinV);
	cout << "Its singular values are:" << endl << svd.singularValues() 
	<<"\n of which "<<nZeros(svd.singularValues())<<" are 0"<<endl;
    //cout << "Its left singular vectors are the columns of the thin U matrix:" << endl << svd.matrixU() << endl;
	//cout << "Its right singular vectors are the columns of the thin V matrix:" << endl << svd.matrixV() << endl;
	//the last 3 columns (4/9 sing vals are nonzero) - for M=2
	//For M=2, 7/27 sing vals are nonzero#
//	int relevant = 4;
//	auto m2 = svd.matrixV().block(0,dim-relevant,dim,relevant).transpose();
//	FullPivLU<MatrixXd> lu(m2);
////	
//	auto mm = lu.matrixLU().triangularView<Upper>().toDenseMatrix();
//	makeRealZeros(mm);
//cout << "Here is, up to permutations, its LU decomposition matrix:"
//<< endl << mm << endl;
//cout << "Here is the permutation:" << endl;
//cout << lu.permutationP().toDenseMatrix()<<endl<<endl<<lu.permutationQ().toDenseMatrix()<< endl;
//cout << "Here is its LU decomposition matrix:"
//	<< endl << mm * lu.permutationQ() << endl;
//FullPivLU<MatrixXd> lu2(m2.transpose());
//	
//cout << "Here is, up to permutations, its LU decomposition matrix:"
//<< endl << lu2.matrixLU() << endl;
//cout << "Here is the permutation:" << endl;
//cout << lu2.permutationP().toDenseMatrix()<<endl<<endl<<lu2.permutationQ().toDenseMatrix()<< endl;
//ColPivHouseholderQR<MatrixXd> q(m2);
//cout << "Here is, its R"
//	<< endl << q.matrixR() << endl;
#endif
}


void verifyIrrotation_4(){
	std::random_device rndd;
	std::mt19937 rnd(rndd());
	std::uniform_real_distribution<float> urd(-1,1);
	Stroke3d d;
	for(int i=0; i!=12; ++i){
		Point3d b;
		b[0]=urd(rnd);
		b[1]=urd(rnd);
		b[2]=urd(rnd);
		d.push_back(b);
	}
	vector<float> f1, f2;
	auto s1=calcPathSig<3>(d);
	try3DInvariants(s1, f1);
	float angle = urd(rnd), c=cos(angle), s=sin(angle);
	for(auto &b: d){
		auto x= c*b[0]+s*b[1];
		auto y=-s*b[0]+c*b[1];
		b[0]=x; b[1]=y;
	}
	auto s2=calcPathSig<3>(d);
	try3DInvariants(s2, f2);
	if(f1.size()!=f2.size()){
		cout<<"different sizes"<<endl;
		return;
	}
	for(size_t i=0; i!=f1.size(); ++i)
		cout<<std::setw(10)<<f1[i]-f2[i]<<",  "<<f1[i]<<",  "<<f2[i]<<endl;
}

//Signature of a sin wave - differs from its negative at level 3, and in irrot at level 2*2
//void vsin(){
//	Stroke d;
//	const int parts = 200;
//	const int n = 1;
//	d.reserve(parts+1);
//	for(int i=0; i<=parts; ++i)
//	{
//		boost::array<float, 2> b;
//		float x = i * (1.0f/parts); b[0]=x;
//		b[1] = sin(x*2*M_PI*n);
//		d.push_back(b);
//	}
//	auto s1=calcPathSig<4>(d);
//	s1.print();
//	vector<float> f;
//	getRawRotationInvariants(s1,f);
//	for(auto a:f)
//		cout<<a<<",";
//	cout<<endl;
//}

static void tri(int i)
{
	static const int dig =5;
	TernaryNumber<dig> t;
	while((i--)>0)
		t.increment();
	for(auto a : t.m_data)
		cout<<(int)a;
	cout<<' ';
}

//void testOctogram();
void foo()
{
	Stroke s;
	Point2d b;
	float pt = 2;
	b[0]=b[1]=pt;
	s.push_back(b);
	b[1]=-pt;
	s.push_back(b);
	b[0]=-pt;
	s.push_back(b);
	b[1]=pt;
	s.push_back(b);
	b[0]=pt;
	s.push_back(b);

	auto ss = calcPathSig<2>(s);
	ss.print();
	auto lo = getLogSigFromSignature(ss);
	for(auto a:lo)
		cout<<a<<endl;
	//testOctogram();
	//IterateInitial3sOnly<5>::indices([&](int i){tri(i); cout<<i<<endl; });
	//cout<<endl;
	//IterateInitial3sOnly<5>::indicesFinalThrees([&](int i){tri(i); cout<<i<<endl;});
}

void verifyIrrotation(){
	std::random_device rndd;
	std::mt19937 rnd(rndd());
	std::uniform_real_distribution<float> urd(-1,1);
	Stroke d;
	for(int i=0; i!=3; ++i){
		Point2d b;
		b[0]=urd(rnd);
		b[1]=urd(rnd);
		d.push_back(b);
	}
	vector<float> f1, f2;
	//auto s1=calcPathSigEndingXAxis<5>(d);
	auto s1=calcPathSig<8>(d);
	f1=getLogSigFromSignature(s1);
	float angle = urd(rnd), c=cos(angle), s=sin(angle);
	for(auto &b: d){
		auto x= c*b[0]+s*b[1];
		auto y=-s*b[0]+c*b[1];
		b[0]=x; b[1]=y;
	}
	auto s2=calcPathSig<8>(d);
	f2=getLogSigFromSignature(s2);
	if(f1.size()!=f2.size()){
		cout<<"different sizes"<<endl;
		return;
	}
	for(size_t i=0; i!=f1.size(); ++i)
		cout<<i<<" "<<std::setw(10)<<f1[i]-f2[i]<<",  "<<f1[i]<<",  "<<f2[i]<<endl;
}

void verifyIrrotation0(){
	std::mt19937 rnd;
	std::uniform_real_distribution<float> urd(-1,1);
	Stroke d;
	for(int i=0; i!=12; ++i){
		Point2d b;
		b[0]=urd(rnd);
		b[1]=urd(rnd);
		d.push_back(b);
	}
	vector<float> f1, f2;
	auto s1=calcPathSig<8>(d);
	getRotationInvariants(s1, f1);
	getRawRotationInvariants(s1, f1);
	float angle = urd(rnd), c=cos(angle), s=sin(angle);
	for(auto &b: d){
		auto x= c*b[0]+s*b[1];
		auto y=-s*b[0]+c*b[1];
		b[0]=x; b[1]=y;
	}
	auto s2=calcPathSig<8>(d);
	getRotationInvariants(s2, f2);
	getRawRotationInvariants(s2, f2);
	if(f1.size()!=f2.size()){
		cout<<"different sizes"<<endl;
		return;
	}
	for(size_t i=0; i!=f1.size(); ++i)
		cout<<std::setw(10)<<f1[i]-f2[i]<<",  "<<f1[i]<<",  "<<f2[i]<<endl;
}

void verifyIrrotation1(){
	static std::mt19937 rnd;
	std::uniform_real_distribution<float> urd(-1,1);
	Stroke d;
	for(int i=0; i!=12; ++i){
		Point2d b;
		b[0]=urd(rnd);
		b[1]=urd(rnd);
		d.push_back(b);
	}
	vector<float> f1, f2;
	auto s1=calcPathSig<4>(d);
	auto f = [](const Signature<2,4>&s, vector<float>&f){//These are the concat products of known level 2 invariants
														//They are invariant. They are in our subspace.
		const auto& d = s.m_data;
		f.push_back(d[0]+d[15]+d[3]+d[12]);
		f.push_back(d[5]+d[10]-d[9]-d[6]);
		f.push_back(d[1]+d[13]-d[2]-d[14]);
		f.push_back(d[4]+d[7]-d[8]-d[11]);
	};
	f(s1,f1);
	float angle = urd(rnd), c=cos(angle), s=sin(angle);
	for(auto &b: d){
		auto x= c*b[0]+s*b[1];
		auto y=-s*b[0]+c*b[1];
		b[0]=x; b[1]=y;
	}
	auto s2=calcPathSig<4>(d);
	f(s2,f2);
	if(f1.size()!=f2.size()){
		cout<<"different sizes"<<endl;
		return;
	}
	for(size_t i=0; i!=f1.size(); ++i)
		cout<<std::setw(10)<<f1[i]-f2[i]<<",  "<<f1[i]<<",  "<<f2[i]<<endl;
}


#ifdef _DEBUG
BOOST_PYTHON_MODULE(embedd)
#else
BOOST_PYTHON_MODULE(embed)
#endif
{
    using namespace boost::python;
	import_array();
	numpy_boost_python_register_type<int, 1>();
	numpy_boost_python_register_type<int, 2>();
	numpy_boost_python_register_type<double, 2>();
	numpy_boost_python_register_type<float, 1>();
	numpy_boost_python_register_type<float, 2>();
	numpy_boost_python_register_type<float, 3>();
	//boost::python::numeric::array::set_module_and_type("numpy", "ndarray");
	def("f", myF);
	def("foo",foo);
	def("getDistortedTrainRepns",getDistortedTrainRepns);
	def("getRepnLength",getRepnLength);
	def("getRepnName",getRepnName);
	def("getRepnNumber",getRepnNumber);
	def("getTestRepns",getTestRepns);
	def("getTrainRepns",getTrainRepns);
	def("setTrainingInterest",setTrainingInterest,"use only these indices of the training data");
	def("setTestingInterest",setTestingInterest,"use only these indices of the testing data");
	def("setChoice", setChoice,"If using UseMulti, use this representation");
	def("openData",openData,"set sqlite filename of training and test data");
	def("openDataAsync",setDataCopyAsync);
	def("waitForOpenData",waitForOpenDataCopy);
	def("sigVec",sigVec);
	def("sigVec2",sigVec2);
	def("sigVec2L",sigVec2L);
	def("sigVec1",sigVec1);
	def("sigVec21",sigVec21);
	def("deriveIrrotation",deriveIrrotation);
	def("verifyIrrotation",verifyIrrotation);
	def("lengths",lengths);
	def("about",about);
	def("bch",bch);

	//from chinese.h/chinese.cpp for RNNs
	def("verifyBB",verifyBoundingBoxes);
	def("getRNNinput",getRNNinput);
	//def("getRNNinput2",getRNNinput_labelled);
	def("getStrokeRepnLength",getStrokeRepnLength);
	def("getStrokeRepnName",getStrokeRepnName);

	//and for looser RNNs "variableLength"
	def("getRNNinput_variableLength",getRNNinput_variableLength);
	def("getVariableLengthStrokeRepnLength",getVariableLengthStrokeRepnLength);
	def("getVariableLengthRepnName",getVariableLengthRepnName);

	
	//and for pyramid structures
	def("getLayerInput",getLayerInput);

	//from utils.h/utils.cpp
	
	def("sig",sig_);
	def("sig_notemplates",sig_notemplates_);
	def("sig_general",sig_general_);
	def("sigs",sigs_);
	def("sigsRescaled",sigsRescaled_);
	def("sigsWithoutFirsts",sigsWithoutFirsts_);
	def("sigupto",sigupto_);
	def("sigsupto",sigsupto_);
	def("logsig",logsig_);
	def("logsig_general",logsig_general_);
	def("logsigfromsig",logsigfromsig_);
	def("logsigs",logsigs_);
	def("logsigsRescaled",logsigsRescaled_);
	
}

