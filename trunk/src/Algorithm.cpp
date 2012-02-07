#include "Algorithm.hpp"
#include "Compare.h"
#include "Scan.h"

namespace Mastermind {

#if 0
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
#endif

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

#if 0
// TODO: Does this function take a lot of time?
CodewordList filterByFeedback(
	const Environment &e,
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
#endif

void countFrequencies(
	const CodewordRules &rules,
	FeedbackList::const_iterator first,
	FeedbackList::const_iterator last,
	FeedbackFrequencyTable &freq)
{
	int maxfb = Feedback::maxValue(rules);
	CountFrequenciesImpl->Run(
		(const unsigned char *)&(*first), (last - first), freq.data(), maxfb);
	freq.setMaxFeedback(maxfb);
}


} // namespace Mastermind
