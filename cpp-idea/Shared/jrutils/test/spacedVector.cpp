#include "../spacedVector.h"
#include<iostream>


int main(){
  SpacedVector<int> c(3,7);
  for(int i : c)
    std::cout<<i<<"\n";
}
