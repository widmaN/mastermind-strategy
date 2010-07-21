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
		/// The high nibble contains <code>nA</code>, i.e. the number of
		/// digits in the right places. The low nibble contains <code>nB</code>,
		/// i.e. the number of digits in wrong places.
		unsigned char value;

	public:

		/// Creates an empty feedback.
		Feedback() : value(0xff) { }

		/// Creates the feedback from a BYTE. 
		/// The BYTE is taken as the internal presentation of the feedback directly.
		Feedback(unsigned char v) : value(v) { }

		/// Sets the content of the feedback.
		void SetValue(int nexact, int ncommon)
		{
			assert(nexact >= 0 && nexact <= MM_MAX_PEGS);
			assert(ncommon >= 0 && ncommon <= MM_MAX_PEGS);
			value = ((nexact << (MM_FEEDBACK_BITS/2)) | ncommon) & ((1<<MM_FEEDBACK_BITS)-1);
		}

		/// Creates the feedback with the given <code>nA</code> and <code>nB</code>.
		Feedback(int nexact, int ncommon)
		{
			SetValue(nexact, ncommon);
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
			return (int)(value >> (MM_FEEDBACK_BITS/2));
		}

		/// Returns <code>nB</code>, the number of correct digits (colors)
		/// in the wrong places (pegs).
		int GetCommon() const
		{
			return (int)(value & ((1<<(MM_FEEDBACK_BITS/2))-1));
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


	class FeedbackFrequencyTable
	{
	private:
		unsigned int m_freq[MM_FEEDBACK_COUNT];

	public:
		FeedbackFrequencyTable()
		{
		}

		void Clear()
		{
			memset(m_freq, 0, sizeof(m_freq));
		}

		unsigned int *GetData() { return m_freq; }

		unsigned int operator [] (Feedback fb) const 
		{
			unsigned char i = fb.GetValue();
			assert(i >= 0 && i < MM_FEEDBACK_COUNT);
			return m_freq[i]; 
		}

		// unsigned int& operator [] (Feedback fb) { return m_freq[fb.GetValue()]; }

		int GetPartitionCount() const;

		unsigned int GetMaximum() const;

		unsigned int GetSumOfSquares() const;

		float GetModifiedEntropy() const;

		void DebugPrint() const;

	};


	class FeedbackList
	{
	private:
		unsigned char *m_values;
		int m_count;
		int m_alloctype; // 0=no alloc, 1=malloc, 2=alloca(on stack)

		void Allocate(int count, int type)
		{
			m_alloctype = type;
			m_count = count;
			switch (type) {
			default:
			case 0:
				m_values = NULL;
				break;
			case 1:
				m_values = (unsigned char *)malloc(m_count);
				break;
			case 2:
				m_values = (unsigned char *)_malloca(count);
				break;
			}
		}

	public:
		FeedbackList(int count)
		{
			Allocate(count, 1);
		}

		FeedbackList(unsigned char *values, int count)
		{
			m_alloctype = 0;
			m_values = values;
			m_count = count;
		}

		FeedbackList(const Codeword &guess, const CodewordList &secrets);

		~FeedbackList()
		{
			switch (m_alloctype) {
			default:
			case 0:
				break;
			case 1:
				free(m_values);
				break;
			case 2:
				_freea(m_values);
				break;
			}
			m_values = NULL;
			m_count = 0;
			m_alloctype = 0;
		}

		unsigned char * GetData() { return m_values; }

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

		void CountFrequencies(FeedbackFrequencyTable *freq) const
		{
			CountFrequenciesImpl->Run(this->m_values, this->m_count, freq->GetData());
		}


	};

}