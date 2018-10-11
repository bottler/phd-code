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

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "../jrarena.h"

int main()
{
  std::cout<<"we provide alignment 8. This platform needs "
	   <<sizeof(std::max_align_t)<<"."<<std::endl;

#if 1
  HeapArena ha (1048576); //parameter is the size of each block to allocate
  typedef HeapArena A;
#else
  StackThenHeapArena ha;
  typedef StackThenHeapArena A;
#endif

  std::vector<double,JRAlloc<double, A> > vd(ha);
  for(int i=0;i!=3212; ++i)
    vd.push_back(34*i);

  //This fails because the alignment is too great
  //  std::vector<__int128,JRAlloc<__int128, A> > vi(ha);
  //for(int i=0;i!=3212; ++i)
  //  vi.push_back(34*i);


  std::list<int,JRAlloc<int, A> > li(ha);
  li.push_front(39);

  //std::set<int,std::less<int>,JRAlloc<int, A> > si(ha);//Should be OK, clang++ OK, but g++-4.7 complains 
  std::set<int,std::less<int>,JRAlloc<int, A> > si(std::less<int>(),ha);
  si.insert(32);
  
  //note that the allocator type for map things is std::pair<key,value>
  //std::map<std::string,int,std::less<std::string>,JRAlloc<std::pair<std::string,int>, A> > msi(ha);
  std::map<std::string,int,std::less<std::string>,JRAlloc<std::pair<std::string,int>, A> > msi(std::less<std::string>(),ha);
  msi["afjklghl"]=li.front();

  boost::unordered_set<int,boost::hash<int>,std::equal_to<int>,JRAlloc<int, A > >
    usi(ha);
  usi.insert(321);


  typedef 
    boost::unordered_map<std::string,int,boost::hash<std::string>,std::equal_to<std::string>,
    JRAlloc<std::pair<const std::string,int>, A> > 
      UMSI;
  UMSI umsi(ha);
  umsi["ajkghlkj"]=34;

}
