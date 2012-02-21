/**
 * On the micro-scope, a strategy is a function that receives as input
 *   - a list of possibilities
 *   - a list of candidate guesses
 * and returns as output a guess to make.
 *
 */
#ifndef MASTERMIND_STRATEGY_HPP
#define MASTERMIND_STRATEGY_HPP

#include <string>
#include "Engine.hpp"

namespace Mastermind {

// simple strategy
// heuristic strategy
// optimal strategy
// playback strategy (reads from a file)

// Defines an interface to a strategy implementation.
struct Strategy
{
	/// Returns a name identifying the strategy.
	virtual std::string name() const = 0;

	/// Returns a description of the strategy.
	virtual std::string description() const = 0;

	/*
	 * Makes a guess.
	 *
	 * @param possibilities List of remaining possibilities.
	 * @param candidates    List of (suggestive) candidate guesses.
	 *      Note that the function is not required to return a guess
	 *      from this list.
	 * @returns If successful, returns the guess to make. The guess
	 *      must be from @c possibility OR @c candidates. If failed,
	 *      returns <code>Codeword::emtpyValue()</code>. The condition
	 *      under which the call fails is implementation-specific.
	 *
	 * @timecomplexity <i>Implementation-specific</i>.
	 * @spacecomplexity <i>Implementation-specific</i>.
	 */
	virtual Codeword make_guess(
		CodewordConstRange possibilities, 
		CodewordConstRange candidates) const = 0;
};

} // namespace Mastermind

#endif // MASTERMIND_STRATEGY_HPP
