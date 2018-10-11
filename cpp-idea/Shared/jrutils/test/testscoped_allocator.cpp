//compile me with 
//  g++-4.7 --std=c++11 a.cpp

#include<vector>
#include<map>
#include<set>
#include<list>
#include<string>

//these just for reporting alignment size
#include <cstddef>
#include<iostream>

#include <scoped_allocator>

//#define JRARENA_DEBUG
#include "../jrarena.h"

int main()
{

  HeapArena ha (1048576); //parameter is the size of each block to allocate
  typedef HeapArena A;

  typedef std::vector<double,JRAlloc<double, A> > VD;
  typedef std::vector<VD,JRAlloc<VD, A> > VVD;
  VVD vvd(ha);
  VD vd(ha);
  vvd.push_back(vd);
  vvd.back().push_back(24.433);
  vvd.assign(3,vd);
  //VVD vvd2(3,ha);
  //vvd.emplace_back(0,0.0);
  vvd.emplace_back(0,0.0,ha);

  std::cout<<"SAA"<<std::endl;

  typedef std::scoped_allocator_adaptor<JRAlloc<VD, A>,JRAlloc<double,A> > SAA;
  SAA saa (ha, ha);
  typedef std::vector<VD,SAA> VVD2;
  VVD2 vvd2 (saa);
  vvd2.push_back(vd);//does this only work because == is true?
  vvd2.emplace_back(0,0.0);
  vvd2.emplace_back();//even this works!!

  std::cout<<"SAA2"<<std::endl;

  typedef std::scoped_allocator_adaptor<JRAlloc<double,A> > SAA2;
  SAA2 saa2 (ha);
  typedef std::vector<VD,SAA2> VVD3; //works just as well as before.
  VVD3 vvd3 (saa2);
  vvd3.push_back(vd);
  vvd3.emplace_back(0,0.0);
  vvd3.emplace_back();//even this works!!

  std::cout<<"foo"<<std::endl;

  std::vector<std::vector<double>,SAA> foo(saa);
  foo.emplace_back(); //this works, the allocator is not passed on.
  foo.back().push_back(234);
  //foo.push_back(vd); //good, fails
  

}
