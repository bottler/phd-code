#ifndef RANDOMUTILS_H
#define RANDOMUTILS_H

#include <vector>
#include <array>
#include <bitset>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iterator>
#include <forward_list>

using std::vector;
//using namespace std;

struct MyRand
{
	typedef unsigned int result_type;
	static result_type min() {return 0;}
	static result_type max() {return 1<<16;}
	result_type operator() () {return 0;}
};
struct MyRand123
{
	unsigned int i;
	MyRand123():i(0){}
	typedef unsigned int result_type;
	static result_type min() {return 0;}
	static result_type max() {return 1<<31;}
	result_type operator() () {return ++i;}
};

//This acts like functor T which takes bool, but calculates N at a time on a background thread.
//The background thread is started with start(). Do not call operator() without calling start.
//repeated use of async inside produce seems to be a bad idea
//The destructor of this object isn't perfect. A couple of Ctrl-C's is good enough for me at the moment.
template<typename T, int N> class AsyncCalculator
{
public:
	typedef typename T::result_type result_type;
private:
	std::mutex m_m;
	std::condition_variable m_cv;
	volatile bool m_e1,m_f1,m_e2, m_f2, m_init;
	std::atomic<bool> m_terminate;
	T& m_func;
	std::array<result_type, N+N> m_buffer;
	std::thread m_producer;
	int m_index;
	void dbg(const char* a){}
public:
 	AsyncCalculator(T& func):m_terminate(false), m_func(func), m_index(N+N-1){
		m_e1=true,m_f1=false,m_e2=true, m_f2=false,m_init=true;
	}
	void start(){m_producer = std::thread( [this]{produce();} ); }
	~AsyncCalculator(){
	  m_terminate = true; 
	  //std::cout<<"a:"<<this<<std::endl;
	  if(m_producer.joinable()){ 
	    //std::cout<<"b:"<<this<<std::endl;
#if 1
	    m_producer.detach();
	    //std::cout<<"e:"<<this<<std::endl;
#else
	    while(m_terminate) 
	      m_cv.notify_all(); 
	    //std::cout<<"c:"<<this<<std::endl;
	    m_producer.join();
#endif
	  }
	  //std::cout<<"d:"<<this<<std::endl;
	}
	result_type operator()(){
		++m_index;
		if(m_index==N){
			{
				std::unique_lock<std::mutex> u(m_m);
				m_e1 = true;
				m_f1 = false;
				dbg("can I read from half 2?\n");
				m_cv.notify_all();
				m_cv.wait(u, [&]{return m_f2;});
				dbg("can read from half 2\n");
			}
			m_cv.notify_all();
		}
		if(m_index==N+N){
			m_index=0;
			{
				std::unique_lock<std::mutex> u(m_m);
				if(m_init)
					m_init=false;
				else
				{
					m_e2 = true;
					m_f2 = false;
				}
				dbg("can I read from half 1?\n");
				m_cv.notify_all();//
				m_cv.wait(u, [&]{return m_f1;});
				dbg("can read from half 1\n");
			}
			m_cv.notify_all();
		}
		result_type o=m_buffer[m_index];
		//std::ostringstream oss;
		//oss<<o<<"\n";
		//dbg(oss.str().c_str());
		return o;
	}
private:
	//Produce is a function which can be running when the destructor is called.
	//It can't wait without checking for m_terminate, and it can't do anything that won't always work immediately.
	void produce(){
		while(1){
			for(int k=0; k!=N; ++k)
				m_buffer[k] = m_func();
			{
				std::unique_lock<std::mutex> u(m_m);
				m_f1 = true;
				m_e1 = false;
				dbg("can I write to half 2?\n");
				m_cv.notify_all();
				while(!m_e2)
				{
					if(m_terminate) {m_terminate = false; return;}
					m_cv.wait(u);
				}
				dbg("can write to half 2\n");
			}
			m_cv.notify_all();
			for(int k=N; k!=N+N; ++k)
				m_buffer[k] = m_func();
			{
				std::unique_lock<std::mutex> u(m_m);
				m_f2 = true;
				m_e2 = false;
				dbg("can I write to half 1?\n");
				m_cv.notify_all();
				while(!m_e1)
				{
					if(m_terminate) {m_terminate = false; return;}
					m_cv.wait(u);
				}
				dbg("can write to half 1\n");
			}
			m_cv.notify_all();
		}
	}
};

//Uses AsyncCalculator to make a random number generator 
//(default constructed or constructed from long)
template<typename T, int N> class AsyncRand
{
public:
	typedef typename T::result_type result_type;
	T m_rng;
	AsyncCalculator<T, N> m_calc;
	static result_type min() {return T::min();}
	static result_type max() {return T::max();}
	AsyncRand():m_calc(m_rng){m_calc.start();}
	AsyncRand(long seed):m_rng(seed),m_calc(m_rng){m_calc.start();}
	result_type operator()(){return m_calc();}
};

//A functor which takes a series of functors and returns the value of each in turn.
template<class T>
class AlternatingFunction
{
	T m_begin, m_end;
	T m_current;
public:
	typedef typename std::iterator_traits<T>::value_type::result_type result_type;
	AlternatingFunction(){}
	void start(T begin, T end){
		m_begin=begin; m_end=end; m_current=begin;
	}
	result_type operator()(){
		if(m_current==m_end)
			m_current=m_begin;
		return ((*(m_current++))());
	}
};

//An alternating series of AsyncCalculators to calculate T's, all the T's are default constructed
template<class T, int threads, int N> 
class AlternatingAsyncs{
	std::array<T, threads> m_funcs;
	std::forward_list<AsyncCalculator<T,N>> m_asyncs;
	AlternatingFunction<typename std::forward_list<AsyncCalculator<T,N>>::iterator> m_alternator;
public:
	typedef typename T::result_type result_type;
	AlternatingAsyncs(){
		for(int i=threads; i!=0; --i){
			m_asyncs.emplace_front(m_funcs[i-1]);
			m_asyncs.front().start();
		}
		m_alternator.start(m_asyncs.begin(),m_asyncs.end());
	}
	result_type operator()(){return m_alternator();}
};


class SecondsCounter
{
	const std::chrono::steady_clock::time_point start;
	double& output;
public:
	SecondsCounter(double& output):start(std::chrono::steady_clock::now()),output(output){}
	~SecondsCounter(){output=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()/1000.0;}
};

#endif
