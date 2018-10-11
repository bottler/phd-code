#ifndef JRARENA_H
#define JRARENA_H

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <vector>

//This is a stripped-down version of jrarena.h which provides a C++98 compatible HeapArena

//However, if the compiler doesn't use move semantics, then you cannot use more blocks 
//than space is initially reserved for in m_blocks, because newBlock will not cope with a reallocation.

//look here
//http://www.codeguru.com/cpp/article.php/c18503/C-Programming-Stack-Allocators-for-STL-Containers.htm
//http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx
class HeapArena
{
	static const size_t s_alloc_max = 1 << 31, s_blocksize_max = 1 << 28, unit = 8;
	size_t m_offset, m_blockSize;
	typedef double Unit;
	std::vector<std::vector<Unit> > m_blocks;
	void newBlock()
	{
		m_blocks.push_back(std::vector<Unit>());
		m_blocks.back().assign(m_blockSize, Unit());
		m_offset = 0;
	}
	HeapArena(const HeapArena&);
	void operator=(const HeapArena &);
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


//JRAlloc allocates from get() on its Source (which it's constructed from) and never deallocates.
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
