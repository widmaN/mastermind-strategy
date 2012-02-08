#include <assert.h>
#include <stdio.h>
//#include <limits>

#include "CallCounter.h"
#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"
#include "CodeBreaker.h"
#include "Algorithm.hpp"

namespace Mastermind {

StrategyTreeMemoryManager *default_strat_mm = new StrategyTreeMemoryManager();

CodeBreaker::CodeBreaker(Engine &engine)
	: e(engine), m_rules(e.rules()), m_all(e.generateCodewords())
{
	assert(m_rules.valid());

	// m_fp = fopen("E:/mastermind_stat.txt", "w");
	m_fp = NULL;

	Reset();
}

CodeBreaker::~CodeBreaker()
{
	if (m_fp) {
		fclose((FILE*)m_fp);
		m_fp = NULL;
	}
}

void CodeBreaker::Reset()
{
	m_possibilities = m_all;
	m_guessed = 0;
	m_unguessed = ((unsigned short)1 << m_rules.colors()) - 1;
	m_impossible = 0;
	if (m_fp) {
		fprintf((FILE*)m_fp, "R\n");
	}
}

static int count_bits(unsigned short a)
{
	int n = 0;
	for (; a; a >>= 1) {
		if (a & 1)
			n++;
	}
	return n;
}

void CodeBreaker::AddFeedback(const Codeword &guess, Feedback fb)
{
	m_possibilities = e.filterByFeedback(m_possibilities, guess, fb);
	assert(m_possibilities.size() > 0);

	unsigned short allmask = ((unsigned short)1 << m_rules.colors()) - 1;
	m_unguessed &= ~e.getDigitMask(guess);
	m_impossible = allmask & ~e.getDigitMask(m_possibilities.cbegin(), m_possibilities.cend());
	m_guessed = allmask & ~m_unguessed & ~m_impossible;

	if (m_fp) {
		fprintf((FILE*)m_fp, "%x%x\n", count_bits(m_unguessed), count_bits(m_impossible));
	}
}

CodewordList::const_iterator CodeBreaker::makeObviousGuess(
	CodewordList::const_iterator first,
	CodewordList::const_iterator last) const
{
	size_t p = e.rules().pegs();
	size_t count = last - first;
	if (count == 0)
		return last;

	// If there are only two possibilities left, return the first one.
	if (count <= 2) 
		return first;

	// If the number of possibilities is more than the number of distinct
	// feedbacks, there will be no obvious guess.
	if (count > p*(p+3)/2)
		return last;

	// Check for obviously optimal guess.
	for (auto it = first; it != last; ++it)
	{
#if 1
		const Codeword &guess = *it;
		FeedbackFrequencyTable freq;
		FeedbackList fbl = e.compare(guess, first, last);
		e.countFrequencies(fbl.begin(), fbl.end(), freq);
		if (freq.max() == 1) 
			return it;
#else
		if (e.frequencies(e.compare(*it, first, last)).max() == 1)
			return it;
#endif
	}

	// Not found.
	return last;
}

} // namespace Mastermind
