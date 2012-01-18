#include <assert.h>
#include <stdio.h>
#include <limits>

#include "CodeBreaker.h"
#include "CallCounter.h"
#include "Feedback.h"
#include "Algorithm.hpp"

namespace Mastermind {

StrategyTreeMemoryManager *default_strat_mm = new StrategyTreeMemoryManager();

CodeBreaker::CodeBreaker(const CodewordRules &rules)
	: m_rules(rules), m_all(generateCodewords(rules))
{
	assert(rules.valid());

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
	m_possibilities = filterByFeedback(m_possibilities, m_rules, guess, fb);
	assert(m_possibilities.size() > 0);

	unsigned short allmask = ((unsigned short)1 << m_rules.colors()) - 1;
	m_unguessed &= ~getDigitMask(guess);
	m_impossible = allmask & ~getDigitMask(m_possibilities);
	m_guessed = allmask & ~m_unguessed & ~m_impossible;

	if (m_fp) {
		fprintf((FILE*)m_fp, "%x%x\n", count_bits(m_unguessed), count_bits(m_impossible));
	}
}

} // namespace Mastermind
