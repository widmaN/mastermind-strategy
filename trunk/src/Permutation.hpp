// Specialized permutation for codeword. Peg permutation and color permutation.

#ifndef MASTERMIND_PERMUTATION_HPP
#define MASTERMIND_PERMUTATION_HPP

#include <iostream>
#include <cassert>

namespace Mastermind {

template <class T, T MaxSize>
struct permutation
{
	T _perm[MaxSize];
	T _size;

	permutation() : _size(0) { }

	permutation(T size) : _size(size)
	{
		for (T i = 0; i < _size; i++)
			_perm[i] = i;
	}

	permutation(T size, T init_value) : _size(size)
	{
		for (T i = 0; i < _size; i++)
			_perm[i] = init_value;
	}

	T* begin() { return _perm; }
	T* end() { return _perm + _size; }

	T operator[](size_t index) const { return _perm[index]; }
	T& operator[](size_t index) { return _perm[index]; }
};

template <class T, T MaxSize>
std::ostream& operator << (std::ostream& os, const permutation<T,MaxSize> &p) 
{
	os << "[";
	for (T i = 0; i < p._size; ++i)
	{
		if (i > 0)
			os << ' ';
		if (p._perm[i] < 0)
			os << '*';
		else
			os << (size_t)p._perm[i];
	}
	os << "]";
	return os;
}

#if 0
template <size_t MaxSize, class Iter1, class Iter2, class Func>
void iterate_permutations_recursion(
	Iter1 from, Iter2 to, size_t n, size_t r, size_t level,
	std::bitset<MaxSize> &mask, Func f)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (!mask[i])
		{
			if (level == r - 1)
			{
				to[level] = from[i];
				f();
			}
			else
			{
				mask[i] = true;
				to[level] = from[i];
				iterate_permutations_recursion(from, to, n, r, level+1, mask, f);
				mask[i] = false;
			}
		}
	}
}

template <size_t MaxSize, class Iter1, class Iter2, class Func>
void iterate_permutations(
	Iter1 from_first, Iter1 from_last, 
	Iter2 to_first, Iter2 to_last,
	Func f)
{
	size_t n = from_last - from_first;
	size_t r = to_last - to_first;
	assert(n >= r && r >= 0 && MaxSize >= n);

	if (r > 0)
	{
		std::bitset<MaxSize> mask;
		iterate_permutations_recursion(from_first, to_first, n, r, 0, mask, f);
	}
	else
	{
		assert(0);
	}
}
#endif

template <size_t MaxSize, class Iter1, class Iter2, class Func>
void generate_permutations_recursion(
	Iter1 from, Iter2 to, 
	size_t n, size_t r, size_t level,
	std::bitset<MaxSize> &mask, Func f)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (!mask[i])
		{
			if (level == r - 1)
			{
				to[level] = from[i];
				f();
			}
			else
			{
				mask[i] = true;
				to[level] = from[i];
				generate_permutations_recursion(from, to, n, r, level+1, mask, f);
				mask[i] = false;
			}
		}
	}
}

/// Generates all permutatations of choosing @c r objects from @c n 
/// objects. The objects to choose from is supplied in 
/// <code>[from_first,from_last)</code>. The objects chosen are
/// stored in <code>[to_first,to_last)</code>. 
///
/// For each permutation generated, the callback function @c f
/// is invoked with no parameters. If <code>r = 0</code>, @c f
/// is invoked once.
///
/// @c MaxSize specifies an upper bound of @c n.
template <size_t MaxSize, class Iter1, class Iter2, class Func>
void generate_permutations(
	Iter1 from_first, Iter1 from_last, 
	Iter2 to_first, Iter2 to_last,
	Func f)
{
	size_t n = from_last - from_first;
	size_t r = to_last - to_first;
	assert(n >= r && r >= 0 && MaxSize >= n);

	if (r == 0)
	{
		f();
	}
	else
	{
		std::bitset<MaxSize> mask;
		generate_permutations_recursion(from_first, to_first, n, r, 0, mask, f);
	}
}

} // namespace Mastermind

#endif // MASTERMIND_PERMUTATION_HPP
