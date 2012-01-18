#ifndef MASTERMIND_ALIGNED_ALLOCATOR_HPP
#define MASTERMIND_ALIGNED_ALLOCATOR_HPP

#include <malloc.h>
#include <limits>

template <class T, size_t Alignment>
struct aligned_allocator 
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
	aligned_allocator(const aligned_allocator&) throw() { }
	template <class U>
	aligned_allocator(const aligned_allocator<U,Alignment>&) throw() { }
	~aligned_allocator() throw() { }

	pointer address(reference value) const { return &value; }
	const_pointer address(const_reference value) const { return &value; }

	pointer allocate(size_type n, const_pointer hint = 0)
	{
		pointer p = static_cast<pointer>(_aligned_malloc(n*sizeof(T), Alignment));
		if (!p)
			throw std::bad_alloc();
		return p;
	}

	void deallocate(pointer p, size_type n) 
	{
		_aligned_free(static_cast<void*>(p));
	}

	size_type max_size () const throw()
	{
		return std::numeric_limits<size_type>::max();
	}

	void construct(pointer p, const T &value) 
	{
		new (static_cast<void*>(p)) T(value);
	}
   
	void destroy(pointer p) 
	{       
		p->~T();
	}
};

template <class T1, size_t A1, class T2, size_t A2>
bool operator == (const aligned_allocator<T1,A1> &, const aligned_allocator<T2,A2> &)
{
	return true;
}

template <class T1, size_t A1, class T2, size_t A2>
bool operator != (const aligned_allocator<T1,A1> &, const aligned_allocator<T2,A2> &)
{
	return false;
}

#endif // MASTERMIND_ALIGNED_ALLOCATOR_HPP
