#pragma once

#include <string>

#include "MMConfig.h"
#include "Frequency.h"

namespace Mastermind 
{
	class Codeword;
	class CodewordList;

	/// Represents the feedback from comparing two codewords.
	class Feedback
	{
	private:

		/// The internal representation of the feedback.
		///
		/// There are two formats to store a feedback: standard format
		/// and compact format. In either format, a feedback takes up one
		/// byte of storage. The format can be chosen at compile time by 
		/// the <code>MM_FEEDBACK_COMPACT</code> directive in 
		/// <code>MMConfig.h</code>.
		///
		/// In the standard format, a feedback takes up 
		/// <code>MM_FEEDBACK_BITS</code> bits (6 by default).
		/// Bits 0-2 stores <code>nB</code>, and bits 3-6 stores
		/// <code>nA</code>. To encode a feedback, use the formula
		/// <code>x = (nA << MM_FEEDBACK_ASHIFT) | nB</code>. To decode 
		/// a feedback, use the formula <code>nA = x >> MM_FEEDBACK_ASHIFT</code>
		/// and <code>nB = x & MM_FEEDBACK_BMASK</code>.
		///
		/// In the compact format, the pair <code>(nA, nB)</code> is mapped
		/// to an ordinal number. The mapping is done by arranging all 
		/// possible feedbacks in the following order:
		/// <pre>
		/// 0A0B
		/// 0A1B  1A0B
		/// 0A2B  1A1B  2A0B
		/// 0A3B  1A2B  2A1B  3A0B
		/// </pre>
		/// To encode a feedback, use the formula 
		/// <code>x=nAB*(nAB+1)/2+nA</code> where <code>nAB=nA+nB</code>.
		/// To decode a feedback, you need a loop.
		unsigned char m_value;

	public:

		/// Creates an empty feedback.
		Feedback() : m_value(0xff) { }

		/// Creates a feedback from its internal representation.
		Feedback(unsigned char value) : m_value(value) { }

		/// Creates a feedback with the given <code>nA</code> and <code>nB</code>.
		Feedback(int nA, int nB);

		/// Sets the value of the feedback.
		void SetValue(int nA, int nB);

		/// Gets the internal representation of the feedback.
		unsigned char GetValue() const { return m_value; }

		/// Tests whether the feedback is empty. 
		/// An empty feedback can be created either by calling the constructor
		/// with no parameters, or through the static call <code>Feedback::Empty()</code>.
		bool IsEmpty() const { return (m_value == 0xff); }

		/// Returns <code>nA</code>, the number of correct colors
		/// in the correct pegs.
		int GetExact() const;

		/// Returns <code>nB</code>, the number of correct colors
		/// in the wrong pegs.
		int GetCommon() const;

		/// Converts the feedback to a string.
		/// The format is "1A2B".
		std::string ToString() const;

		/// Tests whether this feedback is equal to another.
		///
		/// If both feedbacks are non-empty, then they are equal if they 
		/// have the same number of nA and nB. If one of the feedbacks
		/// is empty, then they are equal only if they are both empty.
		bool operator == (const Feedback b) const { return (m_value == b.m_value); }

		/// Tests whether this feedback is not equal to another.
		///
		/// If both feedbacks are non-empty, then they are equal if they 
		/// have the same number of nA and nB. If one of the feedbacks
		/// is empty, then they are equal only if they are both empty.
		bool operator != (const Feedback b) const { return (m_value != b.m_value); }

	public:

		/// Returns an empty feedback value. 
		/// This is equivalent to calling the constructor with no parameter.
		static Feedback Empty()
		{
			return Feedback();
		}

		/// Returns a feedback value corresponding to a perfect match.
		static Feedback Perfect(int npegs)
		{
			return Feedback(npegs, 0);
		}

		/// Parses feedback from a string, in the form of "1A2B".
		/// If the input string is invalid, <code>Feedback::Empty()</code> 
		/// is returned.
		static Feedback Parse(const char *s);

	};

	/// Represents an array of feedbacks.
	class FeedbackList
	{
	private:

		/// Pointer to an array of feedback values.
		unsigned char *m_values;

		/// Number of elements in the feedback list.
		int m_count;

		/// Maximum feedback value in this list.
		unsigned char m_maxfb;

	public:

		/// Creates a feedback list by allocating the necessary amount of
		/// memory without filling the values.
		FeedbackList(int count, int pegs);

		/// Creates a feedback list by attaching to a pre-allocated chunk
		/// of memory.
		FeedbackList(unsigned char *values, int count, int pegs);

		/// Creates a feedback list as the result of comparing a particular
		/// codeword to a list of codewords.
		FeedbackList(const Codeword &guess, const CodewordList &secrets);

		/// Destructor.
		~FeedbackList();

		/// Gets a pointer to the internal feedback value array.
		unsigned char * GetData() { return m_values; }

		/// Gets a const pointer to the internal feedback value array.
		const unsigned char * GetData() const { return m_values; }

		/// Gets the number of elements in the feedback list.
		int GetCount() const { return m_count; }

		/// Gets the feedback value at a given index.
		Feedback operator [] (int index) const;

		/// Gets the maximum feedback value contained in this list.
		unsigned char GetMaxFeedbackValue() const { return m_maxfb; }

	};

	class FeedbackFrequencyTable
	{
	private:
		unsigned int m_freq[256];
		unsigned char m_maxfb;

	public:
		FeedbackFrequencyTable(unsigned char maxfb)
		{
			m_maxfb = maxfb;
		}

		FeedbackFrequencyTable(const FeedbackList &fblist);

		int GetMaxFeedbackValue() const { return m_maxfb; }

		unsigned int * GetData() { return m_freq; }

		unsigned int operator [] (Feedback fb) const;

		/// Gets the number of feedbacks with frequency greater than zero.
		int GetPartitionCount() const;

		/// Gets the maximum frequency value.
		unsigned int GetMaximum() const;

		/// Computes the sum of squares of the frequency values.
		unsigned int GetSumOfSquares() const;

		/// Computes the entropy of the feedback frequencies.
		double GetModifiedEntropy() const;

		/// Displays the feedback frequencies in the console.
		void DebugPrint() const;

	};



}