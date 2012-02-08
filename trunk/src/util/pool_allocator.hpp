#ifndef UTILITIES_POOL_ALLOCATOR_HPP
#define UTILITIES_POOL_ALLOCATOR_HPP

#include <memory>

namespace util {

/**
 * Simplistic pool allocator that only maintains one chunk in the pool.
 *
 * Upon allocation, if the chunk is free and bigger than the requested
 * size, return that chunk. If the chunk is free but smaller than the
 * requested size, enlarge the chunk. If the chunk is in use, allocate
 * a new one.
 *
 * This pool allocator is useful in a scenario where a large chunk is
 * frequently allocated and freed. This is typical for FeedbackList.
 *
 * This allocator is NOT thread-safe. Therefore, it must not be used
 * in multi-threaded applications.
 */
template <class T, class Alloc=std::allocator<T>>
struct pool_allocator
{
	typedef typename Alloc::size_type       size_type;
	typedef typename Alloc::difference_type difference_type;
	typedef typename Alloc::pointer         pointer;
	typedef typename Alloc::const_pointer   const_pointer;
	typedef typename Alloc::reference       reference;
	typedef typename Alloc::const_reference const_reference;
	typedef typename Alloc::value_type      value_type;

	template <class U>
	struct rebind
	{
		typedef pool_allocator<U, typename Alloc::template rebind<U>::other> other;
	};

	pool_allocator() throw() { }
	pool_allocator(const pool_allocator&) throw() { }
	template <class U, class Alloc2>
	pool_allocator(const pool_allocator<U,Alloc2>&) throw() { }
	~pool_allocator() throw() { }

	pointer address(reference value) const
	{
		return alloc.address(value);
	}

	const_pointer address(const_reference value) const
	{
		return alloc.address(value);
	}

	// Allocates n elements. If the pool is free, return the chunk
	// in the pool. Otherwise, return a new chunk.
	pointer allocate(size_type n, const_pointer hint = 0)
	{
		if (!chunk_in_use)
		{
			if (chunk_size < n)
			{
				alloc.deallocate(chunk_ptr, chunk_size);
				chunk_size = n;
				chunk_ptr = alloc.allocate(chunk_size, hint);
			}
			chunk_in_use = true;
			return chunk_ptr;
		}
		else
		{
			return alloc.allocate(n, hint);
		}
	}

	void deallocate(pointer p, size_type n)
	{
		if (p != chunk_ptr)
		{
			alloc.deallocate(p, n);
		}
		else
		{
			chunk_in_use = false;
		}
	}

	size_type max_size () const throw()
	{
		return alloc.max_size();
	}

	void construct(pointer p)
	{
		alloc.construct(p);
	}

	void construct(pointer p, const T &value)
	{
		alloc.construct(p, value);
	}

	void destroy(pointer p)
	{
		alloc.destroy(p);
	}

public:

	Alloc alloc;

	static pointer chunk_ptr; // = 0;
	static size_t chunk_size; // = 0;
	static bool chunk_in_use; // = false;
};

template <class T, class A>
typename pool_allocator<T,A>::pointer pool_allocator<T,A>::chunk_ptr = 0;

template <class T, class A>
size_t pool_allocator<T,A>::chunk_size = 0;

template <class T, class A>
bool pool_allocator<T,A>::chunk_in_use = false;

template <class T, class A1, class A2>
bool operator == (const pool_allocator<T,A1> &a, const pool_allocator<T,A2> &b)
{
	return a.alloc == b.alloc;
}

template <class T1, class A1, class T2, class A2>
bool operator == (const pool_allocator<T1,A1> &, const pool_allocator<T2,A2> &)
{
	return false;
}

template <class T1, class A1, class T2, class A2>
bool operator != (const pool_allocator<T1,A1> &a, const pool_allocator<T2,A2> &b)
{
	return ! operator == (a, b);
}

} // namespace util

#endif // UTILITIES_POOL_ALLOCATOR_HPP
