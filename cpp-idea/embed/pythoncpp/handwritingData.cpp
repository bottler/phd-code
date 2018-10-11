#include "stdafx.h"
#include "handwritingData.h"

#include "SigUtilities.h"

#define ABBREVIATED_LENGTH 100
#define ABBREVIATED_LENGTH_S "100"

class Data
{
public:
	JRSqlite::Connection m_conn;
	JRSqlite::Source1BlobSink1IntStmt m_stmt;
	vector<int> m_trainY, m_testY;
	vector<Character> m_trainX, m_testX;

  //(abbreviated = true) means ignore all but the first 100 characters in the db
  Data(const char* filename, bool abbreviated)
		: m_conn(JRSqlite::Connection::openReadOnly(filename)),
		  m_stmt(m_conn, "select data from strokes where trainxn=? order by stroken"),
		  m_trainX(getAllChars(false, abbreviated)),
		  m_testX(getAllChars(true, abbreviated))
	{
		JRSqlite::Source1IntStmt(m_conn,"select V from TRAINY order by count").execute([&](int i){m_trainY.push_back(i);});
		JRSqlite::Source1IntStmt(m_conn,"select V from TESTY order by count").execute([&](int i){m_testY.push_back(i);});
		if(abbreviated){
		  m_trainY.resize(ABBREVIATED_LENGTH);
		  m_testY.resize(ABBREVIATED_LENGTH);
		}
	}

private:

  vector<Character> getAllChars(bool test, bool abbreviated)
	{
		//#JRSqlite::Source1BlobSink1IntStmt st(m_conn, "select data from strokes where trainxn=? order by stroken");
		//JRSqlite::Source1BlobSink1IntStmt st(m_conn, "select data from strokes where trainxn=?");
		int nChars =JRSqlite::SourceSingleIntStmt(m_conn,test?"select count(*) from testx":"select count(*) from trainx").execute();
		if(abbreviated)
		  nChars=ABBREVIATED_LENGTH;
		vector<Character> out(nChars);
#ifndef EASY
		const char * s = test ? "select testxn, data from teststrokes": "select trainxn, data from strokes";
		const char * s_abb = test ? "select testxn, data from teststrokes where testxn<=" ABBREVIATED_LENGTH_S : "select trainxn, data from strokes where trainxn<=" ABBREVIATED_LENGTH_S;
		JRSqlite::Source1Int1BlobStmt st(m_conn, abbreviated ? s_abb : s);
		st.executeWithSize([&out](int iChar, const char* x, int n)
			{
				Character& vf = out[iChar-1];
				vf.push_back(Stroke());
				Stroke& s = vf.back();
				const int length = n/8;
				s.resize(length);
				const float* xx = reinterpret_cast<const float*>(x);
				for(size_t i=0; i!=length; ++i)
				{
					s[i][0]=xx[i*2];
					s[i][1]=xx[i*2+1];
				}
			}
		);
#endif
		return out;
	}

	Character getStrokes(int index)
	{
		//#JRSqlite::Source1BlobSink1IntStmt st(m_conn, "select data from strokes where trainxn=? order by stroken");
		//JRSqlite::Source1BlobSink1IntStmt st(m_conn, "select data from strokes where trainxn=?");
		Character vf;
		m_stmt.executeWithSize(index,[&vf](const char* x, int n)
			{
				vf.push_back(Stroke());
			
				vf.back().resize(n/8);
				const float* xx = reinterpret_cast<const float*>(x);
				for(size_t i=0; i!=vf.back().size(); ++i)
				{
					vf.back()[i][0]=xx[i*2];
					vf.back()[i][1]=xx[i*2+1];
				}
			}
		);
		return vf;
	}
public:
	void setInterest(const vector<int>& i,bool test){//sorted list of ints, 0-based
		auto& x=test?m_testX : m_trainX;
		auto& y=test?m_testY : m_trainY;
		vector<Character> copyX(i.size());
		vector<int> copyY(i.size());
		int newi=0;
		for(int ii: i){
			copyX[newi].swap(x[ii]);
			copyY[newi++]=y[ii];
		}
		x.swap(copyX);
		y.swap(copyY);
	}
};

void makeDataCopy(const char* db, DataCopy& dc, bool abbreviated){
  {
    Data dd(db, abbreviated);
    dc.m_trainX.swap(dd.m_trainX);
    dc.m_trainY.swap(dd.m_trainY);
    dc.m_testX.swap(dd.m_testX);
    dc.m_testY.swap(dd.m_testY);
  }//let dd be freed and the source database get closed here
}

static std::unique_ptr<Data> g_data;
//static std::future<void> g_data_waiter;

void openData(const char* data){
  bool abbreviated = (*data=='?');
  if(abbreviated)
    ++data;
  g_data.reset(new Data(data, abbreviated));
}

/*
void openDataAsync(const char* data){
  std::string d = data;
  g_data_waiter = std::async(std::launch::async,[d](){
      openData(d.c_str());
    });
}
void waitForOpenData(){
  g_data_waiter.get();
}
*/

void setInterest(const vector<int>& i,bool test){//sorted list of ints, 0-based
  g_data->setInterest(i,test);
}


//shows that all the characters are normalised to [-1,1]x[-1,1]
Stroke concat(const Character& ch);
void verifyBoundingBoxes(){
	for(size_t i=0; i!=g_data->m_trainX.size(); ++i){
		const auto& x = g_data->m_trainX[i];
		auto y = concat(x);
		float x0 = -1000, n0 = 1000;
		float x1 = -1000, n1 = 1000;
		for (auto i : y){
			if(i[0]>x0)
				x0=i[0];
			if(i[1]>x1)
				x1=i[1];
			if(i[0]<n0)
				n0=i[0];
			if(i[1]<n1)
				n1=i[1];
		}
		std::cout<<n0<<","<<x0<<","<<n1<<","<<x1<<endl;
	}
}
const vector<Character>& get_trainX(){return g_data->m_trainX;}
const vector<int>& get_trainY(){return g_data->m_trainY;}
const vector<Character>& get_testX(){return g_data->m_testX;}

	//JRSqlite::SimpleStmt(g_data.m_conn,"begin exclusive transaction").execute();
	//JRSqlite::SimpleStmt(g_data.m_conn,"end transaction").execute();


void distortCharacter(Character& ch,std::mt19937& rnd)
{
	const float stretch=0.3f, rotate=0.3f,jiggle=0.03f;
	std::uniform_real_distribution<float> stretcher(1-stretch,1+stretch);
	std::uniform_real_distribution<float> rotater(-rotate,rotate);
	std::uniform_real_distribution<float> jiggler(-jiggle,jiggle);
	std::uniform_int_distribution<int> chooser(0,2);
	float stretchX=stretcher(rnd), stretchY=stretcher(rnd),alpha=rotater(rnd);
	float c=0,s=0;
	int choice=chooser(rnd);
	if(choice==0)
		c=cos(alpha), s=sin(alpha);
	for(auto& stroke: ch){
		const float offsetX=jiggler(rnd), offsetY=jiggler(rnd);
		for(auto& pt:stroke){
			float x=stretchX*pt[0], y=stretchY*pt[1];
			if(choice==0){
				pt[0]=c*x+s*y+offsetX;
				pt[1]=-s*x+c*y+offsetY;
			}
			if(choice==1){
				pt[0]=x+offsetX;
				pt[1]=alpha*x+y+offsetY;
			}
			if(choice==2){
				pt[0]=x+alpha*y+offsetX;
				pt[1]=y+offsetY;
			}
		}
	}
}

void permuteStrokesRandomly(Character& ch,std::mt19937& rnd){
  std::shuffle(ch.begin(),ch.end(),rnd);
}

//return a stroke resampled at equal distances with a given length
// and also the coordinates of its centre of mass - in mean0 and mean1.
//Ideally we'd do this without so much allocation
Stroke getFixedLength(const Stroke& in, int wantedSize, float& mean0, float& mean1){
  vector<double> lengths (in.size()-1);
  Stroke out(wantedSize);
  if(wantedSize>0){
    out[0]=in[0];
    out.back()=in.back();
  }
  double totalLength=0, sum0=0, sum1=0;
  for(size_t i=0; i<lengths.size(); ++i){
    double length = std::hypot(in[i][0]-in[i+1][0],in[i][1]-in[i+1][1]);
    sum0 += length * (in[i][0]+in[i+1][0]);
    sum1 += length * (in[i][1]+in[i+1][1]);
    totalLength += lengths[i] = length;
  }
  mean0 = sum0 * 0.5 / totalLength;
  mean1 = sum1 * 0.5 / totalLength;
  if(in.size()==1 || totalLength<=0.00000001){
    mean0 = in[0][0];
    mean1 = in[0][1];
    for(auto& a: out)
      a=in[0];
    return out;
  }
  double hadLength = 0, nextTotalLength=lengths[0];
  for(size_t i=1,j=0;i+1<wantedSize;++i){
    double wantedLength=totalLength*i/(wantedSize-1.0);
    while(nextTotalLength<wantedLength && j<lengths.size()){
      hadLength=nextTotalLength;
      ++j;
      nextTotalLength+=lengths[j];
    }
    double frac = (wantedLength-hadLength)/(nextTotalLength-hadLength);
    out[i][0]=static_cast<float>(in[j][0]+frac*(in[j+1][0]-in[j][0]));
    out[i][1]=static_cast<float>(in[j][1]+frac*(in[j+1][1]-in[j][1]));
  }
  return out;
}

//return a stroke resampled at approximately the given distance
//This calculates the length twice - lazy coding
//Ideally we'd do this without so much allocation
Stroke getFixedDistance(const Stroke& in, double wantedDistance){
  double totalLength=0;
  for(size_t i=0; i<in.size()-1; ++i){
    double length = std::hypot(in[i][0]-in[i+1][0],in[i][1]-in[i+1][1]);
    totalLength += length;
  }
  float mean0, mean1; //ignored
  size_t wantedLength = static_cast<size_t>(std::ceil(totalLength/wantedDistance));
  if(wantedLength==0)
    wantedLength=1;
  return getFixedLength(in, wantedLength, mean0, mean1);
}


Stroke3d getFixedLengthUsing2d(const Stroke3d& in, int wantedSize, vector<float>* angles){
  vector<double> lengths (in.size()-1);
  Stroke3d out(wantedSize);
  if(angles)
    angles->assign(wantedSize, 0);
  if(wantedSize>0){
    out[0]=in[0];
    out.back()=in.back();
  }
  double totalLength=0;
  for(size_t i=0; i<lengths.size(); ++i){
    double length = std::hypot(in[i][0]-in[i+1][0],in[i][1]-in[i+1][1]);
    totalLength += lengths[i] = length;
  }
  if(in.size()==1 || totalLength<=0.00000001){
    for(auto& a: out)
      a=in[0];
    return out;
  }
  if(angles){
    angles->front()=std::atan2(in[1][1]-in[0][1],in[1][0]-in[0][0]);
    auto s = in.size();
    angles->back() =std::atan2(in[s-1][1]-in[s-2][1],in[s-1][0]-in[s-2][0]);
  }
  double hadLength = 0, nextTotalLength=lengths[0];
  for(size_t i=1,j=0;i+1<wantedSize;++i){
    double wantedLength=totalLength*i/(wantedSize-1.0);
    while(nextTotalLength<wantedLength && j<lengths.size()){
      hadLength=nextTotalLength;
      ++j;
      nextTotalLength+=lengths.at(j);
    }
    double frac = (wantedLength-hadLength)/(nextTotalLength-hadLength);
    out.at(i)[0]=static_cast<float>(in[j][0]+frac*(in.at(j+1)[0]-in[j][0]));
    out[i][1]=static_cast<float>(in[j][1]+frac*(in[j+1][1]-in[j][1]));
    out[i][2]=static_cast<float>(in[j][2]+frac*(in[j+1][2]-in[j][2]));
    if(angles)
      (*angles)[i]=std::atan2(in[j+1][1]-in[j][1],in[j+1][0]-in[j][0]);
  }
  return out;
}

static double getTotalLengthUsing2d(const Stroke3d& in){
  double totalLength=0;
  for(size_t i=0; i<in.size()-1; ++i){
    double length = std::hypot(in[i][0]-in[i+1][0],in[i][1]-in[i+1][1]);
    totalLength += length;
  }
  return totalLength;
}


//return a stroke resampled at approximately the given distance
//This calculates the length twice - lazy coding
//Ideally we'd do this without so much allocation
Stroke3d getFixedDistanceUsing2d(const Stroke3d& in, double wantedDistance, vector<float>* angles){
  const double totalLength= getTotalLengthUsing2d(in);
  size_t wantedLength = static_cast<size_t>(std::ceil(totalLength/wantedDistance));
  if(wantedLength==0)
    wantedLength=1;
  return getFixedLengthUsing2d(in, wantedLength, angles);
}


vector<Stroke3d> chunkUsing2d(const Stroke3d& in, double wantedDistance){
  vector<double> lengths (in.size()-1);
  vector<Stroke3d> out;
  double totalLength=0;
  for(size_t i=0; i<lengths.size(); ++i){
    double length = std::hypot(in[i][0]-in[i+1][0],in[i][1]-in[i+1][1]);
    totalLength += lengths[i] = length;
  }
  if(in.size()==1 || totalLength<=0.00000001 || totalLength<=wantedDistance){
    out.push_back(in);
    return out;
  }
  double hadLength = 0, nextTotalLength=lengths[0];
  Point3d localPoint = in[0];
  int j=0;
  for(double aim=wantedDistance, lastAim=0; lastAim<totalLength; lastAim=aim, aim+=wantedDistance){
    out.push_back(Stroke3d{});
    auto& dest = out.back();
    dest.push_back(localPoint);
    while(nextTotalLength<aim && j<lengths.size()){
      hadLength=nextTotalLength;
      ++j;
      if(j>=in.size())
	cout<<"!jsadk;l"<<endl;
      dest.push_back(in.at(j));
      nextTotalLength+=lengths[j];
    }
    if(nextTotalLength-0.00000001<hadLength)
      break;
    double frac = (aim-hadLength)/(nextTotalLength-hadLength);
    if(j+1>=in.size())
      cout<<aim<<" "<<hadLength<<" "<<nextTotalLength <<" "<<totalLength<<endl;
    localPoint[0]=static_cast<float>(in[j][0]+frac*(in.at(j+1)[0]-in[j][0]));
    localPoint[1]=static_cast<float>(in[j][1]+frac*(in[j+1][1]-in[j][1]));
    localPoint[2]=static_cast<float>(in[j][2]+frac*(in[j+1][2]-in[j][2]));
    dest.push_back(localPoint);
  }
  //  out.emplace_back(Stroke3d{localPoint});
  return out; 
}

