#ifndef MASTERMIND_CODEWORD_HPP
#define MASTERMIND_CODEWORD_HPP

#include <iostream>
#include <cstring>
#include <cstdint>
#include <emmintrin.h>
#include <algorithm>
#include <cassert>

#include "util/intrinsic.hpp"
#include "Rules.hpp"

namespace Mastermind {

/// Represents a codeword.
/// For performance reason, a codeword is interchangable with __m128i.
/// @ingroup type
class Codeword /* __declspec(align(16)) */
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

	/// Type of the internal representation of a codeword.
	typedef __m128i value_type;

	/// Creates an empty codeword.
	Codeword()
	{
		std::memset(_counter, 0, sizeof(_counter));
		std::memset(_digit, -1, sizeof(_digit));
	}

	/// Creates a codeword from its internal representation.
	Codeword(value_type value) : _value(value) { }

	/// Gets the internal representation of the codeword.
	value_type value() const { return _value; }

	/// Tests whether the codeword is empty.
	bool empty() const { return _digit[0] == 0xFF; }

	/// Returns the color on a given peg.
	unsigned char operator [] (int peg) const 
	{
		assert(peg >= 0 && peg < MM_MAX_PEGS);
		return _digit[peg]; 
	}

	/// Sets the color on a given peg.
	void set(int peg, int color)
	{
		assert(peg >= 0 && peg < MM_MAX_PEGS);
		assert(color == 0xFF || (color >= 0 && color < MM_MAX_COLORS));
		if (_digit[peg] != 0xFF)
			--_counter[_digit[peg]];
		if ((_digit[peg] = (unsigned char)color) != 0xFF)
			++_counter[color];
	}

	/// Returns the number of occurrences of a given color.
	int count(int color) const 
	{
		assert(color >= 0 && color < MM_MAX_COLORS);
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

	/// Tests whether two codewords are not equal.
	bool operator != (const Codeword &c) const { return ! operator == (c); }

	/// Returns an empty codeword.
	static Codeword emptyValue()
	{
		return Codeword();
	}

	typedef uint32_t compact_type;

	/// Packs a codeword into a 4-byte representation.
	compact_type pack() const
	{
		uint32_t w = 0xffffffff;
		for (int i = 0; i < MM_MAX_PEGS; i++) 
		{
			unsigned char d = _digit[i];
			if (d == 0xFF)
				break;
			w <<= 4;
			w |= d;
		}
		return w;
	}

	/// Unpacks a codeword from a 4-byte representation.
	static Codeword unpack(compact_type w)
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

#if 0
/// Utility class that computes the lexicographical index of a codeword.
/// For performance reason, we ALWAYS computes the index as if repetition
/// was allowed in the colors. This improves speed at the cost of a little
/// waste of memory.
class CodewordIndexer
{
	Rules _rules;
	int _weights[MM_MAX_COLORS];

public:

	CodewordIndexer(const Rules &rules) : _rules(rules)
	{
#if 0
		if (_rules.repeatable())
		{
#endif
			int w = 1;
			for (int i = _rules.pegs() - 1; i >= 0; --i)
			{
				_weights[i] = w;
				w *= _rules.colors();
			}
#if 0
		}
		else
		{
			int w = 1;
			for (int i = _rules.pegs() - 1; i >= 0; --i)
			{
				_weights[i] = w;
				w *= (_rules.colors() - i);
			}
		}
#endif
	}

	int size() const { return _weights[0] * _rules.colors(); }

	int operator()(const Codeword &c) const 
	{
#if 0
		if (_rules.repeatable())
		{
#endif
			int index = 0;
			for (int i = 0; i < _rules.pegs(); ++i)
				index += c[i] * _weights[i];
			return index;
#if 0
		}
		else
		{
			unsigned short bitmask = 0;
			int index = 0;
			for (int i = 0; i < _rules.pegs(); ++i)
			{
				int t = c[i] - util::intrinsic::pop_count(bitmask & ((1 << c[i]) - 1));
				index += t * _weights[i];
				bitmask |= (1 << c[i]);
			}
			return index;
		}
#endif
	}
};
#endif

} // namespace Mastermind

#endif // MASTERMIND_CODEWORD_HPP
