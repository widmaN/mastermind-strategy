#ifndef MASTERMIND_CODE_BREAKER_HPP
#define MASTERMIND_CODE_BREAKER_HPP

#include <string>
//#include <cassert>
//#include <emmintrin.h>

#include "CodewordRules.hpp"
#include "CodewordList.hpp"
#include "StrategyTree.h"

namespace Mastermind
{

extern StrategyTreeMemoryManager *default_strat_mm;

/// Base class for code breakers. Provides common functionatility.
class CodeBreaker
{
	void *m_fp;

protected:

	/// Rules of the game that this code breaker works on.
	CodewordRules m_rules;

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
	/// Creates a code breaker for the given rules.
	/// The base implementation initializes all members. In particular,
	/// it fills codeword list <code>m_all</code> with all codewords 
	/// conforming to the rules.
	CodeBreaker(const CodewordRules &rules);

	/// Destructor.
	/// The base implementation cleans up member variables.
	virtual ~CodeBreaker();

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
