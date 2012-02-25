#ifndef MASTERMIND_OPTIMAL_STRATEGY_HPP
#define MASTERMIND_OPTIMAL_STRATEGY_HPP

#include <cassert>
#include <vector>
#include <numeric>

#include "Engine.hpp"
#include "Strategy.hpp"

namespace Mastermind {

namespace Heuristics {

/// A special-purpose heuristic used by an optimal strategy to score
/// candidate guesses by the lower bound of the total cost.
/// This heuristic could also be used by a heuristic strategy.
/// @todo Improve the lower-bound estimate.
class MinimizeLowerBound
{
public:

	/// Type of the heuristic score.
	typedef struct score_t
	{
		int steps : 24; // number of steps needed to reveal all secrets
		int depth : 8;  // number of guesses needed in the worst case
		score_t() : steps(0), depth(0) { }
		bool operator < (const score_t &other) const
		{
			return (steps < other.steps) || 
				(steps == other.steps && depth < other.depth);
		}
	};

	// Returns a simple estimate of minimum total number of steps 
	// required to reveal @c n secrets given a branching factor of @c b.
	static score_t simple_estimate(
		int n, // Number of remaining secrets
		int b  // Branching factor: number of distinct non-perfect feedbacks
		)
	{
		score_t cost;
		for (int remaining = n, count = 1; remaining > 0; )
		{
			cost.steps += remaining;
			cost.depth++;
			remaining -= count;
			count *= b;
		}
		return cost;
	}

private:

	Engine &e;
	std::vector<score_t> _cache;

public:

	MinimizeLowerBound(Engine &engine) 
		: e(engine), _cache(e.rules().size()+1)
	{
		// Build a cache of simple estimates.
		int p = e.rules().pegs();
		int b = p*(p+3)/2-1;
		for (size_t n = 0; n < _cache.size(); ++n)
		{
			_cache[n] = simple_estimate(n, b);
		}
	}

	/// Name of the heuristic function.
	std::string name() const { return "Min-LB"; }

	// Returns a simple estimate of minimum total number of steps 
	// required to reveal @c n secrets, assuming the maximum branching
	// factor for the game.
	score_t simple_estimate(int n) const
	{
		assert(n >= 0 && n < (int)_cache.size());
		return _cache[n];
	}

	/// Computes the heuristic score.
	score_t compute(const FeedbackFrequencyTable &freq) const
	{
		Feedback perfect = Feedback::perfectValue(e.rules());
		score_t lb;
		for (size_t j = 0; j < freq.size(); ++j)
		{
			if (freq[j] > 0)
			{
				lb.steps += freq[j];
				if (j == perfect.value())
				{
					lb.depth = std::max(1, lb.depth);
				}
				else
				{
					score_t tmp = simple_estimate(freq[j]);
					lb.steps += tmp.steps;
					lb.depth = std::max(lb.depth, 1+tmp.depth);
				}
			}
		}
		return lb;
	}

#if 0
	// Computes a lower bound for each candidate guess.
	void estimate_candidates(
		CodewordConstRange guesses,
		CodewordConstRange secrets,
		int lower_bound[]) const
	{
		const int secret_count = secrets.size();
		const int guess_count = guesses.size();
		const int table_size = (int)Feedback::maxValue(e.rules()) + 1;
		std::vector<unsigned int> frequency_cache(table_size*guess_count);

		// Partition the secrets using each candidate guess.
		//int max_b = 0; // maximum branching factor of non-perfect feedbacks
		Feedback perfect = Feedback::perfectValue(e.rules());
		for (int i = 0; i < guess_count; ++i)
		{
			Codeword guess = guesses[i];
			FeedbackFrequencyTable freq = e.frequency(e.compare(guess, secrets));
			
			std::copy(freq.begin(), freq.end(), frequency_cache.begin() + i*table_size);

#if 0
			int b = freq.nonzero_count();
			if (freq[perfect.value()] > 0)
				--b;
			if (b > max_b)
				max_b = b;
#endif
		}
		// assert(frequency_cache.size() == table_size*guess_count);

		// For each guess, compute the lower bound of total number of steps
		// required to reveal all secrets starting from that guess.
		for (int i = 0; i < guess_count; ++i)
		{
			int lb = secret_count;
			int j0 = i * table_size;
			for (int j = 0; j < table_size; ++j)
			{
				if (j != perfect.value())
				{
					// Note: we must NOT use max_b because canonical guesses
					// can change after we make a guess. So max_b no longer
					// works.
					//lb += simple_estimate(frequency_cache[j0+j], max_b);
					lb += simple_estimate(frequency_cache[j0+j]);
				}
			}
			lower_bound[i] = lb;
		}
	}
#endif
};

} // namespace Mastermind::Heuristics

/// Options for finding an optimal strategy.
struct OptimalStrategyOptions
{
	unsigned short max_depth; // maximum number of steps to reveal a secret
	bool find_last; // find the last optimal strategy
	bool min_worst; // minimize the number of secrets revealed using
	                // the worst-case number of steps

	OptimalStrategyOptions() 
		: max_depth(0xFFFF), find_last(false), min_worst(false) { }
};

/// Real-time optimal strategy. To be practical, the search space
/// must be small. For example, it works with Mastermind rules (p4c10r),
/// but probably not larger.
class OptimalStrategy
{
public:
	
	/// Returns a name identifying the strategy.
	virtual std::string name() const { return "optimal"; }

	/// Returns a description of the strategy.
	virtual std::string description() const { return "optimal"; }

	/// Makes a guess.
	virtual Codeword make_guess(
		CodewordConstRange possibilities, 
		CodewordConstRange candidates) const;
};

} // namespace Mastermind

#endif // MASTERMIND_OPTIMAL_STRATEGY_HPP
