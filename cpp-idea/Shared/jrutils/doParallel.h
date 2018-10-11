#ifndef JR_DOPARALLEL_H
#define JR_DOPARALLEL_H

#include<algorithm>
#include<atomic>
#include<vector>
#include<thread>

//calls fn on each of the ints 0, 1, ... nToDo-1 on background threads
//fn should take int and return void

//should separate output by 64 bytes - avoid cache contention on x86

template<typename T>
void doParallel(T&& fn, const int nToDo)
{
  int nThreads = std::min<int>(std::thread::hardware_concurrency(),nToDo);
  const bool useMainThread = true;
  //nThreads=1;
  //if(nThreads==0)
  //  cout<<"nothingToDo"<<endl;
  std::atomic<int> nextToDo{0};
  std::vector<std::thread> threads;
  auto task = [&]{
	int todo;
//	while((todo=nextToDo++)<nToDo)
	while((todo=nextToDo.fetch_add(1,std::memory_order_relaxed)) < nToDo)
	  fn(todo);
  };
  for(int i=0; i<nThreads-(useMainThread?1:0); ++i)
    threads.emplace_back(task);
  if(useMainThread)
    task();
  for(auto& thread : threads)
    thread.join();
}


#endif
