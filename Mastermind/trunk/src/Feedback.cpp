#include <stdio.h>

#include "Feedback.h"
#include "Codeword.h"
#include "Frequency.h"

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// FeedbackList implementations
//

FeedbackList::FeedbackList(int count, int pegs)
{
	m_count = count;
	m_values = (unsigned char *)malloc(m_count);
	m_maxfb = Feedback(pegs, 0).GetValue();
}

FeedbackList::FeedbackList(const Codeword &guess, const CodewordList &secrets)
{
	m_count = secrets.GetCount();
	m_values = (unsigned char *)malloc(m_count);
	m_maxfb = Feedback(guess.GetPegCount(), 0).GetValue();
	guess.CompareTo(secrets, *this);
}

FeedbackList::~FeedbackList()
{
	if (m_values != NULL) {
		free(m_values);
		m_values = NULL;
		m_count = 0;
	}
}

///////////////////////////////////////////////////////////////////////////
// FeedbackFrequencyTable implementations
//

FeedbackFrequencyTable::FeedbackFrequencyTable(const FeedbackList &fblist)
{
	m_maxfb = fblist.GetMaxFeedbackValue();
	CountFrequenciesImpl->Run(fblist.GetData(), fblist.GetCount(), m_freq, m_maxfb);
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

float FeedbackFrequencyTable::GetModifiedEntropy() const 
{
	float ret = 0.0;
	for (int i = 0; i <= (int)m_maxfb; i++) {
		if (m_freq[i] > 0) {
			ret += log((float)m_freq[i]) * (float)m_freq[i];
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
