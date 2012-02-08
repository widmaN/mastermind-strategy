#ifndef MASTERMIND_FEEDBACK_HPP
#define MASTERMIND_FEEDBACK_HPP

#include <cassert>
#include <iostream>
#include <vector>
#include <utility>

#include "Rules.hpp"
#include "util/pool_allocator.hpp"
#include "util/frequency_table.hpp"

// Whether to store feedback in compact format
#ifndef MM_FEEDBACK_COMPACT
#define MM_FEEDBACK_COMPACT 1
#endif

namespace Mastermind 
{

/**
 * Represents the feedback from comparing two codewords.
 *
 * For a feedback of the form <code>xAyB</code>, this class stores 
 * the pair <code>(x,y)</code> and name it as <code>(nA,nB)</code>. 
 * To reduce memory usage, the feedback is stored in <i>packed</i> 
 * form, which takes up exactly one byte, as described below.
 *
 * There are two formats to pack a feedback: the standard format
 * and the compact format. In the standard format, a feedback takes
 * up <code>MM_FEEDBACK_BITS</code> bits (6 by default).
 * Bits 0-2 stores <code>nB</code>, and bits 3-5 stores 
 * <code>nA</code>. To pack a feedback in the standard format, use
 * the formula
 *    <code>x = (nA << MM_FEEDBACK_ASHIFT) | nB</code>. 
 * To unpack a feedback, use the formula 
 *    <code>nA = x >> MM_FEEDBACK_ASHIFT</code> and
 *    <code>nB = x & MM_FEEDBACK_BMASK</code>.
 * The standard format is the default format to store a feedback.
 *
 * In the compact format, the set of all distinct feedback values 
 * is arranged in the shape of a triangle, and a particular feedback
 * <code>(nA,nB)</code> is mapped to its index in this triangle.
 * The arragement is as follows:
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
 * To pack a feedback <code>(nA,nB)</code> in compact form, first  
 * compute <code>nAB=nA+nB</code> and then use the following formula
 *   <code>x = nAB*(nAB+1)/2+nA</code>.
 * The largest packed value for a <code>p</code>-peg game is 
 * <code>p*(p+3)/2</code>. This means one-byte storage can represent
 * up to 21 pegs, which is far larger than the other constraints.
 *
 * To unpack a feedback in compact form, a loop is required. In 
 * practice we build a lookup table for this purpose.
 *
 * The compact format has better cache performance in certain operations,
 * but is not chosen as the default format because of its complexity. 
 * To choose it, define the <code>MM_FEEDBACK_COMPACT</code> macro in
 * this file or in the compiler option.
 */
class Feedback
{
	/// Packed value of the feedback. The special value <code>0xFF</code>
	/// is reserved to indicate an <i>empty</i> feedback.
	unsigned char m_value;

#if MM_FEEDBACK_COMPACT
	/// Lookup table that maps a byte value (in compact format) to a 
	/// feedback <code>(nA,nB)</code>.
	struct compact_format_unpacker
	{
		std::pair<int,int> table[0x100];
		compact_format_unpacker()
		{
			for (int i = 0; i < 256; ++i)
				table[i] = std::pair<int,int>(-1,-1);

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
	};
#endif

public:

	/// Packs a feedback into a byte value.
	static unsigned char pack(int nA, int nB)
	{
		assert(nA >= 0 && nA <= MM_MAX_PEGS);
		assert(nB >= 0 && nB <= MM_MAX_PEGS);
		assert(nA+nB >= 0 && nA+nB <= MM_MAX_PEGS);
#if MM_FEEDBACK_COMPACT
		int nAB = nA + nB;
		return (nAB+1)*nAB/2+nA;
#else
		return (nA << MM_FEEDBACK_ASHIFT) | nB;
#endif
	}

	/// Unpacks a feedback from a byte value.
	static std::pair<int,int> unpack(unsigned char value)
	{
#if MM_FEEDBACK_COMPACT
		static const compact_format_unpacker unpacker;
		return unpacker.table[value];
#else
		return std::pair<int,int>(
			value >> MM_FEEDBACK_ASHIFT,
			value & MM_FEEDBACK_BMASK);
#endif
	}

public:

	/// Creates an empty feedback.
	Feedback() : m_value(0xff) { }

	/// Creates a feedback from its internal representation.
	Feedback(unsigned char value) : m_value(value) { }

	/// Creates a feedback with the given <code>nA</code> and <code>nB</code>.
	Feedback(int nA, int nB) : m_value(pack(nA, nB)) { }

	/// Creates a feedback from a string of the form "1A2B".
	/// If the input string is invalid, the feedback is initialized with
	/// an empty value.
	Feedback(const char *s) : m_value(0xff)
	{
		if (s &&
			(s[0] >= '0' && s[0] <= '9') &&
			(s[1] == 'A' || s[1] == 'a') &&
			(s[2] >= '0' && s[2] <= '9') &&
			(s[3] == 'B' || s[3] == 'b') &&
			(s[4] == '\0')) 
		{
			m_value = pack(s[0]-'0', s[2]-'0');
		}
	}

	/// Sets the value of the feedback.
	void setValue(int nA, int nB) { 	m_value = pack(nA, nB); 	}

	/// Gets the internal representation of the feedback.
	unsigned char value() const { return m_value; }

	/// Tests whether the feedback is empty. 
	/// An empty feedback can be created by <code>Feedback::emptyValue()</code>.
	bool empty() const { return (m_value == 0xff); }

	/// Returns <code>nA</code>, the number of correct colors
	/// in the correct pegs.
	int nA() const { return unpack(m_value).first; 	}

	/// Returns <code>nB</code>, the number of correct colors
	/// in the wrong pegs.
	int nB() const { return unpack(m_value).second; }

	/// Tests whether this feedback is equal to another.
	///
	/// If both feedbacks are non-empty, then they are equal if they 
	/// have the same number of nA and nB. If one of the feedbacks
	/// is empty, then they are equal only if they are both empty.
	bool operator == (Feedback b) const { return (m_value == b.m_value); }

	/// Tests whether this feedback is not equal to another.
	///
	/// If both feedbacks are non-empty, then they are equal if they 
	/// have the same number of nA and nB. If one of the feedbacks
	/// is empty, then they are equal only if they are both empty.
	bool operator != (Feedback b) const { return (m_value != b.m_value); }

public:

	/// Returns an empty feedback value. 
	static Feedback emptyValue()
	{
		return Feedback(0xff);
	}

	/// Returns a feedback value corresponding to a perfect match.
	static Feedback perfectValue(int npegs)
	{
		return Feedback(npegs, 0);
	}

	static Feedback perfectValue(const CodewordRules &rules)
	{
		return Feedback(rules.pegs(), 0);
	}

	static unsigned char maxValue(int npegs)
	{
		return Feedback(npegs, 0).value();
	}

	static unsigned char maxValue(const CodewordRules &rules)
	{
		return perfectValue(rules).value();
	}

};

/// Outputs the feedback to a stream in the form "1A2B".
inline std::ostream& operator << (std::ostream &os, const Feedback &feedback)
{
	if (feedback.empty())
		return os << "(empty_feedback)";
	else
		return os << feedback.nA() << 'A' << feedback.nB() << 'B';
}

///////////////////////////////////////////////////////////////////////////
// Definition of FeedbackList.

typedef std::vector<Feedback,util::pool_allocator<Feedback>> FeedbackList;


///////////////////////////////////////////////////////////////////////////
// Definition of FeedbackFrequencyTable.

typedef util::frequency_table<Feedback,unsigned int,256> FeedbackFrequencyTable;


#if 0
FeedbackFrequencyTable frequency(const FeedbackList &feedbacks)
{
	FeedbackFrequencyTable table;
	countFrequencies(feedbacks.begin(), feedbacks.end(), table);
	return table;
}
#endif

} // namespace Mastermind

#endif // MASTERMIND_FEEDBACK_HPP
