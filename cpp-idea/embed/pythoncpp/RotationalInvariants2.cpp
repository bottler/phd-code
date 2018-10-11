#include "stdafx.h"
#include "SigUtilities.h"

void try3DInvariants(const Signature<3,1>& sig, vector<float>& out)
{
	out.push_back(sig.m_data[2]);
}
void try3DInvariants(const Signature<3,2>& sig, vector<float>& out)
{
	try3DInvariants(sig.m_parent, out);
	out.push_back(sig.m_data[8]);
	out.push_back(sig.m_data[0]+sig.m_data[4]);
	out.push_back(sig.m_data[1]-sig.m_data[3]);
}

template<int N1> struct Tern1{static const int place=1, value=N1;};
template<int N1, int N2>
struct Tern2{
	typedef Tern1<N1> P;
	static const int place=3 * P::place;
	static const int value=place*N2+P::value;
};
template<int N1, int N2, int N3>
struct Tern3{
	typedef Tern2<N1,N2> P;
	static const int place=3 * P::place;
	static const int value=place*N3+P::value;
};

void try3DInvariants(const Signature<3,3>& sig, vector<float>& out)
{
	try3DInvariants(sig.m_parent, out);
	{
		TernaryNumber<3> tn1, tn2;
		tn1.m_data[2] = tn2.m_data[2]=2;
		tn2.m_data[0] = 1;
		tn2.m_data[1] = 1;
		out.push_back(sig.m_data[tn1.toInt()]+sig.m_data[tn2.toInt()]);
	}
	{
		TernaryNumber<3> tn1, tn2;
		tn1.m_data[2] = tn2.m_data[2]=2;
		tn1.m_data[0] = 1;
		tn2.m_data[1] = 1;
		out.push_back(sig.m_data[tn1.toInt()]-sig.m_data[tn2.toInt()]);
	}
	{
		TernaryNumber<3> tn1, tn2;
		tn1.m_data[0] = tn2.m_data[0]=2;
		tn2.m_data[1] = 1;
		tn2.m_data[2] = 1;
		out.push_back(sig.m_data[tn1.toInt()]+sig.m_data[tn2.toInt()]);
	}
	{
		TernaryNumber<3> tn1, tn2;
		tn1.m_data[0] = tn2.m_data[0]=2;
		tn1.m_data[1] = 1;
		tn2.m_data[2] = 1;
		out.push_back(sig.m_data[tn1.toInt()]-sig.m_data[tn2.toInt()]);
	}
	{
		TernaryNumber<3> tn1, tn2;
		tn1.m_data[1] = tn2.m_data[1]=2;
		tn2.m_data[0] = 1;
		tn2.m_data[2] = 1;
		out.push_back(sig.m_data[tn1.toInt()]+sig.m_data[tn2.toInt()]);
	}
	{
		TernaryNumber<3> tn1, tn2;
		tn1.m_data[1] = tn2.m_data[1]=2;
		tn1.m_data[0] = 1;
		tn2.m_data[2] = 1;
		out.push_back(sig.m_data[tn1.toInt()]-sig.m_data[tn2.toInt()]);
	}
	out.push_back(sig.m_data.back());
}

