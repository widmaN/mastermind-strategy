#include <stdio.h>
#include <assert.h>

#include "MMConfig.h"
#include "Feedback.h"
#include "Codeword.h"
#include "Frequency.h"
#include "Compare.h"

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// Feedback class implementations
//

Feedback::Feedback(int nA, int nB)
{
	SetValue(nA, nB);
}

void Feedback::SetValue(int nA, int nB)
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

std::string Feedback::ToString() const
{
	char s[5];
	s[0] = '0' + GetExact();
	s[1] = 'A';
	s[2] = '0' + GetCommon();
	s[3] = 'B';
	s[4] = '\0';
	return s;
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
	return Feedback::Empty();
}

///////////////////////////////////////////////////////////////////////////
// FeedbackList class implementations
//

// TODO: implement this for multi-threading
static unsigned char *prealloc_fblist = NULL;
static int prealloc_size = 0;
static bool prealloc_inuse = false;

FeedbackList::FeedbackList(unsigned char *values, int count, int pegs)
{
	m_values = values;
	m_count = count;
	m_maxfb = Feedback(pegs, 0).GetValue();
}

/*
FeedbackList::FeedbackList(int count, int pegs)
{
	m_count = count;
	m_values = (unsigned char *)malloc(m_count);
	m_maxfb = Feedback(pegs, 0).GetValue();
}
*/



FeedbackList::FeedbackList(const Codeword &guess, const CodewordList &secrets)
{
	// Set attributes
	m_count = secrets.GetCount();
	m_maxfb = Feedback(guess.GetPegCount(), 0).GetValue();

	// Allocate memory
	if (!prealloc_inuse) {
		if (prealloc_size < m_count) {
			free(prealloc_fblist);
			prealloc_size = m_count;
			prealloc_fblist = (unsigned char *)malloc(prealloc_size);
		}
		m_values = prealloc_fblist;
		prealloc_inuse = true;
	} else {
		m_values = (unsigned char *)malloc(m_count);
	}

	// Perform the actual comparison
	if (secrets.GetRules().allow_repetition) {
		CompareRepImpl->Run(guess.GetValue().value, (const __m128i*)secrets.GetData(), m_count, m_values);
	} else {
		CompareNoRepImpl->Run(guess.GetValue().value, (const __m128i*)secrets.GetData(), m_count, m_values);
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
	assert(index >= 0 && index < m_count);
	return Feedback(m_values[index]);
}

///////////////////////////////////////////////////////////////////////////
// FeedbackFrequencyTable class implementations
//

FeedbackFrequencyTable::FeedbackFrequencyTable(const FeedbackList &fblist)
{
	CountFrequencies(fblist);
}

void FeedbackFrequencyTable::CountFrequencies(const FeedbackList &fblist)
{
	m_maxfb = fblist.GetMaxFeedbackValue();
	CountFrequenciesImpl->Run(fblist.GetData(), fblist.GetCount(), m_freq, m_maxfb);
}

unsigned int FeedbackFrequencyTable::operator [] (Feedback fb) const 
{
	unsigned char k = fb.GetValue();
	assert(k >= 0 && k <= m_maxfb);
	return m_freq[k]; 
}

unsigned int FeedbackFrequencyTable::GetSumOfSquares() const
{
	//unsigned int ret = 0;
	//for (int i = 0; i < sizeof(m_freq)/sizeof(m_freq[0]); i++) {
	//	ret += m_freq[i] * m_freq[i];
	//}
	//return ret;
	return GetSumOfSquaresImpl->Run(m_freq, m_maxfb);
}

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
