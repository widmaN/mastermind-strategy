#ifndef MASTERMIND_FEEDBACK_HPP
#define MASTERMIND_FEEDBACK_HPP

#include <cassert>
#include <iostream>
#include <utility>
#include <algorithm>

#include "Rules.hpp"

namespace Mastermind 
{

/**
 * Represents the feedback from comparing two codewords.
 *
 * For a feedback of the form <code>xAyB</code>, this class stores 
 * the pair <code>(x,y)</code> and name it as <code>(nA,nB)</code>. 
 * To improve memory access locality, the feedback is internally  
 * represented by its ordinal position in an order described below.
 *
 * The set of all distinct feedback values can be arranged in the 
 * form of a triangle as follows:
 * <pre>
 * 0A0B  1A0B  2A0B  3A0B  4A0B
 * 0A1B  1A1B  2A1B  3A1B*
 * 0A2B  1A2B  2A2B
 * 0A3B  1A3B
 * 0A4B
 * </pre>
 * The arrangement looks like a Pythagoras triangle, in that the 
 * feedbacks in any diagonal have the same sum <code>nA+nB</code>.
 * A game with <code>p</code> pegs can have any feedback outcome
 * up to the <code>p</code>-th diagonal (starting from zero), 
 * except for the last feedback in the row marked with [*]. For
 * example, in a game with 4 pegs, an outcome can never be 3A1B.
 * It follows that the total number of possible feedbacks in a 
 * <code>p</code>-peg game is <code>(1+2+...+(p+1))-1</code>,
 * which is equal to <code>p*(p+3)/2</code>.
 *
 * To convert a feedback <code>(nA,nB)</code> to its ordinal position
 * in the above arrangement, first compute <code>nAB=nA+nB</code> and
 * then use the following formula
 *   <code>pos = nAB*(nAB+1)/2+nA</code>.
 * The largest feedback value for a <code>p</code>-peg game is 
 * <code>p*(p+3)/2</code>. This means 7-bits storage can represent
 * up to 14 pegs, which corresponds to a maximum ordinal index of 119.
 * The highest bit is reserved to indicate an error condition.
 *
 * To convert a feedback from its ordinal position, there is no 
 * simple formula. Therefore we build a lookup table for this purpose.
 *
 * @ingroup type
 */
class Feedback
{
	/// Ordinal position of the feedback. The special value @c -1
	/// is reserved to indicate an <i>empty</i> feedback.
	char _value;

	/// Lookup table that maps an ordinal position to a feedback.
	struct compact_format_unpacker
	{
		std::pair<int,int> table[129];
		compact_format_unpacker()
		{
			std::fill(table+0, table+129, std::pair<int,int>(-1,-1));

			for (int nAB = 0; nAB <= MM_MAX_PEGS; ++nAB)
			{
				for (int nA = 0; nA <= nAB; ++nA)
				{
					int nB = nAB - nA;
					int k = (nAB+1)*nAB/2+nA;
					table[k] = std::pair<int,int>(nA, nB);
				}
			}
		}

		static std::pair<int,int> unpack(char value)
		{
			static const compact_format_unpacker unpacker;
			return (value >= 0)? unpacker.table[value] : unpacker.table[128];
		}
	};

public:

	/// Creates an empty feedback.
	Feedback() : _value(-1) { }

	/// Creates a feedback from its ordinal position.
	explicit Feedback(size_t index) : _value((char)index) { }

	/// Creates a feedback with the given <code>nA</code> and <code>nB</code>.
	explicit Feedback(int nA, int nB)
	{
		assert(nA >= 0 && nA <= MM_MAX_PEGS);
		assert(nB >= 0 && nB <= MM_MAX_PEGS);
		assert(nA+nB >= 0 && nA+nB <= MM_MAX_PEGS);
		int nAB = nA + nB;
		_value = (char)((nAB+1)*nAB/2+nA);
	}

	/// Creates a feedback from a string of the form "1A2B". If the
	/// input string is invalid, the feedback is set to an empty value.
	explicit Feedback(const char *s) : _value(-1)
	{
		if (s &&
			(s[0] >= '0' && s[0] <= '9') &&
			(s[1] == 'A' || s[1] == 'a') &&
			(s[2] >= '0' && s[2] <= '9') &&
			(s[3] == 'B' || s[3] == 'b') &&
			(s[4] == '\0')) 
		{
			_value = Feedback(s[0]-'0', s[2]-'0')._value;
		}
	}

	/// Type of the internal representation of the feedback.
	typedef char value_type;

	/// Returns the internal representation of the feedback.
	value_type value() const { return _value; }

	/// Tests whether the feedback is empty. 
	bool empty() const { return _value < 0; }

	/// Returns <code>nA</code>, the number of correct colors
	/// in the correct pegs.
	int nA() const { return compact_format_unpacker::unpack(_value).first; }

	/// Returns <code>nB</code>, the number of correct colors
	/// in the wrong pegs.
	int nB() const { return compact_format_unpacker::unpack(_value).second; }

	/**
	 * Compact format of a feedback.
	 * In this format, bits 0-3 stores @c nB, and bits 4-7 stores 
	 * @c nA. To convert a feedback into the compact format, use
	 * the formula <code>x = (nA << 4) | nB</code>.
	 * To restore a feedback from compact format, use the formula 
	 * <code>nA = x >> 4</code> and <code>nB = x & 0x0F</code>.
	 */
	typedef unsigned char compact_type;

	/// Converts the feedback into compact form.
	compact_type pack() const 
	{
		std::pair<int,int> ab = compact_format_unpacker::unpack(_value);
		return (compact_type)((ab.first << 4) | ab.second);
	}

	/// Restores a feedback from compact form.
	/// @todo check malformed input
	static Feedback unpack(compact_type ab)
	{
		int nA = (ab >> 4), nB = (ab & 0x0F);
		return Feedback(nA, nB);
	}

	/// Returns the feedback for a perfect match under a given set 
	/// of rules.
	static Feedback perfectValue(const Rules &rules)
	{
		return Feedback(rules.pegs(), 0);
	}

	/// Returns the size of the set of distinct feedback values under
	/// a given set of rules. The practically impossible feedback
	/// <code>(p-1,1)</code> is included.
	static size_t size(const Rules &rules)
	{
		return (size_t)perfectValue(rules)._value + 1;
	}
};

/// Tests whether two feedbacks are equal.
/// @ingroup type
inline bool operator == (const Feedback &a, const Feedback &b)
{
	return a.value() == b.value();
}

/// Tests whether two feedbacks are not equal.
/// @ingroup type
inline bool operator != (const Feedback &a, const Feedback &b)
{
	return a.value() != b.value();
}

/// Outputs the feedback to a stream in the form "1A2B".
/// @ingroup type
inline std::ostream& operator << (std::ostream &os, const Feedback &feedback)
{
	if (feedback.empty())
		return os << "(empty_feedback)";
	else
		return os << feedback.nA() << 'A' << feedback.nB() << 'B';
}

} // namespace Mastermind

#endif // MASTERMIND_FEEDBACK_HPP
