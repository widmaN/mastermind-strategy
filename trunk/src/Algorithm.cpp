#include "Algorithm.hpp"
#include "Scan.h"

namespace Mastermind {

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
#endif

} // namespace Mastermind
