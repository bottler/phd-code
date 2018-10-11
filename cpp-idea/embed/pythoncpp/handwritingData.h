#ifndef JR_HANDWRITINGDATA_H
#define JR_HANDWRITINGDATA_H

#include "SigUtilities.h"

struct DataCopy{
  vector<int> m_trainY, m_testY;
  vector<Character> m_trainX, m_testX;
  //  std::thread m_t;
};

void makeDataCopy(const char* db, DataCopy& dc, bool abbreviated);

void openData(const char* data);
void setInterest(const vector<int>& i,bool test);
void verifyBoundingBoxes();
const vector<Character>& get_trainX();
const vector<int>& get_trainY();
const vector<Character>& get_testX();

void distortCharacter(Character& ch, std::mt19937& rnd);
void permuteStrokesRandomly(Character& ch,std::mt19937& rnd);

//return a stroke resampled at equal distances with a given length
// and also the coordinates of its centre of mass - in mean0 and mean1.
//Ideally we'd do this without so much allocation
Stroke getFixedLength(const Stroke& in, int wantedSize, float& mean0, float& mean1);
//return a stroke resampled at approximately the given distance
//This calculates the length twice - lazy coding
//Ideally we'd do this without so much allocation
Stroke getFixedDistance(const Stroke& in, double wantedDistance);

//as above but drags an extra dimension along - length just uses the first two.
//If angles is non-null, it is filled with the angles (in the plane of the first 2 dims).
Stroke3d getFixedLengthUsing2d(const Stroke3d& in, int wantedSize, vector<float>* angles);
Stroke3d getFixedDistanceUsing2d(const Stroke3d& in, double wantedDistance, vector<float>* angles);
vector<Stroke3d> chunkUsing2d(const Stroke3d& in, double wantedDistance);

#endif
