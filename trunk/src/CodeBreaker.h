#ifndef MASTERMIND_CODE_BREAKER_HPP
#define MASTERMIND_CODE_BREAKER_HPP

#include <string>
//#include <cassert>
//#include <emmintrin.h>

#include "Codeword.hpp"
#include "StrategyTree.h"
#include "Engine.hpp"

namespace Mastermind
{

extern StrategyTreeMemoryManager *default_strat_mm;

/// Base class for code breakers. Provides common functionatility.
class CodeBreaker
{
	void *m_fp;

protected:

	/// Environment containing settings, engines, etc.
	Engine &e;

	/// Rules of the game that this code breaker works on.
	Rules m_rules;

	/// List of all codewords conforming to the rules of the game.
	CodewordList m_all;

	/// List of all possible secrets given the feedback information
	/// so far. At the start of the game, this list is the same as
	/// <code>m_all</code>. But as the code breaker makes guesses and
	/// gets feedback, this list narrows.
	CodewordList m_possibilities;

	//CodewordList m_candidates;

	/// Bit-mask of digits guessed so far, excluding impossible digits.
	unsigned short m_guessed;

	/// Bit-mask of digits that have not been guessed so far.
	unsigned short m_unguessed;

	/// Bit-mask of digits that are impossible to be in the secret,
	/// based on the feedback so far.
	unsigned short m_impossible;

public:

#if 0
	/// Creates a code breaker for the given rules.
	/// The base implementation initializes all members. In particular,
	/// it fills codeword list <code>m_all</code> with all codewords
	/// conforming to the rules.
	CodeBreaker(const Rules &rules);
#endif

	/// Creates a code breaker for the given environment.
	/// The base implementation initializes all members. In particular,
	/// it fills codeword list <code>m_all</code> with all codewords
	/// conforming to the rules.
	CodeBreaker(Engine &engine);

	/// Destructor.
	/// The base implementation cleans up member variables.
	virtual ~CodeBreaker();

	//Environment& env() { return _env; }
	//const Environment& env() const { return _env; }

	/**
	 * Tries to make an obviously-optimal guess for a list of remaining
	 * possibilities. Returns such a guess if one is found, or the end
	 * iterator if not found.
	 *
	 * An obviously-optimal guess is an element from the remaining
	 * possibilities such that it partitions the possibility set into
	 * discrete cells, i.e. every cell contains exactly one element.
	 * If such a guess exists, then all the remaining possibilities
	 * can be cleared in two steps, including the guessed element
	 * which is cleared in the immediate step. It is easy to see that
	 * there is no better guess than this one because we need at least
	 * two guesses to clear all possibilities when there are two or more
	 * possibilities.
	 *
	 * A necessary condition for such obviously-optimal guess to exist
	 * is that the number of remaining possibilities is no more than
	 * the number of distinct feedbacks. For a game with @c p pegs,
	 * the number of distinct feedbacks is <code>p*(p+3)/2</code>.
	 *
	 * @param first First possibility
	 * @param last  Last possibility
	 * @returns If an obvious is found, returns an iterator to the guess;
	 *      otherwise, returns <code>last</code>.
	 * @timecomplexity No more than <code>K=p*(p+3)/2</code> passes.
	 *      In each pass, a candidate guess is compared to all N codewords
	 *      between <code>[first, last)</code> and the frequencies of
	 *      the feedbacks are checked. The overall complexity is
	 *      <code>O(KN)</code>.
	 * @spacecomplexity Constant.
	 */
	CodewordList::const_iterator makeObviousGuess(
		CodewordConstRange possibilities) const;
		//CodewordList::const_iterator first,
		//CodewordList::const_iterator last) const;

	/// Resets the code breaker so that it is ready to play a new game.
	/// The base implementation resets member variables to the state
	/// when the code breaker is newly created. In particular, codeword
	/// list <code>m_possibilities</code> is set to <code>m_all</code>.
	virtual void Reset();

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
	virtual void AddFeedback(const Codeword &guess, Feedback fb);

	/// Returns the number of remaining possibilities.
	virtual size_t GetPossibilityCount() { return m_possibilities.size(); }

	/// Makes a guess.
	/// This function must be overridden by an inherited class. It
	/// must return a codeword as the guess it makes. This function is
	/// the most important function of a code breaker.
	virtual Codeword MakeGuess() = 0;

	/// Not implemented.
	virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess) = 0;

	/// Returns a name identifying the code breaker.
	/// This function must be overridden by an inherited class. It
	/// should return a short description of its algorithm, like
	/// "simple".
	virtual std::string name() const = 0;

	/// Returns a description of the code breaker.
	/// This function must be overridden by an inherited class. It
	/// should return a descriptive message of its algorithm, like
	/// "guesses the first remaining possibility".
	virtual std::string description() const = 0;
};

}

#endif // MASTERMIND_CODE_BREAKER_HPP
