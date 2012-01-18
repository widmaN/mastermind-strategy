#include "Algorithm.hpp"
#include "Compare.h"
#include "Scan.h"

namespace Mastermind {

Feedback compare(const Codeword& secret, const Codeword& guess)
{
	unsigned char fb;
	__m128i guess_value = guess.value();
	CompareRepImpl->Run(secret.value(), &guess_value, 1, &fb);
	return Feedback(fb);
}

unsigned short getDigitMask(const Codeword &c)
{
	__m128i v = c.value();
	return ScanDigitMask(&v, 1);
}

unsigned short getDigitMask(const CodewordList &list)
{
	return ScanDigitMask((const __m128i*)list.data(), list.size());
}

unsigned short getDigitMask(
	CodewordList::const_iterator first, 
	CodewordList::const_iterator last)
{
	return ScanDigitMask((const __m128i*)&(*first), last - first);
}


// TODO: Does this function take a lot of time?
CodewordList filterByFeedback(
	const CodewordList &list,
	const CodewordRules &rules, 
	const Codeword &guess, 
	Feedback feedback)
{
	unsigned char fb = feedback.GetValue();
	FeedbackList fblist(rules, guess, list);

	// Count feedbacks equal to feedback.
	size_t count = 0;
	if (1) 
	{
		const unsigned char *pfb = fblist.GetData();
		int total = fblist.GetCount();
		while (total-- > 0) {
			if (*(pfb++) == fb)
				count++;
		}
	}

	// Copy elements whose feedback are equal to fb.
	CodewordList result(count);
	int j = 0;
	for (int i = 0; i < fblist.GetCount(); i++) {
		if (fblist[i] == fb) 
			result[j++] = list[i];
	}
	return result;
}

void partition(
	CodewordIterator first, 
	CodewordIterator last,
	const CodewordRules &rules,
	const Codeword &guess, 
	FeedbackFrequencyTable &freq)
{
	FeedbackList fbl(rules, guess, first, last);
	freq.CountFrequencies(fbl);

	// Build a table of the start index of each feedback.
	int start_index[256];
	int k = 0;
	start_index[0] = 0;
	for (int i = 0; i <= freq.maxFeedback(); i++) 
	{
		start_index[i] = k;
		k += freq[Feedback(i)];
	}

#if 1
	// Perform a in-place partitioning.
	extern void TBD();
	TBD();

#else
	// Create a spare list to store temporary result
	// TODO: Modify code to enable MT
	static __m128i* tmp_list = NULL;
	static int tmp_size = 0;
	if (tmp_size < m_count) 
	{
		_aligned_free(tmp_list);
		tmp_list = (__m128i *)_aligned_malloc(sizeof(codeword_t)*m_count, sizeof(codeword_t));
		tmp_size = m_count;
	}

	// Re-order codewords to the temporary list
	const unsigned char *fblist = fbl.GetData();
	for (int i = 0; i < m_count; i++) {
		tmp_list[start_index[fblist[i]]++] = m_data[i];
	}

	// Copy temporary list back
	for (int i = 0; i < m_count; i++) 
	{
		m_data[i] = tmp_list[i];
	}
#endif
}

} // namespace Mastermind
