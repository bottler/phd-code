#include "stdafx.h"
//#include"chinese.cpp"
#include"chinese.h"
#include"handwritingData.h"

int main(){
  //  writeRandomSample();
  setDataCopyAsync("/home/jeremyr/g/jrmisc/chinese/casia1.1.sqlite");
  waitForOpenDataCopy();
  int i=0;
  //if (0)
  while(++i<2000)
  {
    std::cout<<i<<std::endl;
    //auto a = getRNNinput_readyForPython();
    auto a = getRNNinput_variableLength_readyForPython();
    /*
    using namespace boost::accumulators;
    accumulator_set<double,stats<tag::mean,tag::variance>>  ss;
    for(auto& b : a.trainX){
      for(int j=0; j<b.size(); j+=3)
	ss(b[j+2]);
    }
    std::cout<<mean(ss)<<endl;
    */
  }
  return 0;
}

int main3(){
  int nPoints = 40000;
  double pi = 4 * atan(1);
  double increment = pi*2.0/nPoints;
  Stroke s;
  for(int i=0; i<=nPoints; ++i){
    Point2d a;
    double value = increment*i;
    //    a[1]=cos(value);
//   a[0]=sin(value);
//    a[0]=2*cos(value)-cos(2*value);a[1]=2*sin(value)-sin(2*value);//cardioid
//    a[0]=cos(value)/(1+sin(value)*sin(value));a[1]=sin(value)*a[0];//lemniscate
    //a[0]=cos(value)*sin(2*value); a[1]=sin(value)*sin(2*value);//quadrifolium
    //    a[0]=cos(value)*sin(10*value); a[1]=sin(value)*sin(10*value);//rose
    s.push_back(a);
  }
  if(0)
  for(int i=0; i<=nPoints; ++i){
    Point2d a;
    double value = increment*i;
    a[0]=2-cos(value);
    a[1]=sin(value);
    s.push_back(a);
  }
  /*
    using F = LogSigFromSigBaseRepn<6>;
  //  using F = SigBaseRepn<4>;
  vector<float> out(F::strokeRepnLength);
  F::strokeRepn(s,out);
  F::strokeRepn(s,out);
  for(const auto & f:out)
    cout<<f<<"\n";
  */
  return 0;
}

int main4(){
  // Stroke3d in = {{0,0,0},{1,0,0},{1,10,0}};
  Stroke3d in = {{1,10,0}};
  auto out = chunkUsing2d(in,103.0341);
  for(auto& s : out){
    cout<<"\n";
    for(auto& p : s)
      cout<<p[0]<<","<<p[1]<<","<<p[2]<<"\n";
  }
    return 0;
}
