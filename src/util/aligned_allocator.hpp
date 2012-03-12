/**
 * @defgroup AlignedAllocator Aligned Allocator
 * @ingroup util
 */

#ifndef UTILITIES_ALIGNED_ALLOCATOR_HPP
#define UTILITIES_ALIGNED_ALLOCATOR_HPP

#include <malloc.h>
//#include <limits>
#include <memory>

namespace util {

/**
 * STL-compliant allocator that allocates aligned memory.
 * @ingroup AlignedAllocator
 */
template <class T, size_t Alignment>
struct aligned_allocator 
	: public std::allocator<T> // Inherit construct(), destruct() etc.
{
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;
	typedef T*        pointer;
	typedef const T*  const_pointer;
	typedef T&        reference;
	typedef const T&  const_reference;
	typedef T         value_type;

	template <class U>
	struct rebind { 	typedef aligned_allocator<U,Alignment> other; };

	aligned_allocator() throw() { }
	aligned_allocator(const aligned_allocator& other) throw()
		: std::allocator<T>(other) { }
	template <class U>
	aligned_allocator(const aligned_allocator<U,Alignment>&) throw() { }
	~aligned_allocator() throw() { }

	pointer allocate(size_type n)
	{
		return allocate(n, const_pointer(0));
	}

	pointer allocate(size_type n, const_pointer /* hint */)
	{
		void *p;
#ifndef _WIN32
		if (posix_memalign(&p, Alignment, n*sizeof(T)) != 0)
			p = NULL;
#else
		p = _aligned_malloc(n*sizeof(T), Alignment);
#endif
		if (!p)
			throw std::bad_alloc();
		return static_cast<pointer>(p);
	}

	void deallocate(pointer p, size_type /* n */)
	{
#ifndef _WIN32
		free(p);
#else
		_aligned_free(p);
#endif
	}

#if 0
	pointer address(reference value) const { return &value; }
	const_pointer address(const_reference value) const { return &value; }

	size_type max_size () const throw()
	{
		return std::numeric_limits<size_type>::max();
	}

//	typename std::enable_if<std::is_default_constructible<T>::value,void>::type
	void construct(pointer p)
	{
		//std::allocator<int> a(*this);
		::new (static_cast<void*>(p)) T();
	}

	void construct(pointer p, const T &value)
	{
		new (static_cast<void*>(p)) T(value);
	}

	void destroy(pointer p)
	{
		p->~T();
	}
#endif
};

/**
 * Checks whether two aligned allocators are equal.
 * Two allocators are equal if the memory allocated using one allocator
 * can be deallocated by the other.
 * @returns This function always returns <code>true</code>.
 * @ingroup AlignedAllocator
 */
template <class T1, size_t A1, class T2, size_t A2>
bool operator == (const aligned_allocator<T1,A1> &, const aligned_allocator<T2,A2> &)
{
	return true;
}

/**
 * Checks whether two aligned allocators are not equal.
 * Two allocators are equal if the memory allocated using one allocator
 * can be deallocated by the other.
 * @returns This function always returns <code>false</code>.
 * @ingroup AlignedAllocator
 */
template <class T1, size_t A1, class T2, size_t A2>
bool operator != (const aligned_allocator<T1,A1> &, const aligned_allocator<T2,A2> &)
{
	return false;
}

} // namespace util

#endif // UTILITIES_ALIGNED_ALLOCATOR_HPP
