#ifndef MASTERMIND_STRATEGY_HPP
#define MASTERMIND_STRATEGY_HPP

#include <iostream>
#include <string>
#include "Engine.hpp"

namespace Mastermind {

/**
 * Defines the objectives of a strategy. Here we implement three types of
 * objectives, in order of their strength (i.e. an objective with a larger
 * numeric value if strictly more optimal than an objective with a smaller
 * numeric value).
 */
enum StrategyObjective
{
	// Minimize the total number of guesses needed to reveal all secrets.
	MinSteps = 1,

	// In addition to @c MinSteps, also minimize the maximum number of
	// guesses required to reveal any given secret.
 	MinDepth = 2,

	// In addition to @c MinDepth, also minimize the number of secrets
	// revealed by the most number of guesses.
	MinWorst = 3,
};

/**
 * Defines the constraints of the strategy.
 */
struct StrategyConstraints
{
	/// Maximum number of guesses allowed for any single secret.
	unsigned char max_depth;

	/// Flag indicating whether to make a guess only from the remaining
	/// possibilities.
	bool pos_only;

	/// Flag indicating if an obvious guess can be used if available.
	bool use_obvious;

	/// Flag indicating whether to find the last one among all optimal strategies.
	bool find_last;

	/// Creates a default set of (non-)constraints.
	StrategyConstraints()
		: max_depth(100), pos_only(false), use_obvious(true), find_last(false)
	{ }

	/// Creates a set of constraints using the provided parameters.
	//StrategyConstraints(unsigned char _max_depth, bool _pos_only, bool _use_obvious)
	//	: max_depth(_max_depth), pos_only(_pos_only), use_obvious(_use_obvious) { }
};

/**
 * Represents the cost of a strategy in terms of the number of guesses
 * required to reveal the secrets.
 *
 * The cost of a strategy consists of the following parts, in order of
 * decreasing priority:
 * - @c steps: the total number of guesses needed to reveal all
 *   secrets, excluding the initial guess.
 * - @c depth: the maximum number of guesses needed to reveal a
 *   secret, excluding the initial guess.
 * - @c worst: the number of secrets revealed by the worst number
 *   of steps.
 *
 * @remarks The machine must be little-endian for the comparison routine
 *      to work correctly.
 * @ingroup Optimal
 */
struct StrategyCost
{
	union
	{
		unsigned long long value;
		struct
		{
			unsigned short worst; // number of secrets revealed using max depth
			unsigned short depth; // number of guesses needed in the worst case
			unsigned int   steps; // total number of steps to reveal all secrets
		};
	};

	StrategyCost() : value(0) { }
	StrategyCost(unsigned int _steps, unsigned short _depth, unsigned short _worst)
		: worst(_worst),  depth(_depth), steps(_steps) { }
};

/// Compares the costs of two strategies.
/// @ingroup strat
inline bool operator < (const StrategyCost &c1, const StrategyCost &c2)
{
	return c1.value < c2.value;
}

/// Outputs strategy cost to a stream.
/// @ingroup strat
inline std::ostream& operator << (std::ostream &os, const StrategyCost &c)
{
	return os << c.steps << ':' << c.depth;
}

/// Checks if strategy cost @c a is strictly superior to (i.e. lower than)
/// strategy cost @c b with regard to the objective @c obj.
inline bool superior(const StrategyCost &a, const StrategyCost &b, StrategyObjective obj)
{
	if (a.steps < b.steps)
		return true;
	if (b.steps < a.steps)
		return false;
	if (obj <= MinSteps)
		return false;

	if (a.depth < b.depth)
		return true;
	if (b.depth < a.depth)
		return false;
	if (obj <= MinDepth)
		return false;

	return a.worst < b.worst;
}

/**
 * Interface for a Mastermind strategy.
 *
 * On the micro-scope, a strategy is a function that receives as input
 *   - a list of possibilities
 *   - a list of candidate guesses
 * and returns as output a guess to make.
 *
 * A few strategies implemented (or to be implemented) include:
 *   - simple strategy
 *   - heuristic strategy
 *   - optimal strategy
 *   - playback strategy (reads from a strategy tree)
 *
 * @ingroup strat
 */
struct Strategy
{
	/// Returns the name of the strategy.
	virtual std::string name() const = 0;

	/**
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
	 */
	virtual Codeword make_guess(
		CodewordConstRange possibilities,
		CodewordConstRange candidates) const = 0;
};

} // namespace Mastermind

#endif // MASTERMIND_STRATEGY_HPP
