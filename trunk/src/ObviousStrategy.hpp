#ifndef MASTERMIND_OBVIOUS_STRATEGY_HPP
#define MASTERMIND_OBVIOUS_STRATEGY_HPP

#include <algorithm>
#include "Strategy.hpp"

namespace Mastermind {

/**
 * Strategy that makes an obviously-optimal guess when one exists. 
 *
 * This is an example (but useful) strategy that implements the @c
 * Strategy interface. This strategy returns an obviously-optimal 
 * guess if one exists. If such a guess is not found, it returns 
 * failure. Therefore, this strategy cannot be used alone to break
 * a code; instead it serves as a quick check before other more
 * sophisticated (and time consuming) strategies are applied.
 *
 * An obviously-optimal guess is a guess (which may or may not be
 * in the remaining possibilities) that partitions the possibility 
 * set into discrete cells, i.e. where every cell contains exactly 
 * one element. If such a guess exists and comes from the possibility 
 * set, then it is optimal because it reveals one secret in the 
 * immediate step and reveals all the rest in two steps. If no such
 * guess exists in the possibility set but one exists outside the 
 * possibility set, then that one is optimal because it reverals
 * all secrets in two steps.
 *
 * A necessary condition for an obviously-optimal guess to exist
 * is that the number of remaining possibilities is no more than
 * the number of distinct feedbacks. For a game with @c p pegs,
 * the number of distinct feedbacks is <code>p*(p+3)/2</code>.
 */
class ObviousStrategy : public Strategy
{
	Engine &e;

public:

	/// Constructs the strategy.
	ObviousStrategy(Engine &engine) : e(engine) { }

	virtual std::string name() const 
	{
		return "obvious"; 
	}

	virtual std::string description() const 
	{
		return "makes an obviously optimal guess"; 
	}

	/*
	 * Returns an obviously-optimal guess if one exists.
	 *
	 * @param possibilities List of remaining possibilities.
	 * @param candidates    List of (suggestive) candidate guesses.
	 * @returns An obviously-optimal guess if one is found; otherwise
	 *      <code>Codeword::emptyValue()</code>.
	 * @timecomplexity No more than <code>K=p*(p+3)/2</code> passes.
	 *      In each pass, a candidate guess is compared to all @c N 
	 *      possibilities, and the frequencies of the feedbacks are 
	 *      checked. The overall complexity is <code>O(KN)</code>.
	 * @spacecomplexity Constant.
	 * @remarks The possibility set MUST be a subset of candidates.
	 *
	 * @todo Currently we only support candidate == possibilities.
	 *      This should be improved in the future.
	 */
	virtual Codeword make_guess(
		CodewordConstRange possibilities, 
		CodewordConstRange /* candidates */)
	{
		size_t count = possibilities.size();
		if (count == 0)
			return Codeword::emptyValue();

		// If there are only two possibilities left, return the first one.
		if (count <= 2) 
			return *possibilities.begin();

		// If the number of possibilities is greater than the number of 
		// distinct feedbacks, there will be no obvious guess.
		size_t p = e.rules().pegs();
		if (count > p*(p+3)/2)
			return Codeword::emptyValue();

		// Returns the first obviously optimal guess in the possibility 
		// set (if any).
		auto it = std::find_if(possibilities.begin(), possibilities.end(),
			[&](const Codeword &guess) -> bool
		{
			return e.frequency(e.compare(guess, possibilities)).max() == 1;
		});
		if (it == possibilities.end())
			return Codeword::emptyValue();
		else
			return *it;
	}
};

} // namespace Mastermind

#endif // MASTERMIND_OBVIOUS_STRATEGY_HPP
