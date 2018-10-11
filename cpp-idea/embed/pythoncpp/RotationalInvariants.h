#ifndef JR_ROTATIONALINVARIANTS_H
#define JR_ROTATIONALINVARIANTS_H

//based on Joscha Diehl 2013

//getRawRotationInvariants produces a set of ((2*m) choose m)
//rotational invariants from the top level of a
//Signature<2,2*m> for m=1, 2, 3, 4, 5 (length 2,6,20,70,252)
//the code is generated straight from running  
// CcodeFor (snd (calc m))
//after the code in generateBases.fsx

//getRotationInvariants returns a set of INDEPENDENT rotation 
//invariants from the whole of a 
//Signature<2,2*m> for m= 1, 2, 3, 4
//The code is generated from first running
//let ff= calc m in toClip("k="+printR(fst ff)+"\n"+"n="+printR(snd ff))
//in generateBases.fsx
//and then pasting into calcBases.R and running
//toC(reduce(ontoNull(t(k)) %*% n))
//This result does look untidy - I can't work out how to systematically find a "nice" basis for the given subspace.
//The length of the top level is 2,3,10,31,108
//The total length is 2,5,15,46,154

//for m=5 the calculation is easier without RStudio (because pasting makes RStudio think) 
//and also k needs to be sphered for svd to not fail

using std::vector;

void getRotationInvariants (const Signature<2,2>& sig, vector<float>& out);
void getRotationInvariants (const Signature<2,4>& sig, vector<float>& out);
void getRotationInvariants (const Signature<2,6>& sig, vector<float>& out);
void getRotationInvariants (const Signature<2,8>& sig, vector<float>& out);
void getRotationInvariants (const Signature<2,10>& sig, vector<float>& out);
void getRawRotationInvariants (const Signature<2,2>& sig, vector<float>& out);
void getRawRotationInvariants (const Signature<2,4>& sig, vector<float>& out);
void getRawRotationInvariants (const Signature<2,6>& sig, vector<float>& out);
void getRawRotationInvariants (const Signature<2,8>& sig, vector<float>& out);
void getRawRotationInvariants (const Signature<2,10>& sig, vector<float>& out);

struct RotationalInvariantData1{
	static const int dim=2, length=2, rawLength=2;
};
struct RotationalInvariantData2{
	static const int dim=4, length=5, rawLength=6;
};
struct RotationalInvariantData3{
	static const int dim=6, length=15, rawLength=20;
};
struct RotationalInvariantData4{
	static const int dim=8, length=46, rawLength=70;
};
struct RotationalInvariantData5{
	static const int dim=10, length=154, rawLength=252;
};

void try3DInvariants(const Signature<3,3>& sig, vector<float>& out);

#endif
