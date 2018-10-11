#ifndef JR_CHECKANDCALL_H
#define JR_CHECKANDCALL_H

/*
This is a simple solution to the perennial long boring switch statement to convert from an int to an int template parameter.

checkAndCall<A,F>::go(a,nameforerror,p...) calls F<a>::go(p...) if a is an integer between 1 and A else throws an error 
F needs to be a template type (taking an int) with a public static void method called go.
Of course, F will be instantiated for all a between 1 and A.
Parameters are subject to autodeduction and copied from function to function, so objects might want to be wrapped in ref/cref

DEMO:
template<int i>
struct MyTemplate{
  static void go () {cout<<"hello from number "<< i<<endl;}
};
void test(){
  checkAndCall<5, MyTemplate>::go(6, "what MyTemplate does (say hello)");
}

Similarly
checkAndCall2<A,B,F>::go(a,b,nameforerror,p...) calls F<a,b>::go(p...) if a is an integer between 1 and A, b between 1 and B
checkAndCallF<A,F,fixedint>::go(a,nameforerror,p...) calls F<a,fixedint>::go(p...) if a is an integer between 1 and A, for fixedint a fixed int


*/

#include<sstream>


template<int i, template<int> class T>
class checkAndCall{
public:
  template<class... params>
  static void go (int wanted, const char* name, params... p) {
    if(i==wanted){
      T<i>::go(p...);
      return;
    }
    checkAndCall<i-1,T>::go(wanted,name, p...);
  }
};

template<template<int> class T>
class checkAndCall<1, T>{
public:
  template<class... params>
  static void go(int wanted, const char* name, params... p){
    if (1==wanted){
      T<1>::go(p...);
      return;
    }
    std::ostringstream oss;
    //oss<<wanted<<" is out of range of expected in calculation of "<<name;
    oss<<name<<" was requested for an out-of-range parameter";
    throw std::runtime_error(oss.str());
  }
};


template<int i, template<int, int> class T, int fixedint>
class checkAndCallF{
public:
  template<class... params>
  static void go (int wanted, const char* name, params... p) {
    if(i==wanted){
      T<i,fixedint>::go(p...);
      return;
    }
    checkAndCallF<i-1,T,fixedint>::go(wanted,name, p...);
  }
};
template<template<int,int> class T, int fixedint>
class checkAndCallF<1, T, fixedint>{
public:
  template<class... params>
  static void go(int wanted, const char* name, params... p){
    if (1==wanted){
      T<1,fixedint>::go(p...);
      return;
    }
    std::ostringstream oss;
    //oss<<wanted<<" is out of range of expected in calculation of "<<name;
    oss<<name<<" was requested for an out-of-range parameter";
    throw std::runtime_error(oss.str());
  }
};


template<int i, int j, template<int,int> class T>
class checkAndCall2{
public:
  template<class... params>
    static void go (int wantedi, int wantedj, const char* name, params... p) {
    if(i==wantedi){
      if(j==wantedj){
	T<i,j>::go(p...);
	return;
      }
      checkAndCall2<i,j-1,T>::go(wantedi,wantedj,name,p...);
      return;
    }
    checkAndCall2<i-1,j,T>::go(wantedi,wantedj,name, p...);
  }
};

template<int i,template<int,int> class T>
class checkAndCall2<i, 1, T>{
public:
  template<class... params>
    static void go(int wantedi, int wantedj, const char* name, params... p){
    if (i==wantedi){
      if(1==wantedj){
	T<i,1>::go(p...);
	return;
      }
      std::ostringstream oss;
      oss<<name<<" was requested for an out-of-range parameter";
      throw std::runtime_error(oss.str());
    }
    checkAndCall2<i-1,1,T>::go(wantedi,wantedj,name, p...);
  }
};

template<int j,template<int,int> class T>
class checkAndCall2<1, j, T>{
public:
  template<class... params>
    static void go(int wantedi, int wantedj, const char* name, params... p){
    if (1==wantedi){
      if(j==wantedj){
	T<1,j>::go(p...);
	return;
      }
      checkAndCall2<1,j-1,T>::go(wantedi,wantedj,name,p...);
      return;
    }
    std::ostringstream oss;
    oss<<name<<" was requested for an out-of-range parameter";
    throw std::runtime_error(oss.str());
  }
};

template<template<int,int> class T>
class checkAndCall2<1, 1, T>{
public:
  template<class... params>
    static void go(int wantedi, int wantedj, const char* name, params... p){
    if (1==wantedi && 1==wantedj){
	T<1,1>::go(p...);
	return;
    }
    std::ostringstream oss;
    oss<<name<<" was requested for an out-of-range parameter";
    throw std::runtime_error(oss.str());
  }
};


#endif
