#ifndef MASTERMIND_CODEWORD_HPP
#define MASTERMIND_CODEWORD_HPP

#include <iostream>
#include <cstring>
#include <cstdint>
#include <emmintrin.h>
#include <algorithm>
#include <cassert>

#include "Rules.hpp"
#include "util/intrinsic.hpp"
#include "util/simd.hpp"

namespace Mastermind {

/// Represents a codeword.
/// For performance reason, a codeword is interchangable with __m128i.
/// @ingroup Codeword
class Codeword /* __declspec(align(16)) */
{
	// The internal representation of the codeword value.
	union
	{
		__m128i _value;
		struct
		{
			int8_t _counter[MM_MAX_COLORS];
			int8_t _digit[MM_MAX_PEGS];
		};
	};

public:

	/// Constant representing an 'empty' color.
	static const int EmptyColor = -1;
	
	/// Type of the internal representation of a codeword.
	typedef __m128i value_type;
	
	/// Creates an empty codeword.
	Codeword()
	{
		std::memset(_counter, 0, sizeof(_counter));
		std::memset(_digit, -1, sizeof(_digit));
	}

	/// Creates a codeword from its internal representation.
	/// No validation of arguments is performed, so the caller must ensure
	/// the arguments are valid.
	Codeword(value_type value) : _value(value) { }

	/// Gets the internal representation of the codeword.
	value_type value() const { return _value; }

	/// Tests whether the codeword is empty.
	bool empty() const { return _digit[0] < 0; }

	/// Tests whether the codeword is non-empty.
	operator void* () const { return empty()? 0 : (void*)this; }

	/// Tests whether the codeword is empty.
	bool operator ! () const { return empty(); }

	/// Returns the color on a given peg.
	int operator [] (int peg) const
	{
		assert(peg >= 0 && peg < MM_MAX_PEGS);
		return _digit[peg];
	}

	/// Sets the color on a given peg.
	void set(int peg, int color)
	{
		assert(peg >= 0 && peg < MM_MAX_PEGS);
		assert(color == -1 || (color >= 0 && color < MM_MAX_COLORS));
		if (_digit[peg] >= 0)
			--_counter[(int)_digit[peg]];
		if ((_digit[peg] = (char)color) >= 0)
			++_counter[color];
	}

	/// Returns the number of occurrences of a given color.
	int count(int color) const
	{
		assert(color >= 0 && color < MM_MAX_COLORS);
		return _counter[color];
	}

	/// Tests whether the codeword contains any color more than once.
	bool has_repetition() const
	{
#if 1
		int mask = util::simd::byte_mask(
			util::simd::simd_t<int8_t,16>(_value) > (int8_t)1);
		return (mask & ((1 << MM_MAX_COLORS) - 1)) != 0;
#else	
		for (int i = 0; i < MM_MAX_COLORS; ++i)
		{
			if (_counter[i] > 1)
				return true;
		}
		return false;
#endif
	}

#if 0
	/// Returns the number of pegs in the codeword.
	int pegs() const
	{
		int n = 0;
		for (int i = 0; i < MM_MAX_PEGS; ++i)
		{
			if (_digit[i] >= 0)
				++n;
		}
		return n;
	}
#endif

	/// Checks whether this codeword conforms to the supplied rules.
	bool conforming(const Rules &rules) const;

	/// Type of the packed value.
	typedef uint32_t compact_type;

	/// Packs a codeword into a 4-byte representation.
	compact_type pack() const
	{
		uint32_t w = 0xffffffff;
		for (int i = 0; i < MM_MAX_PEGS; i++)
		{
			char d = _digit[i];
			if (d < 0)
				break;
			w <<= 4;
			w |= d;
		}
		return w;
	}

	/// Unpacks a codeword from a 4-byte representation.
	static Codeword unpack(compact_type w)
	{
		Codeword c;
		int i = 0;
		for (int nibble = 7; nibble >= 0; --nibble)
		{
			char d = (w >> (nibble*4)) & 0xF;
			if (d != 0xF)
				c.set(i++, d);
		}
		return c;
	}
};

/// Tests whether two codewords are equal.
/// @ingroup Codeword
inline bool operator == (const Codeword &a, const Codeword &b)
{
	return memcmp(&a, &b, sizeof(Codeword)) == 0;
}

/// Tests whether two codewords are not equal.
/// @ingroup Codeword
inline bool operator != (const Codeword &a, const Codeword &b)
{
	return ! operator == (a, b);
}

/// Outputs a codeword to a stream.
/// @ingroup Codeword
std::ostream& operator << (std::ostream &os, const Codeword &c);

/// Inputs a codeword from a stream.
/// @ingroup Codeword
std::istream& operator >> (std::istream &is, Codeword &c);

} // namespace Mastermind

#endif // MASTERMIND_CODEWORD_HPP
