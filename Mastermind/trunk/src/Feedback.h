#pragma once

#include <stdio.h>
#include <string>
#include <cassert>
#include <memory.h>

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
		// The high nibble contains <code>nA</code>, i.e. the number of
		// digits in the right places. The low nibble contains <code>nB</code>,
		// i.e. the number of digits in wrong places.
		unsigned char value;

	public:

		/// Creates an empty feedback.
		Feedback() : value(0xff) { }

		/// Creates the feedback from a BYTE. 
		/// The BYTE is taken as the internal presentation of the feedback directly.
		Feedback(unsigned char v) : value(v) { }

		/// Sets the content of the feedback.
		void SetValue(int nA, int nB)
		{
			assert(nA >= 0 && nA <= MM_MAX_PEGS);
			assert(nB >= 0 && nB <= MM_MAX_PEGS);
#if MM_FEEDBACK_COMPACT
			int nAB = nA + nB;
			value = (nAB+1)*nAB/2+nA;
#else
			value = (nA << MM_FEEDBACK_ASHIFT) | nB;
#endif
		}

		/// Creates the feedback with the given <code>nA</code> and <code>nB</code>.
		Feedback(int nA, int nB)
		{
			SetValue(nA, nB);
		}

		/// Gets the internal BYTE presentation of the feedback.
		unsigned char GetValue() const { return value; }

		/// Tests whether the feedback is empty. 
		/// An empty feedback can be created either by calling the constructor
		/// with no parameters, or through the static call <code>Feedback::Empty()</code>.
		bool IsEmpty() const
		{
			return (value == 0xff);
		}

		/// Returns <code>nA</code>, the number of correct digits (colors) 
		/// in the correct places (pegs).
		int GetExact() const
		{
#if MM_FEEDBACK_COMPACT
			int k = 0;
			for (int nAB = 0; nAB <= MM_MAX_PEGS; nAB++) {
				if (value >= k && value <= (k+nAB)) {
					return (value - k);
				}
				k += (nAB + 1);
			}
			assert(0);
			return 0;
#else
			return (int)(value >> MM_FEEDBACK_ASHIFT);
#endif
		}

		/// Returns <code>nB</code>, the number of correct digits (colors)
		/// in the wrong places (pegs).
		int GetCommon() const
		{
#if MM_FEEDBACK_COMPACT
			int k = 0;
			for (int nAB = 0; nAB <= MM_MAX_PEGS; nAB++) {
				if (value >= k && value <= (k+nAB)) {
					return (nAB - (value - k));
				}
				k += (nAB + 1);
			}
			assert(0);
			return 0;
#else
			return (int)(value & MM_FEEDBACK_BMASK);
#endif
		}

		/// Converts the feedback to a string, in the form of "1A2B".
		std::string ToString() const
		{
			char s[5];
			s[0] = '0' + GetExact();
			s[1] = 'A';
			s[2] = '0' + GetCommon();
			s[3] = 'B';
			s[4] = '\0';
			return s;
		}

	public:

		/// Returns an empty feedback value. 
		/// This is equivalent to calling the constructor with no parameter.
		static Feedback Empty()
		{
			return Feedback();
		}

		/// Parses feedback from a string, in the form of "1A2B".
		/// If the input string is invalid, <code>Feedback::Empty()</code> 
		/// is returned.
		static Feedback Parse(const char *s)
		{
			assert(s != NULL);
			if ((s[0] >= '0' && s[0] <= '9') &&
				(s[1] == 'A' || s[1] == 'a') &&
				(s[2] >= '0' && s[2] <= '9') &&
				(s[3] == 'B' || s[3] == 'b') &&
				(s[4] == '\0')) {
				return Feedback(s[0] - '0', s[2] - '0');
			} 
			return Feedback::Empty();
		}

		/// Parses feedback from a string, in the form of "1A2B".
		/// If the input string is invalid, <code>Feedback::Empty()</code> 
		/// is returned.
		static Feedback Parse(const std::string &s)
		{
			return Parse(s.c_str());
		}

		/// Tests whether two feedbacks are equal.
		static friend bool operator == (Feedback a, Feedback b) 
		{ 
			return (a.value == b.value); 
		}

		/// Tests whether two feedbacks are unequal.
		static friend bool operator != (Feedback a, Feedback b) 
		{ 
			return (a.value != b.value); 
		}
	};

	class FeedbackList;

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

		int GetMaxFeedbackValue() const 
		{
			return m_maxfb;
		}

		/*
		void Clear()
		{
			memset(m_freq, 0, sizeof(unsigned int)*((int)m_maxfb+1));
		}
		*/

		unsigned int *GetData() { return m_freq; }

		unsigned int operator [] (Feedback fb) const 
		{
			unsigned char k = fb.GetValue();
			assert(k >= 0 && k <= m_maxfb);
			return m_freq[k]; 
		}

		/// Gets the number of feedbacks with frequency greater than zero.
		int GetPartitionCount() const;

		/// Gets the maximum frequency value.
		unsigned int GetMaximum() const;

		/// Computes the sum of squares of the frequency values.
		unsigned int GetSumOfSquares() const;

		/// Computes the entropy of the feedback frequencies.
		float GetModifiedEntropy() const;

		/// Displays the feedback frequencies in the console.
		void DebugPrint() const;

	};

	class FeedbackList
	{
	private:
		unsigned char *m_values;
		int m_count;
		unsigned char m_maxfb;

	public:
		FeedbackList(int count, int pegs);

		FeedbackList(unsigned char *values, int count, int pegs)
		{
			m_values = values;
			m_count = count;
			m_maxfb = Feedback(pegs, 0).GetValue();
		}

		FeedbackList(const Codeword &guess, const CodewordList &secrets);

		~FeedbackList();

		unsigned char * GetData() { return m_values; }

		const unsigned char * GetData() const { return m_values; }

		int GetCount() const { return m_count; }

		Feedback GetAt(int index) const
		{
			assert(index >= 0 && index < m_count);
			return Feedback(m_values[index]);
		}

		Feedback operator [] (int index) const
		{
			return GetAt(index);
		}

		/*
		const FeedbackFrequencyTable* CountFrequencies()
		{
			CountFrequenciesImpl->Run(this->m_values, this->m_count, m_freq.GetData(), m_maxfb);
		}
		*/
		unsigned char GetMaxFeedbackValue() const
		{
			return m_maxfb;
		}

	};

}