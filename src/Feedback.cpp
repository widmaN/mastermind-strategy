#include <iostream>
#include <stdio.h>
#include <assert.h>

#include "MMConfig.h"
#include "Feedback.h"
#include "CodewordList.hpp"
//#include "Frequency.h"

// using namespace Mastermind;

namespace Mastermind {

///////////////////////////////////////////////////////////////////////////
// Feedback class implementations
//

Feedback::Feedback(int nA, int nB)
{
	setValue(nA, nB);
}

void Feedback::setValue(int nA, int nB)
{
	assert(nA >= 0 && nA <= MM_MAX_PEGS);
	assert(nB >= 0 && nB <= MM_MAX_PEGS);
	assert(nA+nB >= 0 && nA+nB <= MM_MAX_PEGS);
#if MM_FEEDBACK_COMPACT
	int nAB = nA + nB;
	m_value = (nAB+1)*nAB/2+nA;
#else
	m_value = (nA << MM_FEEDBACK_ASHIFT) | nB;
#endif
}

// @todo Should we use the mapping defined in MMConfig.h?
int Feedback::GetExact() const
{
#if MM_FEEDBACK_COMPACT
	int k = 0;
	for (int nAB = 0; nAB <= MM_MAX_PEGS; nAB++) {
		if (m_value >= k && m_value <= (k+nAB)) {
			return (m_value - k);
		}
		k += (nAB + 1);
	}
	assert(0);
	return 0;
#else
	return (int)(m_value >> MM_FEEDBACK_ASHIFT);
#endif
}

// @todo Should we use the mapping defined in MMConfig.h?
int Feedback::GetCommon() const
{
#if MM_FEEDBACK_COMPACT
	int k = 0;
	for (int nAB = 0; nAB <= MM_MAX_PEGS; nAB++) {
		if (m_value >= k && m_value <= (k+nAB)) {
			return (nAB - (m_value - k));
		}
		k += (nAB + 1);
	}
	assert(0);
	return 0;
#else
	return (int)(m_value & MM_FEEDBACK_BMASK);
#endif
}

std::ostream& operator << (std::ostream &os, const Feedback &fb)
{
	return os << fb.nA() << 'A' << fb.nB() << 'B';
}


Feedback Feedback::Parse(const char *s)
{
	assert(s != NULL);
	if ((s[0] >= '0' && s[0] <= '9') &&
		(s[1] == 'A' || s[1] == 'a') &&
		(s[2] >= '0' && s[2] <= '9') &&
		(s[3] == 'B' || s[3] == 'b') &&
		(s[4] == '\0')) {
		return Feedback(s[0] - '0', s[2] - '0');
	} 
	return Feedback::emptyValue();
}


///////////////////////////////////////////////////////////////////////////
// FeedbackList class implementations
//

#if 0
// TODO: implement this for multi-threading
static unsigned char *prealloc_fblist = NULL;
static size_t prealloc_size = 0;
static bool prealloc_inuse = false;

FeedbackList::FeedbackList(unsigned char *values, int count, int pegs)
{
	m_values = values;
	m_count = count;
	m_maxfb = Feedback::MaxValue(pegs);
}
#endif

/*
FeedbackList::FeedbackList(int count, int pegs)
{
	m_count = count;
	m_values = (unsigned char *)malloc(m_count);
	m_maxfb = Feedback(pegs, 0).GetValue();
}
*/

#if 0
FeedbackList::FeedbackList(
	const CodewordRules &rules, 
	const Codeword &guess, 
	CodewordList::const_iterator first,
	CodewordList::const_iterator last)
	: m_count(last - first), m_maxfb(Feedback::maxValue(rules))
{
	// Allocate memory
	if (!prealloc_inuse) 
	{
		if (prealloc_size < m_count)
		{
			free(prealloc_fblist);
			prealloc_size = m_count;
			prealloc_fblist = (unsigned char *)malloc(prealloc_size);
		}
		m_values = prealloc_fblist;
		prealloc_inuse = true;
	} 
	else 
	{
		m_values = (unsigned char *)malloc(m_count);
	}

	// Perform the actual comparison
	if (rules.repeatable()) 
	{
		CompareRepImpl->Run(guess.value(), (const __m128i*)&(*first), m_count, m_values);
	} 
	else 
	{
		CompareNoRepImpl->Run(guess.value(), (const __m128i*)&(*first), m_count, m_values);
	}
}

FeedbackList::~FeedbackList()
{
	if (m_values != NULL) {
		if (m_values != prealloc_fblist) {
			free(m_values);
		} else {
			prealloc_inuse = false;
		}
		m_values = NULL;
		m_count = 0;
	}
}

Feedback FeedbackList::operator [] (int index) const
{
	assert(index >= 0 && index < (int)m_count);
	return Feedback(m_values[index]);
}
#endif

///////////////////////////////////////////////////////////////////////////
// FeedbackFrequencyTable class implementations
//

#if 0
FeedbackFrequencyTable::FeedbackFrequencyTable(const FeedbackList &fblist)
{
	CountFrequencies(fblist);
}
#endif

#if 0
void FeedbackFrequencyTable::CountFrequencies(const FeedbackList &fblist)
{
	m_maxfb = fblist.GetMaxFeedbackValue();
	CountFrequenciesImpl->Run(fblist.GetData(), fblist.GetCount(), m_freq, m_maxfb);
}
#endif

#if 0
unsigned int FeedbackFrequencyTable::GetSumOfSquares() const
{
	//unsigned int ret = 0;
	//for (int i = 0; i < sizeof(m_freq)/sizeof(m_freq[0]); i++) {
	//	ret += m_freq[i] * m_freq[i];
	//}
	//return ret;
	return GetSumOfSquaresImpl->Run(m_freq, m_maxfb);
}
#endif

double FeedbackFrequencyTable::GetModifiedEntropy() const 
{
	double ret = 0.0;
	for (int i = 0; i <= (int)m_maxfb; i++) {
		if (m_freq[i] > 0) {
			ret += log((double)m_freq[i]) * (double)m_freq[i];
		}
	}
	return ret;
}

unsigned int FeedbackFrequencyTable::GetMaximum() const
{
	unsigned int m = 0;
	for (int i = 0; i <= (int)m_maxfb; i++) {
		if (m_freq[i] > m) {
			m = m_freq[i];
		}
	}
	return m;
}

int FeedbackFrequencyTable::GetPartitionCount() const
{
	int n = 0;
	for (int i = 0; i <= (int)m_maxfb; i++) {
		if (m_freq[i] != 0)
			n++;
	}
	return n;
}

#if 1
std::ostream& operator << (std::ostream &os, const FeedbackFrequencyTable &f)
{
	int total = 0;
	for (unsigned char i = 0; i <= f.maxFeedback(); i++) 
	{
		if (f[i] != 0) 
		{
			os << Feedback(i) << " = " << f[i] << std::endl;
			total += f[i];
		}
	}
	os << "Total: " << total << std::endl;
	return os;
}
#else
void FeedbackFrequencyTable::DebugPrint() const 
{
	int total = 0;
	for (int i = 0; i <= (int)m_maxfb; i++) {
		if (m_freq[i] != 0) {
			printf("%s = %d\n", 
				Feedback((unsigned char)i).ToString().c_str(),
				m_freq[i]);
			total += m_freq[i];
		}
	}
	printf("Total: %d\n", total);
}
#endif

} // namespace Mastermind
