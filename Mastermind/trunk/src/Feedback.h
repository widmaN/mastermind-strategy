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

		int GetPartitionCount() const 
		{
			int n = 0;
			for (int i = 0; i < MM_FEEDBACK_COUNT; i++) {
				if (m_freq[i] != 0)
					n++;
			}
			return n;
		}

		unsigned int GetMaximum(Feedback *p = NULL) const 
		{
			unsigned int k = 0;
			int pos = 0;
			for (int i = 0; i < sizeof(m_freq)/sizeof(m_freq[0]); i++) {
				if (m_freq[i] > k) {
					k = m_freq[i];
					pos = i;
				}
			}
			if (p) {
				*p = Feedback((unsigned char)pos);
			}
			return k;
		}

		unsigned int GetSumOfSquares() const 
		{
			unsigned int ret = 0;
			for (int i = 0; i < sizeof(m_freq)/sizeof(m_freq[0]); i++) {
				ret += m_freq[i] * m_freq[i];
			}
			return ret;
		}

		double GetModifiedEntropy() const 
		{
			double ret = 0;
			for (int i = 0; i < MM_FEEDBACK_COUNT; i++) {
				if (m_freq[i] > 0) {
					ret += log((double)m_freq[i]) * (double)m_freq[i];

				}
			}
			return ret;
		}

		void DebugPrint() const 
		{
			for (int i = 0; i < MM_FEEDBACK_COUNT; i++) {
				if (m_freq[i] != 0) {
					printf("%d A %d B = %d\n", 
						i >> MM_FEEDBACK_ASHIFT,
						i & MM_FEEDBACK_BMASK,
						m_freq[i]);
				}
			}
		}

	};


	class FeedbackList
	{
	private:
		unsigned char *m_values;
		int m_count;
		int m_alloctype; // 0=new, 1=malloc, 2=alloca(on stack)

		void Allocate(int count, int type)
		{
			m_alloctype = type;
			m_count = count;
			switch (type) {
			default:
			case 0:
				m_values = (unsigned char *)malloc(m_count);
				break;
			case 1:
				m_values = (unsigned char *)_malloca(count);
				break;
			}
		}

	public:
		FeedbackList(int count)
		{
			Allocate(count, 0);
		}

		FeedbackList(const Codeword &guess, const CodewordList &secrets);

		~FeedbackList()
		{
			if (m_values != NULL) {
				switch (m_alloctype) {
				default:
				case 0:
					free(m_values);
					break;
				case 1:
					_freea(m_values);
					break;
				}
				m_values = NULL;
				m_count = 0;
			}
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