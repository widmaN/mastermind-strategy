#ifndef MASTERMIND_SIMPLE_CODE_BREAKER_HPP
#define MASTERMIND_SIMPLE_CODE_BREAKER_HPP

#include "CodeBreaker.h"

namespace Mastermind {

/// Naive code breaker that always guesses the first codeword from
/// the remaining possibilities.
class SimpleCodeBreaker : public CodeBreaker
{
	Codeword MakeGuess(CodewordList &possibilities);
	StrategyTreeNode* FillStrategy(CodewordList possibilities, const Codeword &force_guess);

public:
	/// Creates a simple code breaker.
	SimpleCodeBreaker(Engine &engine) : CodeBreaker(engine)
	{ 
	}

	/// Makes a guess.
	/// The simple code breaker always guesses the first codeword from
	/// the remaining possibilities.
	virtual Codeword MakeGuess();

	/// Returns a name identifying the simple code breaker.
	virtual std::string name() const
	{
		return "simple";
	}

	/// Returns a description of the simple code breaker.
	virtual std::string description() const
	{
		return "guesses the first possibility";
	}

	/// Not implemented.
	virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess);
};

} // namespace Mastermind

#endif // MASTERMIND_SIMPLE_CODE_BREAKER_HPP
