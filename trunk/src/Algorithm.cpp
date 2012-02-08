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
// Generates all codewords that conforms to the given set of rules.
CodewordList generateCodewords(const CodewordRules &rules)
{
	assert(rules.valid());

	// Compute number of possibilities.
	size_t count = rules.repeatable()? 
		NPower(rules.colors(), rules.pegs()) : NPermute(rules.colors(), rules.pegs());

	// Creates the empty list.
	// @todo: do not initialize Codewords.
	CodewordList list(count);

	// Invoke the handler function to generate all codewords.
	if (rules.repeatable()) 
	{
		Enumerate_Rep(rules.pegs(), rules.colors(), (codeword_t*)list.data());
	} 
	else 
	{
		Enumerate_NoRep(rules.pegs(), rules.colors(), (codeword_t*)list.data());
	}
	return list;
}
#endif

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
