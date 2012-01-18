#ifndef MASTERMIND_OPTIMAL_CODE_BREAKER_HPP
#define MASTERMIND_OPTIMAL_CODE_BREAKER_HPP

#include "CodeBreaker.h"

namespace Mastermind {

/// Code breaker that finds the optimal guess by Depth-First Search.
class OptimalCodeBreaker : public CodeBreaker
{
private:
	int *m_partsize2minsteps;

	struct progress_t 
	{
		double begin;
		double end;
		bool display;
	};

	int SearchLowestSteps(
		CodewordList possibilities,
		unsigned short unguessed_mask,
		unsigned short impossible_mask,
		int *depth,
		Codeword *choice);

	struct search_t {
		int round;
		int total_steps;
		int lower_bound;
		int max_cost;
	};

	StrategyTreeNode* FillStrategy(
		CodewordList possibilities,
		unsigned short unguessed_mask,
		unsigned short impossible_mask,
		const Codeword& first_guess,
		const progress_t *progress,
		const search_t *arg);

public:
	/// Creates an optimal code breaker.
	OptimalCodeBreaker(CodewordRules rules) : CodeBreaker(rules) { }

	Codeword MakeGuess(
		CodewordList possibilities,
		unsigned short unguessed_mask,
		unsigned short impossible_mask);

	/// Makes a guess.
	/// The optimal code breaker performs a depth-first search to pick
	/// the guess that minimizes the expected number of further guesses
	/// needed to get the secret. The code breaker applies the maximum-steps
	/// constraint if it is provided in the constructor.
	virtual Codeword MakeGuess();

	virtual const char* GetName() const;

	/// Returns a description of the optimal code breaker.
	virtual std::string GetDescription() const;

	/// Not implemented.
	virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess);
};

} // namespace Mastermind

#endif // MASTERMIND_OPTIMAL_CODE_BREAKER_HPP
