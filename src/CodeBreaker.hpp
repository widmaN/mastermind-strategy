#ifndef MASTERMIND_CODE_BREAKER_HPP
#define MASTERMIND_CODE_BREAKER_HPP

#include <string>
#include "Engine.hpp"
#include "Strategy.hpp"
#include "ObviousStrategy.hpp"
#include "StrategyTree.h"

namespace Mastermind
{

extern StrategyTreeMemoryManager *default_strat_mm;

/// Agent class that uses a given strategy to break a code.
class CodeBreaker
{
	// void *m_fp;

	/// Algorithm engine.
	Engine &e;

	/// Rules of the game that this code breaker works on.
	//Rules m_rules;

	/// Set of all codewords conforming to the rules.
	CodewordList m_all;

	/// Set of remaining possible secrets given the feedbacks so far.
	/// At the beginning of the game, this set is the same as
	/// <code>m_all</code>. But as the code breaker makes guesses and
	/// gets feedback, this list narrows.
	CodewordList m_possibilities;

	//CodewordList m_candidates;

	/// Information about the guesses made so far. Used to prune the
	/// search space.
	struct Hints
	{
		/// Algorithm engine.
		Engine &e;

		/// Bit-mask of colors that have not been guessed so far.
		unsigned short fresh; // m_unguessed;

		/// Bit-mask of colors that are excluded from the possibilities.
		/// These colors are sure to be not present in the secret.
		unsigned short excluded; // m_impossible

		/// Bit-mask of colors guessed so far, excluding impossible digits.
		/// This member is not useful.
		unsigned short guessed; // m_guessed

		/// Initializes hints.
		Hints(Engine &engine)
			: e(engine), fresh(((unsigned short)1 << e.rules().colors()) - 1),
			excluded(0), guessed(0) { }

		/// Updates the hints with a new guess.
		void udpate(	const Codeword &guess, CodewordConstRange possibilities)
		{
			unsigned short allmask = ((unsigned short)1 << e.rules().colors()) - 1;
			fresh &= ~e.getDigitMask(guess);
			excluded = allmask & ~e.color_mask(possibilities);
			guessed = allmask & ~fresh & ~excluded;
		}
	};
	
	/// The hints collected so far.
	Hints hints;

	/// The strategy used to make guesses.
	/// @todo Use auto_ptr or smart_ptr.
	Strategy *strat;

	/// Helper strategy that checks for obvious guesses.
	ObviousStrategy obvious;

	/// Whether to make guess only from remaining possibilities.
	bool _pos_only;

public:

	/// Creates a code breaker using the given engine and strategy.
	CodeBreaker(Engine &engine, Strategy *strategy, bool pos_only)
		: e(engine), m_all(e.generateCodewords()), m_possibilities(m_all),
		hints(e), strat(strategy), obvious(e), _pos_only(pos_only)
	{
		// m_fp = fopen("E:/mastermind_stat.txt", "w");
		// m_fp = NULL;
		//if (m_fp) 
		//fprintf((FILE*)m_fp, "R\n");
	}

	/// Destructor.
	/// The base implementation cleans up member variables.
	~CodeBreaker()
	{
		//if (m_fp) 
		//fclose((FILE*)m_fp);
		//m_fp = NULL;
	}

	/// Provides the code breaker with a piece of feedback for a given
	/// guess.
	///
	/// The base implementation updates <code>m_possibilities</code>
	/// and several other digit masks based on the feedback information
	/// provided.
	///
	/// Note that the guess provided here may not be the guess that the
	/// code breaker suggests previously (via <code>MakeGuess()</code>).
	/// It might even happen that the code breaker has not been asked
	/// to make a guess at all. If a particular implementation does not
	/// support such behavior (e.g. one that makes guesses according to
	/// a pre-built strategy tree), it must throw an exception.
	void AddFeedback(const Codeword &guess, Feedback fb)
	{
		m_possibilities = e.filterByFeedback(m_possibilities, guess, fb);
		hints.udpate(guess, m_possibilities);
		//if (m_fp) {
		//	fprintf((FILE*)m_fp, "%x%x\n", count_bits(m_unguessed), count_bits(m_impossible));
		//}
	}

	/// Returns the set of remaining possibilities.
	CodewordConstRange possibilities() const { return m_possibilities; }
	// size_t GetPossibilityCount() { return m_possibilities.size(); }

	const Strategy* strategy() const { return strat; }

	/// Makes a guess.
	/// This function must be overridden by an inherited class. It
	/// must return a codeword as the guess it makes. This function is
	/// the most important function of a code breaker.
	Codeword MakeGuess();

#if 0
	/// Not implemented.
	virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess) = 0;
#endif
};

}

#endif // MASTERMIND_CODE_BREAKER_HPP
