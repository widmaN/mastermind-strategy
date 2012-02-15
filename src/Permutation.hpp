#ifndef MASTERMIND_PERMUTATION_HPP
#define MASTERMIND_PERMUTATION_HPP

#include <iostream>
#include <cassert>
#include <bitset>
#include <emmintrin.h>
#include "Rules.hpp"
#include "Codeword.hpp"

#if 0
#include <tmmintrin.h>
#include "util/simd.hpp"

namespace util { namespace simd {
	
#define SIMD_REQUIRE(version)

SIMD_REQUIRE("SSSE3")
inline simd_t<uint8_t,16> 
shuffle(const simd_t<uint8_t,16> &a, const simd_t<int8_t,16> &perm)
{
	return _mm_shuffle_epi8(a, perm);
}

} } // namespace util::simd
#endif

namespace Mastermind {

/**
 * Permutation that reorders the pegs and remaps the colors in a codeword.
 *
 * A codeword permutation consists of two parts: peg permutation and color
 * permutation. Here we actually store the inverse of the peg permutation
 * because it's easier to compute a permuted codeword using that.
 */
struct CodewordPermutation
{
	Rules _rules;

	union
	{
		__m128i _perm;
		struct
		{
			char color[MM_MAX_COLORS];
			char peg[MM_MAX_PEGS];
		};
		struct 
		{
			char _bytes[16];
		};
	};

public:

	/// Creates an empty permutation, i.e. all elements are unmapped.
	CodewordPermutation(const Rules &rules)
		: _rules(rules), _perm(_mm_set1_epi8((char)-1)) { }

#if 0
	CodewordPermutation& operator = (const CodewordPermutation &p)
	{
		_rules = p._rules;
		_perm = p._perm;
		return (*this);
	}
#endif

#if 0
	/// Returns the inverse of the permutation.
	CodewordPermutation inverse() const 
	{
		CodewordPermutation ret(_rules);
		for (int i = 0; i < _rules.pegs(); ++i)
		{
			if (peg[i] >= 0)
				ret.peg[peg[i]] = i;
		}
		for (int i = 0; i < _rules.colors(); ++i)
		{
			if (color[i] >= 0)
				ret.color[color[i]] = i;
		}
		return ret;
	}
#endif

	/// Tests whether the permutation is fully specified.
	bool complete() const 
	{
		int unspecified = _mm_movemask_epi8(_perm);
		int color_mask = (1 << _rules.colors()) - 1;
		int peg_mask = (1 << _rules.pegs()) - 1;
		int mask = (peg_mask << MM_MAX_COLORS) | color_mask;
		return (unspecified & mask) == 0;
	}

	/// Tests whether the permutation is "almost fully specified", i.e.
	/// it is fully specified or at most one element is not mapped.
	/// However, in this case this element has only one choice.
	bool almost_complete() const 
	{
		int unspecified = _mm_movemask_epi8(_perm);
		int color_mask = (1 << _rules.colors()) - 1;
		int peg_mask = (1 << _rules.pegs()) - 1;
		int mask = (peg_mask << MM_MAX_COLORS) | color_mask;
		int test = (unspecified & mask);
		return (test & (test - 1)) == 0;
	}

	/// Returns the mapped value of a peg.
	//char peg(size_t i) const { return _peg[i]; }
	//char& peg(size_t i) { return _peg[i]; }

	/// Returns the mapped value of a color.
	//int color(int i) const { return _color[i]; }

	/// Returns a begin iterator to the peg permutation.
	//char* peg_begin() { return _peg + 0; }

	/// Returns an end iterator to the peg permutation.
	//char* peg_end() { return _peg + _rules.pegs(); }

	Codeword permute(const Codeword &w) const
	{
		Codeword ret;
		for (int i = 0; i < _rules.pegs(); ++i)
			ret.set(i, color[w[peg[i]]]);
		return ret;
	}

	Codeword permute_pegs(const Codeword &w) const
	{
		Codeword ret;
		for (int i = 0; i < _rules.pegs(); ++i)
			ret.set(i, w[peg[i]]);
		return ret;
	}

	Codeword permute_colors(const Codeword &w) const
	{
		Codeword ret;
		for (int i = 0; i < _rules.pegs(); ++i)
			ret.set(i, color[w[i]]);
		return ret;
	}

	friend std::ostream& operator << (
		std::ostream& os, const CodewordPermutation &p) 
	{
		// Output peg permutation.
		os << "(";
		for (int i = 0; i < p._rules.pegs(); ++i)
		{
			if (i > 0)
				os << ' ';
			if (p.peg[i] < 0)
				os << '*';
			else
				os << (size_t)p.peg[i];
		}
		os << ") o (";

		// Output color permutation.
		for (int i = 0; i < p._rules.colors(); ++i)
		{
			if (i > 0)
				os << ' ';
			if (p.color[i] < 0)
				os << '*';
			else
				os << (size_t)p.color[i];
		}
		os << ")";
		return os;
	}
};

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
