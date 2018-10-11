#include "stdafx.h"
#include "SigUtilities.h"
#include "Representations.h"
#include "boost/mpl/vector.hpp"
#include <boost/mpl/at.hpp>


Stroke concat(const Character& ch){
	Stroke o = ch[0];
	for(int i=1; i<(int)ch.size(); ++i)
		o.insert(o.end(),ch[i].begin(), ch[i].end());
	return o;
}

Stroke3d concatWithStrokeNumber(const Character& ch){
	Stroke3d o;
	for(int i=0; i!=ch.size(); ++i){
		for(const auto& a:ch[i]){
			Point3d pt;
			pt[0]=a[0];
			pt[1]=a[1];
			pt[2]=(float)i;
			o.push_back(pt);
		}
	}
	return o;
}

Stroke3d concatWithPenDimension(const Character& ch){ //0 is a point with pen down, 1 in a point with pen up
	Stroke3d o;
	for(int i=0; i!=ch.size(); ++i){
		if(i!=0)
		{
			Point3d pt;
			pt[0]=ch[i][0][0];
			pt[1]=ch[i][0][1];
			pt[2]=1;
			o.push_back(pt);
		}
		for(const auto& a:ch[i]){
			Point3d pt;
			pt[0]=a[0];
			pt[1]=a[1];
			pt[2]=0;
			o.push_back(pt);
		}
		if(i+1!=ch.size())
		{
			Point3d pt;
			pt[0]=ch[i].back()[0];
			pt[1]=ch[i].back()[1];
			pt[2]=1;
			o.push_back(pt);
		}
	}
	return o;
}

Stroke3d concatWithInkDimension(const Character& ch, vector<float>& workingSpace, bool normalise=true){ 
	//calculate cumulative path length in workingSpace
	workingSpace.assign(1,0);
	float prevx, prevy, totalDistance=0;
	for(int i=0; i<(int)ch.size(); ++i)
	{
		for(int j=0; j!=ch[i].size(); ++j)
		{
			float x=ch[i][j][0], y=ch[i][j][1];
			if(j!=0){
				float distance = hypotf(x-prevx, y-prevy);
				workingSpace.push_back(totalDistance += distance);
			}
			prevx=x; prevy=y;
		}
	}
	const float inverseDistance = normalise ? 1/totalDistance : 1;
	Stroke3d o;
	for(int i=0,workIdx=0; i!=ch.size(); ++i){
		for(int j=0; j!=ch[i].size(); ++j){
			Point3d pt;
			pt[0]=ch[i][j][0];
			pt[1]=ch[i][j][1];
			if (j==0)
				pt[2]=workingSpace[workIdx] * inverseDistance;
			else
				pt[2]=workingSpace[++workIdx] * inverseDistance;
			o.push_back(pt);
		}
	}
	return o;
}

template<int m>
struct UseLogSig1stStroke
{ //Very slow - uses libalgebra
	static int getRepnLength(){return getLogSigSize<2,m>();}
	static vector<float> representChar(const Character& ch){
		auto logSig = getLogSig<2,m>(ch[0]);
		return logSig;
	}
};

template<int m>
struct UseLogSig1stStrokeMyWay
{
	static int getRepnLength(){return getLogSigSize<2,m>();}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(ch[0]);
		return getLogSigFromSignature(sig);
	}
};

template<int m>
struct UseSig1stStroke
{
	static int getRepnLength(){return Signature<2,m>::totalLength;}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(ch[0]);
		return representSignature(sig);
	}
};
template<int m>
struct UseTopLevelSig1stStroke
{
	static int getRepnLength(){return Signature<2,m>::length;}
	static int getNumber(){return m;}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(ch[0]);
		return representSignatureTopLevel(sig);
	}
};
template<int m>
struct UseTopLevelSig1stStrokeWithDisplacement
{
	static int getRepnLength(){return Signature<2,m>::length+2;}
	static int getNumber(){return m;}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(ch[0]);
		return representSignatureTopLevelWithDisplacement(sig);
	}
};

template<class RotationalInvariantData, bool withDisplacement, bool raw>
struct UseIrrotationConcat//1stStroke
{
	typedef RotationalInvariantData D;
	static int getRepnLength(){return (raw ? D::rawLength : D::length) + (withDisplacement?2:0);}
	static int getNumber(){return D::dim;}
	static string getName(){return "concatenated, " + string (raw ? "raw ": "")
		+ "rotational invariant sig parts"
		+string(withDisplacement ? " plus total displacement vector" : "");}

	static vector<float> representChar(const Character& ch){
		//auto sig = calcPathSig<D::dim>(ch[0]);
		auto sig = calcPathSig<D::dim>(concat(ch));
		vector<float> o;
		if(raw)
			getRawRotationInvariants(sig,o);
		else
			getRotationInvariants(sig,o);
		if(withDisplacement){
			const auto& root = sig.root();
			o.push_back(root.m_data[0]);
			o.push_back(root.m_data[1]);
		}
		return o;
	}
};

template<int m>
struct UseSigConcat
{
	static int getRepnLength(){return Signature<2,m>::totalLength;}
	static int getNumber(){return m;}
	static string getName(){return "strokes concatenated, signature";}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(concat(ch));
		return representSignature(sig);
	}
};
template<int m>
struct UseSigConcatEndingXaxis
{
	static int getRepnLength(){return Signature<2,m>::totalLength-m;}
	static int getNumber(){return m;}
	static string getName(){return "strokes concatenated, rotated to end on x axis, signature";}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSigEndingXAxis<m>(concat(ch));
		vector<float> o(getRepnLength()+1);
		int i=0;
		sig.iterateOverWithSignal([&](float f){o[i++]=f;},[&](){--i;});
		o.pop_back();
		return o;
	}
};
template<int m>
struct UseLogSigConcat
{
	static int getRepnLength(){return getLogSigSize<2,m>();}
	static int getNumber(){return m;}
	static string getName(){return "strokes concatenated, log signature";}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(concat(ch));
		return getLogSigFromSignature(sig);
	}
};
//template<int m>
//struct UseSigPenDim
//{
//	static int getRepnLength(){return Signature<3,m>::totalLength;}
//	static int getNumber(){return m;}
//	static string getName(){return "Pen dimension, signature";}
//	static vector<float> representChar(const Character& ch){
//		auto sig = calcPathSig<m>(concatWithPenDimension(ch));
//		return representSignature(sig);
//	}
//};
template<int m>
struct UseTopLevelSigWithDisplacementPenDim
{
	static int getRepnLength(){return Signature<3,m>::length+3;}
	static int getNumber(){return m;}
	static string getName(){return "Pen dimension, top level signature";}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(concatWithPenDimension(ch));
		return representSignatureTopLevelWithDisplacement(sig);
	}
};
template<int m>
struct UseTopLevelSigWithDisplacementInkDim
{
	static int getRepnLength(){return Signature<3,m>::length+3;}
	static int getNumber(){return m;}
	static string getName(){return "Ink dimension, top level signature";}
	static vector<float> representChar(const Character& ch){
		vector<float> workingSpace; //should be shared in the thread to save allocations;
		auto sig = calcPathSig<m>(concatWithInkDimension(ch,workingSpace));
		return representSignatureTopLevelWithDisplacement(sig);
	}
};

template<int m,bool initial, bool final, bool normalised>
struct UseSigInkDim
{
	static int getRepnLength(){return initial?
		IterateInitial3sOnly<m>::totalLength : Signature<3,m>::totalLength;}
	static int getNumber(){return m;}
	static string getName(){return string(normalised?"normalised ":"")+"ink dimension"
		+string(initial? (final ? " final" : " initial") : "") + " signature";}
	static vector<float> representChar(const Character& ch){
		vector<float> workingSpace; //should be shared in the thread to save allocations;
		auto sig = calcPathSig<m>(concatWithInkDimension(ch,workingSpace, normalised));
		if(initial){
			workingSpace.clear();
			if(final)
				iterateFinalThrees(sig,[&](float f){workingSpace.push_back(f);});
			else
				iterateInitialThrees(sig,[&](float f){workingSpace.push_back(f);});
			return workingSpace;
		}
		return representSignature(sig);
	}
};
template<int m,bool initial, bool final>
struct UseSigPenDim
{
	static int getRepnLength(){return initial?
		IterateInitial3sOnly<m>::totalLength : Signature<3,m>::totalLength;}
	static int getNumber(){return m;}
	static string getName(){return "pen dimension"
		+string(initial? (final ? " final" : " initial") : "") + " signature";}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(concatWithPenDimension(ch));
		if(initial){
			vector<float> o;
			o.reserve(IterateInitial3sOnly<m>::totalLength);
			if(final)
				iterateFinalThrees(sig,[&](float f){o.push_back(f);});
			else
				iterateInitialThrees(sig,[&](float f){o.push_back(f);});
			return o;
		}
		return representSignature(sig);
	}
};

template<int m>
struct UseSigStrokeDim
{
	static int getRepnLength(){return Signature<3,m>::totalLength;}
	static int getNumber(){return m;}
	static string getName(){return "stroke dimension signature";}
	static vector<float> representChar(const Character& ch){
		auto sig = calcPathSig<m>(concatWithStrokeNumber(ch));
		return representSignature(sig);
	}
};


//This gives a repn which takes another repn but returns a random N from all but the first KeepFirst
template<class T, int N, int KeepFirst> //N excludes KeepFirst
struct UseRandomNFrom
{
	static int getRepnLength(){return N+KeepFirst;}
	static int getNumber(){return T::getNumber();}
	static vector<float> representChar(const Character& ch){
		static std::array<int, N> g_selection;
		static bool init = false;
		if(!init)
		{
			
			std::mt19937 rnd= std::mt19937(std::random_device()());
			std::uniform_int_distribution<int> uid(KeepFirst,T::getRepnLength()-1);
			std::set<int> s;
			if(T::getRepnLength()<getRepnLength())
				reportError("invalid use of UseRandomNFrom");
			while(s.size()<N)
				s.insert(uid(rnd));
			int i=0;
			for(auto n : s)
				g_selection[i++]=n;
			init=true;
		}
		vector<float> o, a=T::representChar(ch);
		for(int i=0; i<KeepFirst; ++i)
			o.push_back(a[i]);
		for(int i=0; i<N; ++i)
			o.push_back(a[g_selection[i]]);
		return o;
	}
};

template<class T>
struct Double
{
	static int getRepnLength(){return 2*T::getRepnLength();}
	static int getNumber(){return T::getNumber();}
	static string getName(){return "Double: " + T::getName();}
	static vector<float> representChar(const Character& ch){
		auto v = T::representChar(ch);
		const int s = v.size();
		v.resize(2*s);
		for(int i=0; i!=s; ++i)
			v[s+i]=v[i];
		return v;
	}
};

struct Bad
{
	static int getRepnLength(){reportError("bad choice");}
	static int getNumber(){reportError("bad choice");}
	static string getName(){reportError("bad choice");}
	static vector<float> representChar(const Character& ch){
		reportError("bad choice");
	}
};

Stroke constantSpeed(const Stroke& in, float density)
{
	if(in.size()==1)
		return in;
	vector<float> workingSpace;
	workingSpace.assign(1,0);
	float prevx, prevy, totalDistance=0;
	for(int j=0; j!=in.size(); ++j)
	{
		float x=in[j][0], y=in[j][1];
		if(j!=0){
			float distance = hypotf(x-prevx, y-prevy);
			workingSpace.push_back(totalDistance += distance);
		}
		prevx=x; prevy=y;
	}
	
	Stroke out(1,in[0]);
	out.reserve(3+(int)(totalDistance/density));
	if(in.size()>0)
	{
		float currentPos = 0;
		while (currentPos<totalDistance)
		{
			currentPos += density;
			if(currentPos>= totalDistance)
				out.push_back(in.back());
			else {
				auto it = std::lower_bound(workingSpace.begin(), workingSpace.end(),currentPos);
				if(it==workingSpace.end()){
					assert(false);
					out.push_back(in.back());
				}
				else if(it==workingSpace.begin())
					assert(false);
				else if(*it == currentPos){
					int idx = it-workingSpace.begin();
					out.push_back(in[idx]);
				}
				else{
					auto it2 = it;
					--it;
					int idx = it-workingSpace.begin();
					float frac = (currentPos-*it)/(*it2-*it);
					Point2d pt;
					pt[0]=(1-frac)*in[idx][0] + frac*in[idx+1][0];
					pt[1]=(1-frac)*in[idx][1] + frac*in[idx+1][1];
					out.push_back(pt);
				}
			}
		}
	}
	return out;
};

void boundingBox(const Character& ch, float& up, float& dn, float& l, float& r){
	up=r=-999; dn=l=999;
	for(const auto& s:ch){
		for(const auto& p:s){
			if(p[0]>r)
				r=p[0];
			if(p[0]<l)
				l=p[0];
			if(p[1]>up)
				up=p[1];
			if(p[1]<dn)
				dn=p[1];
		}
	}
}

void makeSquare(float& up, float& dn, float& l, float& r){
	if(up-dn>r-l){
		float toAdd = (up-dn)-(r-l);
		r+=0.5f*toAdd;
		l-=0.5f*toAdd;
	}else{
		float toAdd = (r-l)-(up-dn);
		up+=0.5f*toAdd;
		dn-=0.5f*toAdd;
	}
}

template<int nx, int ny, int nDirections, bool bidirectional=false>
struct UseOctogram{
	static int getRepnLength(){return nx*ny*nDirections;}
	static int getNumber(){return nx*1000+ny*1000000+nDirections;}
	static string getName(){return bidirectional ? "Bidirectional Octogram" : "Octogram";}
	static vector<float> representChar(const Character& ch){
		float up, dn, left, right;
		boundingBox(ch, up, dn, left, right);
		makeSquare(up, dn, left, right);
		vector<float> out(getRepnLength());
		for(const auto& st : ch){
			auto source = constantSpeed(st,0.0125f);
			auto lastit = source.begin();
			for(auto it = ++source.begin(); it!=source.end(); ++it){
				float xpos=std::min((*it)[0],(*lastit)[0]);
				float ypos=std::min((*it)[1],(*lastit)[1]);
				int xindex = static_cast<int>(std::floor(((xpos-(left))/(right-left))*nx));
				if(xindex == nx || xindex == -1)
					xindex=0;
				int yindex = static_cast<int>(std::floor(((ypos-(dn))/(up-dn))*ny));
				if(yindex == ny || yindex == -1)
					yindex=0;
				float dir = std::atan2((*it)[1]-(*lastit)[1],(*it)[0]-(*lastit)[0]);
				const float pi = 3.14159265358979323846f;
				if(bidirectional && dir<0)
					dir += pi;
				float dir2 = bidirectional ? nDirections*dir/pi
								: nDirections*(dir/(2*pi)+0.5f); //in [0,nDirections];
				int idxToSet = static_cast<int>(std::floor(dir2));
				if(idxToSet == nDirections || idxToSet == -1)
					idxToSet=0;
				bool ok = xindex>=0 && xindex<nx && yindex>=0 && yindex<ny && idxToSet>=0 && idxToSet<nDirections;
				if(!ok){
					cout<<xpos<<","<<ypos<<endl;
					cout<<xindex<<","<<yindex<<","<<idxToSet<<endl;
					reportError("octogram failure");
				}
				int nextIdx = (idxToSet+1) % nDirections;
				float nextamt = dir2-idxToSet, amt=1-nextamt;
				out[xindex*ny*nDirections + yindex*nDirections + idxToSet] += amt;
				out[xindex*ny*nDirections + yindex*nDirections + nextIdx] += nextamt;
				lastit = it;
			}
		}
		return out;
	}
};

template<int N>
struct FixedNumberOfPointsIgnoringJumps{
	static int getRepnLength(){return N*2;}
	static string getName(){return "evenly spaced points ignoring jumps";}
	static vector<float> representChar(const Character& ch){
		Stroke st;
		for(const auto& s:ch)
		{
			auto constant = constantSpeed(s,0.05f);
			st.insert(st.end(), constant.begin(), constant.end());
		}
		vector<float> out;
		out.reserve(getRepnLength());
		out.push_back(st[0][0]); out.push_back(st[0][1]);
		for(int i=1; i<N; ++i){
			float pos = (((float) i)*(st.size()-1))/N;
			int ipos = (int) floor(pos);
			if(ipos<0)
				ipos=0;
			if(ipos==st.size()-1){
				out.push_back(st[ipos][0]); out.push_back(st[ipos][1]);
			}
			else{
				float frac = pos-ipos;
				out.push_back(st[ipos][0]*(1-frac)+st[ipos+1][0]*(frac));
				out.push_back(st[ipos][1]*(1-frac)+st[ipos+1][1]*(frac));
			}
		}
		return out;
	}
};

struct UseMulti
{
	static int g_choice;
#define TypeForUseMulti(i) UseSigConcat<i>
	//typedef
	//	boost::mpl::vector<
	//		TypeForUseMulti(1),
	//		TypeForUseMulti(2),
	//		TypeForUseMulti(3),
	//		TypeForUseMulti(4),
	//		TypeForUseMulti(5),
	//		TypeForUseMulti(6),
	//		TypeForUseMulti(7),
	//		TypeForUseMulti(8),
	//		TypeForUseMulti(9),
	//		TypeForUseMulti(10),
	//		TypeForUseMulti(11),
	//		TypeForUseMulti(12),
	//		TypeForUseMulti(13),
	//		TypeForUseMulti(14),
	//		TypeForUseMulti(15)
	//	> type;	
	typedef
		boost::mpl::vector<
			TypeForUseMulti(1),
			TypeForUseMulti(8),
			TypeForUseMulti(9),
			TypeForUseMulti(10),
			Bad,Bad,Bad,Bad,Bad,Bad,Bad,Bad,Bad,Bad,Bad,Bad,Bad
		> type;
	//typedef
	//	boost::mpl::vector<
	//		UseSigConcat<2>,
	//		UseLogSigConcat<2>,
	//		Double<UseLogSigConcat<2>>,
	//		Bad,Bad,Bad,Bad,Bad,Bad,Bad,Bad
	//	> type;
	//typedef
	//	boost::mpl::vector<
	//		UseOctogram<4,4,6,true>,
	//		UseLogSigConcat<7>,
	//		UseOctogram<4,4,6,false>,
	//		UseOctogram<4,4,3,true>,
	//		UseOctogram<4,4,3,false>,
	//		UseOctogram<2,2,3,true>,
	//		UseOctogram<2,2,3,false>,
	//		Bad,Bad,Bad,//
	//		//Bad,Bad,Bad,Bad,Bad,Bad,
	//		Bad,Bad,Bad,Bad,Bad,Bad,
	//		Bad> type;
	static int getRepnLength(){
		using boost::mpl::at_c;
		switch(g_choice-1)
		{
		case 0: return at_c<type,0>::type::getRepnLength();
		case 1: return at_c<type,1>::type::getRepnLength();
		case 2: return at_c<type,2>::type::getRepnLength();
		case 3: return at_c<type,3>::type::getRepnLength();
		case 4: return at_c<type,4>::type::getRepnLength();
		case 5: return at_c<type,5>::type::getRepnLength();
		case 6: return at_c<type,6>::type::getRepnLength();
		case 7: return at_c<type,7>::type::getRepnLength();
		case 8: return at_c<type,8>::type::getRepnLength();
		case 9: return at_c<type,9>::type::getRepnLength();
		case 10: return at_c<type,10>::type::getRepnLength();
		case 11: return at_c<type,11>::type::getRepnLength();
		case 12: return at_c<type,12>::type::getRepnLength();
		case 13: return at_c<type,13>::type::getRepnLength();
		case 14: return at_c<type,14>::type::getRepnLength();
		}
		reportError("bad choice");
	}
	static int getNumber(){
		using boost::mpl::at_c;
		switch(g_choice-1)
		{
		case 0: return at_c<type,0>::type::getNumber();
		case 1: return at_c<type,1>::type::getNumber();
		case 2: return at_c<type,2>::type::getNumber();
		case 3: return at_c<type,3>::type::getNumber();
		case 4: return at_c<type,4>::type::getNumber();
		case 5: return at_c<type,5>::type::getNumber();
		case 6: return at_c<type,6>::type::getNumber();
		case 7: return at_c<type,7>::type::getNumber();
		case 8: return at_c<type,8>::type::getNumber();
		case 9: return at_c<type,9>::type::getNumber();
		case 10: return at_c<type,10>::type::getNumber();
		case 11: return at_c<type,11>::type::getNumber();
		case 12: return at_c<type,12>::type::getNumber();
		case 13: return at_c<type,13>::type::getNumber();
		case 14: return at_c<type,14>::type::getNumber();
		}
		reportError("bad choice");
	}
	static vector<float> representChar(const Character& ch){
		using boost::mpl::at_c;
		switch(g_choice-1)
		{
		case 0: return at_c<type,0>::type::representChar(ch);
		case 1: return at_c<type,1>::type::representChar(ch);
		case 2: return at_c<type,2>::type::representChar(ch);
		case 3: return at_c<type,3>::type::representChar(ch);
		case 4: return at_c<type,4>::type::representChar(ch);
		case 5: return at_c<type,5>::type::representChar(ch);
		case 6: return at_c<type,6>::type::representChar(ch);
		case 7: return at_c<type,7>::type::representChar(ch);
		case 8: return at_c<type,8>::type::representChar(ch);
		case 9: return at_c<type,9>::type::representChar(ch);
		case 10: return at_c<type,10>::type::representChar(ch);
		case 11: return at_c<type,11>::type::representChar(ch);
		case 12: return at_c<type,12>::type::representChar(ch);
		case 13: return at_c<type,13>::type::representChar(ch);
		case 14: return at_c<type,14>::type::representChar(ch);
		}
		reportError("bad choice");
	}
	static string getName(){
		using boost::mpl::at_c;
		switch(g_choice-1)
		{
		case 0: return at_c<type,0>::type::getName();
		case 1: return at_c<type,1>::type::getName();
		case 2: return at_c<type,2>::type::getName();
		case 3: return at_c<type,3>::type::getName();
		case 4: return at_c<type,4>::type::getName();
		case 5: return at_c<type,5>::type::getName();
		case 6: return at_c<type,6>::type::getName();
		case 7: return at_c<type,7>::type::getName();
		case 8: return at_c<type,8>::type::getName();
		case 9: return at_c<type,9>::type::getName();
		case 10: return at_c<type,10>::type::getName();
		case 11: return at_c<type,11>::type::getName();
		case 12: return at_c<type,12>::type::getName();
		case 13: return at_c<type,13>::type::getName();
		case 14: return at_c<type,14>::type::getName();
		}
		reportError("bad choice");
	}
};
int UseMulti::g_choice=-1;
void setChoice(int c){UseMulti::g_choice = c;}

//typedef UseRandomNFrom<UseTopLevelSig1stStrokeWithDisplacement,0,2> Use;
//typedef UseIrrotation1stStroke<false,false> Use;
//typedef UseOctogram<4,4,6> Use;
//typedef UseLogSigConcat<6> Use;
typedef UseMulti Use;

int getRepnLength(){return Use::getRepnLength();}
vector<float> representChar(const Character& ch){return Use::representChar(ch);}
string getRepnName(){return Use::getName();}
int getRepnNumber(){return Use::getNumber();}

//for pendigits, 2stroke 457, stroke number is very good for m=4

//void testOctogram()
//{
//	UseOctogram<1,1,8,true> u;
//	Character ch;
//	ch.push_back(Stroke());
//	ch[0].resize(2);
//	const int n = 20;
//	for(int i=0; i!=n; ++i){
//		float angle = 3.14159f*i/n;
//		ch[0][1][0]=cos(angle);
//		ch[0][1][1]=sin(angle);
//		auto a = u.representChar(ch);
//		for(auto b : a)
//			cout<<b<<", ";
//		cout<<endl;
//	}
//}

