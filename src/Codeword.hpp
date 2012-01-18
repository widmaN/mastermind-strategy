#ifndef MASTERMIND_CODEWORD_HPP
#define MASTERMIND_CODEWORD_HPP

#include <iostream>
#include <memory>
#include <emmintrin.h>
#include <algorithm>
#include "MMConfig.h"

namespace Mastermind {

/// Represents a codeword.
/// For performance reason, a codeword is interchangable with __m128i.
// __declspec(align(16)) 
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
		memset(_counter, 0, sizeof(_counter));
		memset(_digit, -1, sizeof(_digit));
	}

	/// Creates a codeword from its internal representation.
	Codeword(__m128i value) : _value(value) { }

	/// Gets the internal representation of the codeword.
	__m128i value() const { 	return _value; }

	/// Tests whether the codeword is empty.
	bool empty() const { return _digit[0] == 0xFF; }

	/// Returns the color on a given peg.
	unsigned char operator [] (int peg) const { return _digit[peg]; }

	/// Tests whether the codeword contains any color more than once.
	bool repeated() const
	{
		return std::any_of(&_counter[0], &_counter[MM_MAX_COLORS], 
			[](unsigned char c) -> bool { return c > 1; });
	}

	/// Returns the number of pegs in the codeword.
	int pegs() const;

	/// Tests whether two codewords are equal.
	static friend bool operator == (const Codeword &a, const Codeword &b)
	{
		return memcmp(&a, &b, sizeof(Codeword)) == 0;
	}

#if 0
	/// Converts the codeword to a string.
	std::string ToString() const;

	/// Compares this codeword to another codeword. 
	/// Returns the feedback.
	Feedback CompareTo(const Codeword& guess) const;

	// Compares this codeword to a codeword list. 
	// @return A list of feedbacks.
	//void CompareTo(const CodewordList& list, FeedbackList& fbl) const;

	/// Returns a 16-bit mask of digits present in the codeword.
	unsigned short GetDigitMask() const;	
#endif

	/// Returns an empty codeword. This function has the same effect as
	// calling the constructor with no parameter.
	static Codeword Empty()
	{
		return Codeword();
	}

#if 0
	/// Parses codeword from a string, conforming to given rules.
	/// If the input text is invalid or is not conformant to the given rules,
	/// <code>Codeword::Empty()</code> is returned.
	static Codeword Parse(
		/// The string to parse
		const char *text,
		/// The rules to apply
		const CodewordRules &rules);
#endif

#if 0
	void* operator new (size_t size)
	{
		std::cout << "new(" << size << ")" << std::endl;
		return ::operator new(size);
	}

	void* operator new[](size_t size)
	{
		std::cout << "new[](" << size << ")" << std::endl;
		return ::operator new(size);
	}
#endif
};

/// Outputs a codeword to a stream.
std::ostream& operator << (std::ostream &os, const Codeword &c);

} // namespace Mastermind

#endif // MASTERMIND_CODEWORD_HPP
