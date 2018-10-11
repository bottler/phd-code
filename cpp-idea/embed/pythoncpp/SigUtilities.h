#ifndef JR_SIGUTILITIES_H
#define JR_SIGUTILITIES_H

typedef std::array<float, 3> Point3d; //beware - need to explicitly construct if you want zero initialization
typedef vector<Point3d> Stroke3d;
typedef std::array<float,2> Point2d;
typedef vector<std::array<float, 2>> Stroke;
typedef vector<Stroke> Character;

#ifndef LINUX
__declspec(noreturn) 
#else
static void reportError(const char* f) __attribute__ ((noreturn));
//[[noreturn]] - needs GCC 4.8
#endif
static void reportError(const char* f)
{
	std::cout<<"Problem in C++ code "<<f<<std::endl;
	throw std::runtime_error(f);
}

template<int D, int M>
int getSigSize()
{
	const int w=D, d=M;
	static_assert(w>1,"w>1");
	int f = (alg::ConstPower<w,d+1>::ans-1)/(w-1)-1;
	return f-1;
}

template<int D, int M>
int getLogSigSize()
{
	typedef alg::lie<float,float,D,M> L;
	L::basis.growup(M);
	//return 3;
	return L::basis.size();
}

template<int D, int M>
vector<float> getLogSigFromSignature(const Signature<D,M>& sig)
{
	alg::free_tensor<float,float,D,M> tens; //use own allocator in the map?
	auto& a = tens.basis;
	auto i = a.begin();
	tens[i]=1;
	sig.iterateOver([&](float f){i=a.nextkey(i); tens[i]=f;});

	auto lo = log(tens);
	alg::maps<float,float,D,M> m;
	auto& b = alg::lie<float,float,D,M>::basis;
	b.growup(M);
	auto lo2 = m.t2l(lo);

	vector<float> out(b.size());

	int pos = 0;
	for(auto i=b.begin(); i!=b.end(); i=b.nextkey(i), ++pos)
	  //		if(lo2.count(i))
	  //		out[pos]=lo2[i];
	  {
	    auto it = lo2.find(i);
	    if(it!=lo2.end())
	      out[pos]=it->second;
	  }
	return out;
}

//take a Signature<D,M> and forget any information in it above level m
template<int D, int M, int reducedM>
void forgetLevels(Signature<D,M>& sig)
{
  //  static_assert(m<M
	alg::free_tensor<float,float,D,M> tens; //use own allocator in the map?
	auto& a = tens.basis;
	auto i = a.begin();
	tens[i]=1;
	sig.iterateOver([&](float f){i=a.nextkey(i); tens[i]=f;});

	auto lo = log(tens);
	alg::maps<float,float,2,M> m;
	auto& b = alg::lie<float,float,2,M>::basis;
	b.growup(M);
	auto lo2 = m.t2l(lo);
	for(auto i=b.begin(); i!=b.end(); i=b.nextkey(i))
	  if(b.degree(i)>reducedM)
		  lo2[i]=0;
	auto lo3 = m.l2t(lo2);
	auto tens2 = exp(lo3);
	i=a.begin();
	sig.iterateOver_mutably([&](float& f){i=a.nextkey(i); f=tens2[i];});
}

//in.length is 2^M
template<int M>
vector<SignatureNumeric> unpermute(const Signature<2,M>& in)
{
	vector<SignatureNumeric> o(M+1);
	static_assert(Signature<2,M>::length<32768,"what?");
	for(size_t i=0; i!=in.length; ++i)
		o[std::bitset<M> (i).count()]+=in.m_data[i];
	return o;
}

template<int digits>
struct TernaryNumber
{
	int m_nOnes, m_nTwos;
	std::array<unsigned char,digits> m_data;
	TernaryNumber():m_data(){}
	void increment(){
		int i=0;
		while(i<digits){
			if(m_data[i]<2)
			{
				++m_data[i];
				if(m_data[i]==2){
					++m_nTwos;--m_nOnes;
				}
				else
					++m_nOnes;
				for(int j=0;j<i;++j){
					--m_nTwos;
					m_data[j]=0;
				}
				return;
			}
			++i;
		}
		reportError("overran ternaryNumber");
	}
	int toInt() const
	{
		int out = 0;
		for(int i=0, placeValue=1; i<digits; ++i, placeValue*=3)
			out += m_data[i]*placeValue;
		return out;
	}
};

template<int M>
vector<SignatureNumeric> unpermute(const Signature<3,M>& in)
{
	vector<SignatureNumeric> o((M*(M-1))/2);
	TernaryNumber<M> tn;
	for(size_t i=0; i!=in.length; ++in)
	{
		int numPrevRows=(tn.m_nOnes+tn.m_nTwos);
		int numInPrevRows=(numPrevRows*(numPrevRows+1))/2;
		o[numInPrevRows+tn.m_nTwos]+=in.m_data[i];
		tn.increment();
	}
	return o;
}

template<int D, int M> 
vector<float> getLogSig(const vector<std::array<float, D>>& path)
{
	typedef alg::lie<float,float,D,M> Lie;
	typedef alg::cbh<float,float,D,M> Cbh;
	vector<Lie> increments;
	increments.reserve(path.size());
	for(size_t i=1; i<path.size(); ++i)
	{
		Lie inc;
		for(int d=0; d<D; ++d)
			inc += Lie(d+1,path[i][d]-path[i-1][d]);
		increments.push_back(inc);
	}
	Cbh cbh;
	Lie o = cbh.full(increments.cbegin(), increments.cend());
	vector<float> ans(getLogSigSize<D,M>());
	for(auto a = o.begin();a!=o.end();++a)
	{
		ans[a->first-1]=a->second;
	}
	return ans;
}

template<int M>
Signature<2,M> calcPathSig(const Stroke& in)
{
	Signature<2,M> o;
	for(size_t i=1; i<in.size(); ++i)
	{
		Point2d displacement;
		displacement[0]=in[i][0]-in[i-1][0];
		displacement[1]=in[i][1]-in[i-1][1];
		o=Signature<2,M>(o,Signature<2,M>(displacement));
	}
	return o;
}

template<int M>
Signature<2,M> calcPathSig1(const Stroke& in)
{
	Signature<2,M> o;
	for(size_t i=1; i!=in.size(); ++i)
	{
		Point2d displacement;
		displacement[0]=in[i][0]-in[i-1][0];
		displacement[1]=in[i][1]-in[i-1][1];
		o.postConcatenateWith(Signature<2,M>(displacement));
	}
	return o;
}

//template<int M>
//unique_ptr<Signature<2,M>> calcPathSigHeap(const Stroke& in)
//{
//	unique_ptr<Signature<2,M>> o(new Signature<2,M>);
//	vector<Signature<2,M>> workspace;
//	workspace.reserve(3);
//	for(int i=1; i!=in.size(); ++i)
//	{
//		workspace.resize(1);
//		Point2d displacement;
//		displacement[0]=in[i][0]-in[i-1][0];
//		displacement[1]=in[i][1]-in[i-1][1];
//		workspace.emplace_back(displacement);
//		workspace.emplace_back(workspace[0],workspace[1]);
//		s
//	}
//	return o;
//}

template<int M>
Signature<3,M> calcPathSig(const Stroke3d& in)
{
	Signature<3,M> o;
	for(size_t i=1; i!=in.size(); ++i)
	{
		Point3d displacement;
		displacement[0]=in[i][0]-in[i-1][0];
		displacement[1]=in[i][1]-in[i-1][1];
		displacement[2]=in[i][2]-in[i-1][2];
		o=Signature<3,M>(o,Signature<3,M>(displacement));
	}
	return o;
}

template<int M>
Signature<3,M> calcPathSig1(const Stroke3d& in)
{
	Signature<3,M> o;
	for(size_t i=1; i!=in.size(); ++i)
	{
		Point3d displacement;
		displacement[0]=in[i][0]-in[i-1][0];
		displacement[1]=in[i][1]-in[i-1][1];
		displacement[2]=in[i][2]-in[i-1][2];
		o.postConcatenateWith(Signature<3,M>(displacement));
	}
	return o;
}


//calc the signature of the path, having rotated it so it makes no net vertical movement.
template<int M>
Signature<2,M> calcPathSigEndingXAxis(const Stroke& in)
{
	const float x=in.back()[0]-in.front()[0];
	const float y=in.back()[1]-in.front()[1];
	if(fabs(y)<0.0001)
		return calcPathSig<M>(in);
	//const float angle = atan2(y,x), c= cos(angle), s=sin(angle); 
	const float hyp = hypotf(y,x), c= x/hyp, s=y/hyp; 
	Signature<2,M> o;
	for(int i=1; i!=in.size(); ++i)
	{
		Point2d displacement;
		const float x_=in[i][0]-in[i-1][0];
		const float y_=in[i][1]-in[i-1][1];
		displacement[0]=c*x_+s*y_;
		displacement[1]=c*y_-s*x_;
		o=Signature<2,M>(o,Signature<2,M>(displacement));
	}
	return o;
}

//calc the signature of the path, having rotated it so it makes no net vertical movement.
template<int M>
Signature<3,M> calcPathSigEndingXAxis(const Stroke3d& in)
{
	const float x=in.back()[0]-in.front()[0];
	const float y=in.back()[1]-in.front()[1];
	if(fabs(y)<0.0001)
		return calcPathSig<M>(in);
	//const float angle = atan2(y,x), c= cos(angle), s=sin(angle); 
	const float hyp = hypotf(y,x), c= x/hyp, s=y/hyp; 
	Signature<3,M> o;
	for(int i=1; i!=in.size(); ++i)
	{
		Point3d displacement;
		const float x_=in[i][0]-in[i-1][0];
		const float y_=in[i][1]-in[i-1][1];
		displacement[0]=c*x_+s*y_;
		displacement[1]=c*y_-s*x_;
		displacement[2]=in[i][2]-in[i-1][2];
		o=Signature<3,M>(o,Signature<3,M>(displacement));
	}
	return o;
}

template<int D, int M>
vector<float> representSignature(const Signature<D,M>& a){
	vector<float> o(a.totalLength);
	int i=0;
	//a.iterateOver([&](float f){array[i++]=0.f;});
	//a.iterateOver([&](float f){array[i++]=f;});
	a.iterateOver([&](float f){o[i++]=f;});
	//a.iterateOver([&](float f){array[i++]=f;array[i++]=f;});
	return o;
}

template<int D, int M>
vector<float> representSignatureTopLevel(const Signature<D,M>& a){
	vector<float> o(a.length);
	for(int i=0; i<a.length; ++i)
		o[i]=a.m_data[i];
	return o;
}

template<int D, int M>
vector<float> representSignatureTopLevelWithDisplacement(const Signature<D,M>& a){
	vector<float> o(a.length+D);
	for(int i=0; i<a.length; ++i)
		o[i]=a.m_data[i];
	auto r = a.root();
	for(int d=0; d<D; ++d)
		o[a.length+d] = r.m_data[d];
	return o;
}

//If a is a Signature<3,M> then
//iterateInitialThrees(a,o) it applies o to all the elements of a whose indices only contain 
//the third dimension initially. Similarly for iterateFinalThrees.
//The number of calls to o will be IterateInitial3sOnly<M>::totalLength

template<int M>
struct IterateInitial3sOnly
{
	typedef IterateInitial3sOnly<M-1> Parent;
	static const int length = 1+Signature<2,M>::totalLength;
	static const int totalLength = Parent::totalLength + length;
	static const int largestInterval=3*Parent::largestInterval;
	template<class T>
	static void indices(T&& o){
		indicesNoThrees(o);
		Parent::indices([&](int i){o(i+2*largestInterval);});
	}
	template<class T>
	static void indicesFinalThrees(T&& o){
		Parent::indicesFinalThrees(o);
		Parent::indicesFinalThrees([&](int i){o(i+largestInterval);});
		o(3*largestInterval-1);
	}
	template<class T>
	static void indicesNoThrees(T&& o){
		Parent::indicesNoThrees(o);
		Parent::indicesNoThrees([&](int i){o(i+largestInterval);});
	}
	template<class T>
	static void go(const Signature<3,M> a, T&& o){
		Parent::go(a.m_parent,o);
		indices([&](int i){o(a.m_data[i]);});
	}
	template<class T>
	static void goFinalThrees(const Signature<3,M> a, T&& o){
		Parent::goFinalThrees(a.m_parent,o);
		indicesFinalThrees([&](int i){o(a.m_data[i]);});
	}
};

template<>
struct IterateInitial3sOnly<1>
{
	static const int length = 3;
	static const int totalLength = 3;
	static const int largestInterval=1;
	template<class T>
	static void indices(T&& o){o(0);o(1);o(2);}
	template<class T>
	static void indicesFinalThrees(T&& o){o(0);o(1);o(2);}
	template<class T>
	static void indicesNoThrees(T&& o){o(0);o(1);}
	template<class T>
	static void go(const Signature<3,1> a, T&& o){
		indices([&](int i){o(a.m_data[i]);});
	}
	template<class T>
	static void goFinalThrees(const Signature<3,1> a, T&& o){
		indicesFinalThrees([&](int i){o(a.m_data[i]);});
	}
};

template<int M, class T>
void iterateInitialThrees(const Signature<3,M> a, T&& o){
	IterateInitial3sOnly<M>::go(a,o);
}
template<int M, class T>
void iterateFinalThrees(const Signature<3,M> a, T&& o){
	IterateInitial3sOnly<M>::goFinalThrees(a,o);
}

#endif
