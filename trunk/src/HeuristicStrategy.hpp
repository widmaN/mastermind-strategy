#ifndef MASTERMIND_HEURISTIC_STRATEGY_HPP
#define MASTERMIND_HEURISTIC_STRATEGY_HPP

#include "Strategy.hpp"
#include "util/wrapped_float.hpp"

namespace Mastermind {

/**
 * Strategy that makes the guess that produces the optimal score for
 * a heuristic function.
 *
 * The heuristic strategy makes a guess by computing a score for each
 * candidate guess using a <i>heuristic function</i>. The heuristic 
 * function takes as input the partitioning of the possibility set by
 * the guess, and outputs a scalar score. The guess that produces the
 * optimal (lowest) score is chosen. If more than one guesses have
 * the same score, the guess that is in the possibility set is chosen.
 * If multiple choices still exist, the first one is chosen.
 *
 * The heuristic function used by the strategy is specified in the
 * template parameter <code>Heuristic</code>. It takes as input the
 * frequency table of the partition, and returns a heuristic score.
 * The partition is defined as below.
 *
 * Suppose there are <code>N</code> remaining possibilities. Given
 * a guess, these possibilities can be partitioned into <code>m</code> 
 * cells, where the possibilities in each cell have the same feedback
 * when compared to the guess. That is, given that guess, there is 
 * no way to distinguish one possibility from another in the same 
 * cell.
 *
 * Note that when computing the heuristic score, we always assume that
 * each codeword in the possibility set is equally-likely to be the
 * secret.
 */
template <class Heuristic>
class HeuristicStrategy : public Strategy
{		
	Engine &e;
	Heuristic h;

public:

	typedef typename Heuristic::score_t score_type;

	/// Constructs the strategy.
	HeuristicStrategy(Engine &engine, const Heuristic &heuristic = Heuristic())
		: e(engine), h(heuristic) { }

	Heuristic& heuristic() { return h; }

	/// Returns the name of the strategy.
	virtual std::string name() const
	{
		 return h.name();
	}

	/// Returns a description of the strategy.
	virtual std::string description() const
	{
		return h.name();
	}

	/// Makes the guess that produces the lowest heuristic score.
	/// Optionally stores the score of all candidates in an array.
	Codeword make_guess(
		CodewordConstRange possibilities, 
		CodewordConstRange candidates,
		score_type *scores) const
	{
		if (candidates.empty())
			return Codeword::emptyValue();

		Codeword choice = Codeword::emptyValue();
		typename Heuristic::score_t choice_score;
		bool choice_ispos = false;
		size_t target = Feedback::perfectValue(e.rules()).value();

		// Evaluate each candidate guess and find the one that 
		// produces the lowest score.
		for (auto it = candidates.begin(); it != candidates.end(); ++it)
		{
			Codeword guess = *it;
			FeedbackFrequencyTable freq = 
				e.frequency(e.compare(guess, possibilities));

			// Compute a score of the partition.
			score_type score = h.compute(freq);

			// Store the score if requested.
			if (scores)
				*(scores++) = score;
			
			// Keep track of the guess that produces the lowest score.
#if 0
			if ((it == candidates.begin()) || (score < choice_score) || 
				(score == choice_score && !choice_ispos && freq[target] > 0)) 
#else
			if ((it == candidates.begin()) || (score < choice_score) || 
				(!(choice_score < score) && !choice_ispos && freq[target] > 0)) 
#endif
			{
				choice = guess;
				choice_score = score;
				choice_ispos = (freq[target] > 0);
			}
		}
		return choice;
	}

	virtual Codeword make_guess(
		CodewordConstRange possibilities, 
		CodewordConstRange candidates) const
	{
		return make_guess(possibilities, candidates, NULL);
	}
};

namespace Heuristics {
		
/// A conservative heuristic that scores a guess as the worst-case 
/// number of remaining possibilities (Knuth, 1976). 
/// The score to minimize is <code>Max{ n[i] }</code>, where 
/// <code>n[i]</code> is the number of elements in partition 
/// <code>i</code>.
template <int Levels>
struct MinimizeWorstCase
{
	static_assert(Levels == 1, "Only Levels == 1 is supported at present.");

	/// Data type of the score (unsigned integer).
	typedef unsigned int score_t;

	/// Short identifier of the heuristic function.
	std::string name() const { return "minmax"; }

	/// Computes the heuristic score - size of the largest partition.
	score_t compute(const FeedbackFrequencyTable &freq) const
	{
		return freq.max();
	}
};

/// A balanced heuristic that scores a guess as the expected number 
/// of remaining possibilities (Irving, 1978). The score to minimize
/// is <code>Sum{ n[i] * (n[i]/N) }</code>, or equivalently 
/// <code>Sum{ n[i]^2 }</code>, where <code>n[i]</code> is the number
/// of elements in partition <code>i</code>.
struct MinimizeAverage
{
	/// Data type of the score (unsigned integer).
	typedef unsigned int score_t;

	/// Short identifier of the heuristic function.
	std::string name() const { return "minavg"; }

	/// Computes the heuristic score - sum of squares of the size
	/// of each partition.
	score_t compute(const FeedbackFrequencyTable &freq) const
	{
		unsigned int s = 0;
		for (size_t i = 0; i < freq.size(); ++i)
		{
			unsigned int f = freq[i];
			s += f * f;
		}
		return s;
	}
};

/// A theoretically advanced heuristic that scores a guess as roughly
/// the expected number of further guesses needed (Neuwirth, 1982).
/// The precise definition is to maximize the entropy, defined as
/// <code>-Sum{ (n[i]/N) * log(n[i]/N) }</code>, which is equivalent
/// to minimizing <code>Sum{ (n[i]/N) * log(n[i]) }</code>. If we 
/// interpret <code>log(n[i])</code> as an estimate of the number of 
/// further guesses needed for a partition of size <code>n[i]</code>, 
/// then we can interpret the the objective function as an 
/// estimate of the expected number of further guesses needed. 
/// As a side note, note that the base of the logrithm doesn't matter 
/// in computing the score.
template <bool ApplyCorrection>
struct MaximizeEntropy
{
	/// Data type of the score (double precision).
#if 0
	typedef double score_t;
#else
	typedef util::wrapped_float<double, 100> score_t;
#endif

	/// Short identifier of the heuristic function.
	std::string name() const
	{
		return ApplyCorrection? "entropy*" : "entropy";
	}

	/// Computes the heuristic score - negative of the entropy.
	static score_t compute(const FeedbackFrequencyTable &freq)
	{
		double s = 0.0;
		for (size_t i = 0; i < freq.size(); ++i)
		{
			unsigned int f = freq[i];
			if (f > 1)
			{
				s += std::log((double)f) * (double)f;
			}
		}
		if (ApplyCorrection)
		{
			if (freq[freq.size()-1]) // 4A0B
				s -= 2.0 * std::log(2.0);
		}
		return score_t(s);
	}
};

/// An aggressive heuristic that scores a guess as the number of
/// partitions it produces. The rationale is that more partitions,
/// fewer steps.
struct MaximizePartitions
{
	/// Data type of the score (signed integer).
	typedef int score_t;

	/// Short identifier of the heuristic function.
	static std::string name() { return "maxparts"; }

	/// Computes the heuristic score - negative of the number of
	/// partitions.
	static score_t compute(const FeedbackFrequencyTable &freq)
	{
		return -(int)freq.nonzero_count();
	}
};


} // namespace Heuristics

} // namespace Mastermind

#endif // MASTERMIND_HEURISTIC_STRATEGY_HPP
