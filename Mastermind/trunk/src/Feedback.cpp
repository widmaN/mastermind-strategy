#include <stdio.h>

#include "Feedback.h"
#include "Codeword.h"

using namespace Mastermind;

FeedbackList::FeedbackList(const Codeword &guess, const CodewordList &secrets)
{
	Allocate(secrets.GetCount(), 1);
	guess.CompareTo(secrets, *this);
}

unsigned int FeedbackFrequencyTable::GetSumOfSquares() const
{
	unsigned int ret = 0;
	for (int i = 0; i < sizeof(m_freq)/sizeof(m_freq[0]); i++) {
		ret += m_freq[i] * m_freq[i];
	}
	return ret;
}

float FeedbackFrequencyTable::GetModifiedEntropy() const 
{
	float ret = 0.0;
	for (int i = 0; i < MM_FEEDBACK_COUNT; i++) {
		if (m_freq[i] > 0) {
			ret += log((float)m_freq[i]) * (float)m_freq[i];

		}
	}
	return ret;
}

unsigned int FeedbackFrequencyTable::GetMaximum() const
{
	unsigned int k = 0;
	int pos = 0;
	for (int i = 0; i < sizeof(m_freq)/sizeof(m_freq[0]); i++) {
		if (m_freq[i] > k) {
			k = m_freq[i];
			pos = i;
		}
	}
	return k;
}

int FeedbackFrequencyTable::GetPartitionCount() const
{
	int n = 0;
	for (int i = 0; i < MM_FEEDBACK_COUNT; i++) {
		if (m_freq[i] != 0)
			n++;
	}
	return n;
}

void FeedbackFrequencyTable::DebugPrint() const 
{
	int total = 0;
	for (int i = 0; i < MM_FEEDBACK_COUNT; i++) {
		if (m_freq[i] != 0) {
			printf("%d A %d B = %d\n", 
				i >> MM_FEEDBACK_ASHIFT,
				i & MM_FEEDBACK_BMASK,
				m_freq[i]);
			total += m_freq[i];
		}
	}
	printf("Total: %d\n", total);
}