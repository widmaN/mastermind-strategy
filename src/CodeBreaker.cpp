#include "CodeBreaker.hpp"

using namespace Mastermind;

Codeword CodeBreaker::MakeGuess()
{
	CodewordConstRange possibilities = m_possibilities;
	size_t count = possibilities.size();
	if (count == 0)
		return Codeword::emptyValue();

	// state->NPossibilities = count;

	// Check for obvious guess.
	Codeword guess = obvious.make_guess(possibilities, possibilities);
	if (!guess.empty())
	{
		//state->NCandidates = (it - possibilities.begin()) + 1;
		//state->Guess = *it;
		return guess;
	}

	// Initialize the set of candidate guesses.
	CodewordConstRange candidates = _pos_only?
		possibilities : CodewordConstRange(m_all);

	// Filter the candidate set to remove "equivalent" guesses.
	// For now the equivalence is determined by bit-mask of colors.
	// In the next step, we should determine it by graph isomorphasm.
	CodewordList canonical = 
		CodewordList(candidates.begin(), candidates.end());
		//candidates.FilterByEquivalence(unguessed_mask, impossible_mask);

	// Make a guess using the strategy provided.
	guess = strat->make_guess(possibilities, canonical);
	return guess;
}

#if 0
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

CodewordList::const_iterator 
CodeBreaker::makeObviousGuess(CodewordConstRange possibilities) const
{
	size_t count = possibilities.size();
	if (count == 0)
		return possibilities.end();

	// If there are only two possibilities left, return the first one.
	if (count <= 2) 
		return possibilities.begin();

	// If the number of possibilities is more than the number of distinct
	// feedbacks, there will be no obvious guess.
	size_t p = e.rules().pegs();
	if (count > p*(p+3)/2)
		return possibilities.end();

	// Find the first obviously optimal guess (if any).
#if 0
	for (auto it = possibilities.begin(); it != possibilities.end(); ++it)
	{
#if 0
		const Codeword &guess = *it;
		FeedbackFrequencyTable freq;
		FeedbackList fbl = e.compare(guess, possibilities);
		e.countFrequencies(fbl.begin(), fbl.end(), freq);
		if (freq.max() == 1) 
			return it;
#else
		if (e.frequency(e.compare(*it, possibilities)).max() == 1)
			return it;
#endif
	}
	return possibilities.end(); // not found
#else
	return std::find_if(possibilities.begin(), possibilities.end(),
		[&](const Codeword &guess) -> bool
	{
		return e.frequency(e.compare(guess, possibilities)).max() == 1;
	});
#endif
}
#endif
