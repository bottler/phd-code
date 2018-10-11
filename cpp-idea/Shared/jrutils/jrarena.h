#ifndef JRARENA_H
#define JRARENA_H

#include <array>
//#include <scoped_allocator>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

//TODO: these objects are aligning to 8 bytes when they should really be checking max_align_t. Perhaps there should be a parameter for the alignment, to avoid gross wastage.

//look here
//http://www.codeguru.com/cpp/article.php/c18503/C-Programming-Stack-Allocators-for-STL-Containers.htm
//http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx
class HeapArena
{
	static const size_t s_alloc_max = 1 << 31, s_blocksize_max = 1 << 28, unit = 8;
	size_t m_offset, m_blockSize;
	typedef std::aligned_storage<unit>::type Unit;
	std::vector<std::vector<Unit> > m_blocks;
	void newBlock()
	{
		m_blocks.push_back(std::vector<Unit>());
		m_blocks.back().assign(m_blockSize, Unit());
		m_offset = 0;
	}
	HeapArena(const HeapArena&);
	void operator=(const HeapArena&);
public:
	HeapArena(size_t blockSize) :m_offset(0), m_blockSize(blockSize / unit){
		assert(blockSize<s_alloc_max);
		assert(blockSize >= unit);
		m_blocks.reserve(20);
		newBlock();
	}
	void* get(size_t bytes){
		size_t unitsToAllocate = bytes / 8 + (bytes % 8 == 0 ? 0 : 1);
		if (bytes >= s_alloc_max || bytes == 0 || unitsToAllocate>m_blockSize)
			return 0;
		if (m_blockSize - m_offset < unitsToAllocate)
			newBlock();
		size_t t = m_offset;
		m_offset += unitsToAllocate;
		return &m_blocks.back()[t];
	}
};

template<size_t size>
class StackArena
{
	static const size_t s_max = 1 << 30, unit = 8;
	static_assert(0 == size % unit, "must be a multiple of 8 bytes");
	static_assert(size<s_max, "too big");
	static_assert(size>0, "too small");
	typedef typename std::aligned_storage<unit>::type Unit;	
	std::array<Unit, size / unit> m_arr;
	size_t m_offset;
	StackArena(const StackArena<size>&);
	void operator=(const StackArena<size>&);
public:
	StackArena() :m_offset(0){}
	void* get(size_t bytes){
		size_t unitsToAllocate = bytes / unit + (bytes % unit == 0 ? 0 : 1);
		if (bytes >= s_max || bytes == 0 || unit * (m_offset + unitsToAllocate) >= size)
			return 0;
		size_t t = m_offset;
		m_offset += unitsToAllocate;
		return &m_arr[t];
	}
};

class StackThenHeapArena
{
	StackArena<2000> m_stack;
	HeapArena m_heap;
public:
	StackThenHeapArena() :m_heap(65536){}
	void* get(size_t bytes)
	{
		if (void* p = m_stack.get(bytes))
			return p;
		return m_heap.get(bytes);
	}
};

//alloc_traits not in VS2012, so this minimalistic stuff works not
//template<class T> class Zil{
//public:
//	typedef T value_type;
//	Zil(int i){}
//	template<class S> Zil(const Zil<S> o){}
//	T* allocate(std::size_t n){return 0;}
//	void deallocate(T* t, std::size_t n){}
//};
//
//template<typename T, typename Source> class All {
//public:
//	typedef T value_type;
//};
//
//void newAlloc()
//{
//	Zil<double> nild (2);
//	std::vector<double, Zil<double>> v(nild);
//}

//JRAlloc allocates from get() on its Source (which it's constructed from) and never deallocates.
//TODO: add noexcept to many of these functions
//TODO: I think == and != should be templates able to take JRAlloc<U,Source>?
//TODO: safer to throw bad_array_new_length not bad_alloc if allocate given a much too big number.
//TODO: change construct() to allow move semantics
//STL at cppcon 2014 
template <typename T, typename Source> class JRAlloc {
public:
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	T * address(T& r) const { return &r; }
	const T * address(const T& s) const { return &s; }
	size_t max_size() const {
		return (static_cast<size_t>(0) - static_cast<size_t>(1)) / sizeof(T);
	}
	template <typename U> struct rebind {
		typedef JRAlloc<U, Source> other;
	};
	bool operator!=(const JRAlloc& other) const {
		return !(*this == other);
	}
	void construct(T * const p, const T& t) const {
		void * const pv = static_cast<void *>(p);
		new (pv)T(t);
	}
	void destroy(T * const p) const{ p->~T(); }
	bool operator==(const JRAlloc& other) const { return true; }//I can destroy for my friend
	JRAlloc(Source& s) :m_s(s) { }
	JRAlloc(const JRAlloc& s) :m_s(s.m_s){ }
	template<class U, class S> friend class JRAlloc;
	template <typename U> JRAlloc(const JRAlloc<U, Source>& s) :m_s(s.m_s) { }
	~JRAlloc() { }
	T * allocate(const size_t n) const {
	  static_assert(alignof(T)<=8, "I am not able to supply storage aligned for this type");
		if (n == 0)
			return NULL;
		if (n > max_size())
			throw std::length_error("JRAlloc<T>::allocate() - Integer overflow.");
		void * const pv = m_s.get(n * sizeof(T));
		if (pv == NULL)
			throw std::bad_alloc();
		return static_cast<T *>(pv);
	}
	void deallocate(T * const p, const size_t n) const {}
	template <typename U> T * allocate(const size_t n, const U * /* const hint */) const {
		return allocate(n);
	}
	JRAlloc& operator=(const JRAlloc& a){ assert(&m_s == &a.m_s); }//should not be necessary. need 4 scoped_allocator_adaptor
private:
	Source& m_s;
	//JRAlloc& operator=(const JRAlloc& a);
};

#endif
