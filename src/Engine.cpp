#include "Engine.hpp"

namespace Mastermind {

// TODO: Does this function take a lot of time?
CodewordList Engine::filterByFeedback(
	const CodewordList &list,
	const Codeword &guess,
	Feedback feedback) const
{
	//Feedback fb = feedback.value();
	FeedbackList fblist = compare(guess, list);

	// Count feedbacks equal to feedback.
	size_t count = std::count(fblist.begin(), fblist.end(), feedback);
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
	size_t j = 0;
	for (size_t i = 0; i < fblist.size(); i++)
	{
		if (fblist[i] == feedback)
			result[j++] = list[i];
	}
	return result;
}

FeedbackFrequencyTable Engine::partition(
	CodewordRange codewords,
	const Codeword &guess) const
{
	// If there's no element in the list, do nothing.
	if (codewords.empty())
		return FeedbackFrequencyTable();

	// Compare the guess to each codeword in the list.
	FeedbackList fbl = compare(guess, codewords);
	FeedbackFrequencyTable freq = frequency(fbl);

	// Build a table to store the range of each partition.
	struct partition_location
	{
		//size_t begin; // begin of the partition
		size_t end;     // end of the partition
		size_t current; // next location to insert
	} part[256+1];

	size_t i = 0;
	part[0].current = 0;
	for (size_t k = 0; k < freq.size(); k++)
	{
		i += freq[k];
		part[k].end = i;
		part[k+1].current = i;
	}
	part[freq.size()].end = std::numeric_limits<size_t>::max();

	// Find the first non-empty partition.
	int k = 0; // current partition
	while (freq[k] == 0)
		++k;

	// Perform a in-place partitioning.
	CodewordIterator first = codewords.begin();
	size_t count = codewords.size();
	for (size_t i = 0; i < count; )
	{
		int fbv = fbl[i].value();
		//std::cout << "Feedback[" << i << "] = " << fbl[i] << std::endl;
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
			size_t j = part[fbv].current++;
			std::swap(first[i], first[j]);
			std::swap(fbl[i], fbl[j]);
		}
	}
	return freq;
}

void Engine::countFrequencies(
	FeedbackList::const_iterator first,
	FeedbackList::const_iterator last,
	FeedbackFrequencyTable &freq) const
{
	if (first != last)
	{
		size_t fb_count = Feedback::size(_rules);
		const unsigned char *buffer = (const unsigned char *)&(*first);
		size_t count = last - first;
		_freq(buffer, buffer + count, freq.data(), fb_count);
		freq.resize(fb_count);
	}
	else
	{
		freq.resize(0);
	}
}

void compare_frequencies_generic(
	const Rules & /* rules */,
	const Codeword &_secret,
	const Codeword *_first,
	const Codeword *_last,
	unsigned int freq[],
	size_t size);

/// @todo Make the interface nicer and reduce duplicate code.
/// @todo Implement it also for codeword without repetition.
/// @todo If the codeword involved in comparison doesn't contain repetitive
/// colors, can it be compared faster?
FeedbackFrequencyTable
Engine::frequencies(const Codeword &guess, CodewordConstRange secrets) const
{
#if 0
	return frequency(compare(guess, secrets));
#else
	FeedbackFrequencyTable freq;
	size_t fb_size = Feedback::size(rules());
	freq.resize(fb_size);
	size_t count = secrets.size();
	const Codeword *first = &(*secrets.begin());
	compare_frequencies_generic(_rules, guess, first, first + count, freq.data(), fb_size);
	return freq;
#endif
}


} // namespace Mastermind
