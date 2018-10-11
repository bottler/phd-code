#ifndef JR_SPACEDVECTOR
#define JR_SPACEDVECTOR

#include <boost/align/aligned_allocator.hpp>
#include <vector>

// SpacedVector<T> behaves like a non-resizable vector<T>
// except that each element lives on its own cache line.

//This should do the right thing in a range-for loop:
// for (const auto T& : v)

//It should be future proof in the following sense: 
//if you have a standard library which supports over-aligned object
//then you can remove boost and just make V a normal vector,
// and then it is still a good solution.

constexpr size_t cacheLineSize=64;

template<typename T, size_t N = cacheLineSize>
class SpacedVector{
  
  constexpr static size_t N2 = (N > alignof(T) ? N : alignof(T));
public:
  struct alignas(N2) Datum{
    T value;
    operator T&() {return value;}
    operator const T&() const {return value;}
  };
  using V = std::vector<Datum,boost::alignment::aligned_allocator<Datum>>;
  using VI = typename V::iterator;
  using VCI = typename V::const_iterator;

  SpacedVector() {}
  SpacedVector(size_t size): m_data(size) {} //PODs potentially uninitialised
  SpacedVector(size_t size, T t): m_data(size, Datum{t}){}
  T& operator[](size_t i){return m_data[i];}
  const T& operator[] (size_t i) const {return m_data[i];}
  VI begin() {return m_data.begin();}
  VCI begin() const {return m_data.begin();}
  VI end() {return m_data.end();}
  VCI end() const {return m_data.end();}
  size_t size() const {return m_data.size();}
  T* data() {return &m_data[0].value;}
  const T* data() const {return &m_data[0].value;}
  void push_back(T& t){Datum d; d.value = t; m_data.push_back(d);}
private:
  V m_data;
};

#endif
