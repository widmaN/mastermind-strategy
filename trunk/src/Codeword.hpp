#ifndef MASTERMIND_CODEWORD_HPP
#define MASTERMIND_CODEWORD_HPP

#include <iostream>
#include <cstring>
#include <cstdint>
#include <emmintrin.h>
#include <algorithm>

#include "Rules.hpp"

namespace Mastermind {

/// Represents a codeword.
/// For performance reason, a codeword is interchangable with __m128i.
/* __declspec(align(16)) */
class Codeword
{
	// The internal representation of the codeword value.
	union
	{
		__m128i _value;
		struct
		{
			unsigned char _counter[MM_MAX_COLORS];
			unsigned char _digit[MM_MAX_PEGS];
		};
	};

public:

	/// Creates an empty codeword. An empty codeword contains no pegs.
	/// It can be used as a special value to indicate, for example,
	/// an error.
	Codeword()
	{
		std::memset(_counter, 0, sizeof(_counter));
		std::memset(_digit, -1, sizeof(_digit));
	}

	/// Creates a codeword from its internal representation.
	Codeword(__m128i value) : _value(value) { }

	/// Gets the internal representation of the codeword.
	__m128i value() const { 	return _value; }

	/// Tests whether the codeword is empty.
	bool empty() const { return _digit[0] == 0xFF; }

	/// Returns the color on a given peg.
	unsigned char operator [] (int peg) const { return _digit[peg]; }

	/// Sets the color on a given peg.
	void set(int peg, int color)
	{
		if (_digit[peg] != 0xFF)
			--_counter[_digit[peg]];
		if ((_digit[peg] = color) != 0xFF)
			++_counter[color];
	}

	/// Returns the number of occurrences of a given color.
	int count(int color) const 
	{
		return _counter[color];
	}

	/// Tests whether the codeword contains any color more than once.
	bool repeated() const
	{
		return std::any_of(&_counter[0], &_counter[MM_MAX_COLORS],
			[](unsigned char c) -> bool { return c > 1; });
	}

	/// Returns the number of pegs in the codeword.
	int pegs() const;

	/// Checks whether this codeword conforms to the supplied rules.
	bool valid(const Rules &rules) const;

	/// Tests whether two codewords are equal.
	bool operator == (const Codeword &c) const
	{
		//return a.value() == b.value();
		return memcmp(this, &c, sizeof(Codeword)) == 0;
	}

	/// Returns an empty codeword.
	static Codeword emptyValue()
	{
		return Codeword();
	}

	/// Packs a codeword into a 4-byte representation.
	static uint32_t pack(const Codeword &c)
	{
		uint32_t w = 0xffffffff;
		for (int i = 0; i < MM_MAX_PEGS; i++) 
		{
			unsigned char d = c[i];
			if (d == 0xFF)
				break;
			w <<= 4;
			w |= d;
		}
		return w;
	}

	/// Unpacks a codeword from a 4-byte representation.
	static Codeword unpack(uint32_t w)
	{
		Codeword c = emptyValue();
		int i = 0;
		for (int nibble = 7; nibble >= 0; --nibble)
		{
			unsigned char d = (w >> (nibble*4)) & 0xFF;
			if (d != 0xFF)
				c.set(i++, d);
		}
		return c;
	}
};

/// Outputs a codeword to a stream.
std::ostream& operator << (std::ostream &os, const Codeword &c);

/// Inputs a codeword from a stream. No rules are enforced.
std::istream& operator >> (std::istream &is, Codeword &c);


} // namespace Mastermind

#endif // MASTERMIND_CODEWORD_HPP
