#include<vector>
#include<map>
#include<set>
#include<list>
#include<string>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "../jrarena_old.h"

int main()
{


  HeapArena ha (1048576); //parameter is the size of each block to allocate

  std::vector<double,JRAlloc<double, HeapArena> > vd(ha);
  for(int i=0;i!=3212; ++i)
    vd.push_back(34*i);


  std::list<int,JRAlloc<int, HeapArena> > li(ha);
  li.push_front(39);

  //in C++98, you can't supply the allocator without supplying the comparator
  std::set<int,std::less<int>,JRAlloc<int, HeapArena> > si(std::less<int>(),ha);
  si.insert(32);
  
  //note that the allocator type for map things is std::pair<key,value>
  std::map<std::string,int,std::less<std::string>,JRAlloc<std::pair<std::string,int>, HeapArena> > msi(std::less<std::string>(),ha);
  msi["afjklghl"]=li.front();

  boost::unordered_set<int,boost::hash<int>,std::equal_to<int>,JRAlloc<int, HeapArena > >
    usi(ha);
  usi.insert(321);


  typedef 
    boost::unordered_map<std::string,int,boost::hash<std::string>,std::equal_to<std::string>,
    JRAlloc<std::pair<const std::string,int>, HeapArena > > 
      UMSI;
  UMSI umsi(ha);
  umsi["ajkghlkj"]=34;

}
