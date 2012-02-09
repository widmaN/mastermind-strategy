#ifndef MASTERMIND_CODE_BREAKER_HPP
#define MASTERMIND_CODE_BREAKER_HPP

#include <string>
#include "Engine.hpp"
#include "Strategy.hpp"
#include "ObviousStrategy.hpp"
#include "StrategyTree.hpp"
#include "State.hpp"

namespace Mastermind
{

struct CodeBreakerOptions
{
	bool optimize_obvious; // make an obvious guess first
	bool possibility_only; // only make a guess from possibilities
	
	CodeBreakerOptions()
		: optimize_obvious(true), 
		possibility_only(false)
	{ }
};

// Free-standing function that makes a guess.
Codeword MakeGuess(
	Engine &e,
	State &state,
	Strategy *strat,
	const CodeBreakerOptions &options);

// Free-standing function that builds a strategy tree.
StrategyTree BuildStrategyTree(
	Engine &e, 
	Strategy *strat, 
	const CodeBreakerOptions &options);

/// Helper class that uses a given strategy to break a code.
class CodeBreaker
{
	/// Algorithm engine.
	Engine &e;

	/// The strategy used to make guesses.
	/// @todo Use auto_ptr or smart_ptr.
	Strategy *strat;

	/// Options.
	CodeBreakerOptions _options;

	/// Set of possibilities. This set is updated on the way.
	CodewordList _possibilities;

	/// State of the game.
	State _state;

public:

	/// Creates a code breaker using the given engine and strategy.
	CodeBreaker(
		Engine &engine, 
		Strategy *strategy, 
		const CodeBreakerOptions &options)
		: e(engine), 
		strat(strategy), 
		_options(options),
		_possibilities(e.universe().begin(), e.universe().end()),
		_state(e, _possibilities)
	{ 
	}

	/// Returns the strategy used.
	const Strategy* strategy() const { return strat; }

	/// Adds a constraint (guess:feedback pair). Updates the state of
	/// the game.
	///
	/// Note that the guess provided here may not be the guess that the
	/// code breaker suggests previously (via <code>MakeGuess()</code>).
	/// It might even happen that the code breaker has not been asked
	/// to make a guess at all. If a particular implementation does not
	/// support such behavior (e.g. one that makes guesses according to
	/// a pre-built strategy tree), it must throw an exception.
	void AddConstraint(const Codeword &guess, Feedback feedback)
	{
		_possibilities = e.filterByFeedback(_possibilities, guess, feedback);
		_state.udpate(e, guess, feedback, _possibilities);
	}

	/// Makes a guess.
	Codeword MakeGuess()
	{
		return Mastermind::MakeGuess(e, _state, strat, _options);
	}
};

}

#endif // MASTERMIND_CODE_BREAKER_HPP
