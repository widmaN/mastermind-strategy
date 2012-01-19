#include "Algorithm.hpp"
#include "Compare.h"
#include "Scan.h"

namespace Mastermind {

Feedback compare(
	const CodewordRules &rules,
	const Codeword& secret, 
	const Codeword& guess)
{
	unsigned char fb;
	__m128i guess_value = guess.value();

	if (rules.repeatable()) 
		CompareRepImpl->Run(secret.value(), &guess_value, 1, &fb);
	else 
		CompareNoRepImpl->Run(secret.value(), &guess_value, 1, &fb);

	return Feedback(fb);
}

FeedbackList compare(
	const CodewordRules &rules, 
	const Codeword &guess, 
	CodewordList::const_iterator first,
	CodewordList::const_iterator last)
{
	size_t count = last - first;
	FeedbackList feedbacks(count);

	// Perform the actual comparison
	if (rules.repeatable()) 
	{
		CompareRepImpl->Run(
			guess.value(), (const __m128i*)&(*first), count, (unsigned char *)feedbacks.data());
	} 
	else 
	{
		CompareNoRepImpl->Run(
			guess.value(), (const __m128i*)&(*first), count, (unsigned char *)feedbacks.data());
	}
	return feedbacks;
}

unsigned short getDigitMask(const Codeword &c)
{
	__m128i v = c.value();
	return ScanDigitMask(&v, 1);
}

#if 0
unsigned short getDigitMask(const CodewordList &list)
{
	return ScanDigitMask((const __m128i*)list.data(), list.size());
}
#endif

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
	unsigned char fb = feedback.value();
	FeedbackList fblist = compare(rules, guess, list.cbegin(), list.cend());

	// Count feedbacks equal to feedback.
	size_t count = std::count(fblist.cbegin(), fblist.cend(), fb);
#if 0
	if (1) 
	{
		const unsigned char *pfb = fblist.GetData();
		int total = fblist.GetCount();
		while (total-- > 0) {
			if (*(pfb++) == fb)
				count++;
		}
	}
#endif

	// Copy elements whose feedback are equal to fb.
	CodewordList result(count);
	int j = 0;
	for (size_t i = 0; i < fblist.size(); i++) {
		if (fblist[i] == fb) 
			result[j++] = list[i];
	}
	return result;
}

void partition(
	CodewordList::iterator first,
	CodewordList::iterator last,
	const CodewordRules &rules,
	const Codeword &guess,
	FeedbackFrequencyTable &freq)
{
	// If there's no element in the list, do nothing.
	if (first == last)
		return;

	// Compare guess to each codeword in the list.
	FeedbackList fbl = compare(rules, guess, first, last);
	countFrequencies(rules, fbl.cbegin(), fbl.cend(), freq);
	// freq.CountFrequencies(fbl);

	// Build a table to store the range of each partition.
	struct partition_location
	{
		//size_t begin; // begin of the partition
		size_t end;     // end of the partition
		size_t current; // next location to insert
	} part[256+1];

	size_t i = 0;
	part[0].current = 0;
	for (int k = 0; k <= freq.maxFeedback(); k++)
	{
		i += freq[Feedback(k)];
		part[k].end = i;
		part[k+1].current = i;
	}
	part[freq.maxFeedback()+1].end = std::numeric_limits<size_t>::max();

#if 1
	// Find the first non-empty partition.
	int k = 0; // current partition
	while (freq[k] == 0)
		++k;

	// Perform a in-place partitioning.
	size_t count = last - first;
	for (size_t i = 0; i < count; )
	{
		int fbv = fbl[i].value();
		if (fbv == k) 
		{
			// Codeword[i] is in the correct partition.
			// Advance the current partition pointer.
			// If it's reached the end, move to the next partition.
			if (++part[k].current >= part[k].end)
			{
				for (++k; part[k].current >= part[k].end; ++k);
			}
			i = part[k].current;
		}
		else 
		{
			// Codeword[i] is NOT in the correct partition.
			// Swap it into the correct partition, and increment
			// the pointer of that partition.
			std::swap(first[i], first[part[fbv].current++]);
		}
	}

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

void countFrequencies(
	const CodewordRules &rules,
	FeedbackList::const_iterator first,
	FeedbackList::const_iterator last,
	FeedbackFrequencyTable &freq)
{
	int maxfb = Feedback::maxValue(rules);
	CountFrequenciesImpl->Run(
		(const unsigned char *)&(*first), (last - first), freq.data(), maxfb);
}


} // namespace Mastermind
