#ifndef MASTERMIND_HEURISTIC_STRATEGY_HPP
#define MASTERMIND_HEURISTIC_STRATEGY_HPP

#include "Strategy.hpp"
#include "util/wrapped_float.hpp"
#include "util/call_counter.hpp"

/**
 * Define FAVOR_POSSIBILITY to 1 to make a heuristic strategy favor
 * a guess that is among the remaining possibilities when multiple
 * guesses have the same heuristic score.
 *
 * Normally, this flag should be set to 0 because we expect the
 * heuristic score to fully capture the available information from
 * the partition. However, setting this to 1 does improve the result
 * marginally in certain cases, e.g. "-r bc -s entropy". Also it
 * may help in compatilibity with legacy implementations that favor
 * a remaining possibility as guess in case of a tie.
 *
 * @ingroup Heuristic
 */
#define FAVOR_POSSIBILITY 0

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
 *
 * @ingroup Heuristic
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

	/// Makes the guess that produces the lowest heuristic score.
	/// Optionally stores the score of all candidates in an array.
	Codeword make_guess(
		CodewordConstRange possibilities,
		CodewordConstRange candidates,
		score_type *scores) const
	{
		REGISTER_CALL_COUNTER(EvaluateHeuristic_Possibilities);
		REGISTER_CALL_COUNTER(EvaluateHeuristic_Candidates);
		UPDATE_CALL_COUNTER(EvaluateHeuristic_Possibilities, (unsigned int)possibilities.size());
		UPDATE_CALL_COUNTER(EvaluateHeuristic_Candidates, (unsigned int)candidates.size());

		if (candidates.empty())
			return Codeword::emptyValue();

		struct choice_t
		{
			typedef typename Heuristic::score_t score_t;

			int i; // index to the choice, -1 = undefined
			typename Heuristic::score_t score; // score of the choice
#if FAVOR_POSSIBILITY
			bool ispos; // whether the choice is a remaining possibility
			choice_t() : i(-1), score(), ispos(false) { }
			choice_t(int _i, const score_t _score, bool _ispos)
				: i(_i), score(_score), ispos(_ispos) { }
#else
			choice_t() : i(-1), score() { }
			choice_t(int _i, const score_t &_score)
				: i(_i), score(_score) { }
#endif

			bool operator < (const choice_t &other) const
			{
				if (i < 0)
					return false;
				if (other.i < 0)
					return true;
				if (score < other.score)
					return true;
				if (other.score < score)
					return false;
#if FAVOR_POSSIBILITY
				if (!ispos && other.ispos)
					return true;
				if (ispos && !other.ispos)
					return false;
#endif
				return i < other.i;
			}
		};

#if _OPENMP
		choice_t global_choice;
#else
		choice_t choice;
#endif

#if FAVOR_POSSIBILITY
		size_t target = Feedback::perfectValue(e.rules()).value();
#endif

		// Evaluate each candidate guess and find the one that
		// produces the lowest score.
		int n = (int)candidates.size();

		// OpenMP index variable (i) must have signed integer type.
#if _OPENMP
		#pragma omp parallel
		{
			choice_t choice;

			#pragma omp for schedule(static)
#endif
			for (int i = 0; i < n; ++i)
			{
				Codeword guess = candidates[i];
				FeedbackFrequencyTable freq = e.frequencies(guess, possibilities);

				// Compute a score of the partition.
				score_type score = h.compute(freq);

				// Store the score if requested.
				if (scores)
					scores[i] = score;

				// Keep track of the guess that produces the lowest score.
#if FAVOR_POSSIBILITY
				choice_t current(i, score, freq[target] > 0);
#else
				choice_t current(i, score);
#endif
				choice = std::min(choice, current);
			}
#if _OPENMP
			#pragma omp critical
			{
				global_choice = std::min(global_choice, choice);
			}
		}
		return candidates[global_choice.i];
#else
		return candidates[choice.i];
#endif
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
/// @ingroup Heuristic
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

/// Heuristic that scores a guess by the expected number of remaining
/// possibilities (Irving, 1979). The score to minimize is
/// <code>Sum{ n[i] * (n[i]/N) }</code>, or equivalently
/// <code>Sum{ n[i]^2 }</code>, where <code>n[i]</code> is the number
/// of elements in partition <code>i</code>.
/// @ingroup Heuristic
struct MinimizeAverage
{
	/// Type of the score (64-bit integer to avoid overflow if the
	/// partition size is larger than 2^16).
	typedef unsigned long long score_t;

	/// Flag indicating whether to make an adjustment to the score
	/// if the guess is among the remaining possibilities.
	bool apply_correction;

	/// Constructs the heuristic using the given policy.
	MinimizeAverage(bool _apply_correction = true)
		: apply_correction(_apply_correction) { }

	/// Name of the heuristic.
	std::string name() const
	{
		return apply_correction? "minavg" : "minavg~";
	}

	/// Computes the heuristic score - sum of squares of the size
	/// of each partition.
	score_t compute(const FeedbackFrequencyTable &freq) const
	{
		score_t s = 0;
		for (size_t i = 0; i < freq.size(); ++i)
		{
			score_t f = freq[i];
			s += f * f;
		}
		if (apply_correction && freq[freq.size()-1])
			--s;
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
/// @ingroup Heuristic
struct MaximizeEntropy
{
#if 0
	/// Type of the score (double precision).
	typedef double score_t;
#else
	/// Type of the score (wrapped double to avoid numerical instability
	/// during comparison).
	typedef util::wrapped_float<double, 100> score_t;
#endif

	/// Flag indicating whether to make an adjustment to the score
	/// if the guess is among the remaining possibilities.
	bool apply_correction;

	/// Constructs the heuristic using the given policy.
	MaximizeEntropy(bool _apply_correction = true)
		: apply_correction(_apply_correction) { }

	/// Short identifier of the heuristic function.
	std::string name() const
	{
		return apply_correction? "entropy" : "entropy~";
	}

	/// Computes the heuristic score - negative of the entropy.
	score_t compute(const FeedbackFrequencyTable &freq) const
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
		if (apply_correction && freq[freq.size()-1]) // 4A0B
		{
			s -= 2.0 * std::log(2.0);
		}
		return score_t(s);
	}
};

/// An aggressive heuristic that scores a guess as the number of
/// partitions it produces. The rationale is that more partitions,
/// fewer steps.
/// @ingroup Heuristic
struct MaximizePartitions
{
	/// Type of the score (signed integer because we need to minimize
	/// the negative value of the number of partitions).
	typedef int score_t;

	/// Flag indicating whether to make an adjustment to the score
	/// if the guess is among the remaining possibilities.
	bool apply_correction;

	/// Constructs the heuristic using the given policy.
	MaximizePartitions(bool _apply_correction = true)
		: apply_correction(_apply_correction) { }

	/// Short identifier of the heuristic function.
	std::string name() const
	{
		return apply_correction? "parts" : "parts~";
	}

	/// Computes the heuristic score - negative of the number of
	/// partitions.
	score_t compute(const FeedbackFrequencyTable &freq) const
	{
		int score = 2 * (int)freq.nonzero_count();
		if (apply_correction && freq[freq.size()-1])
		{
			++score;
		}
		return -score;
	}
};

} // namespace Heuristics

} // namespace Mastermind

#endif // MASTERMIND_HEURISTIC_STRATEGY_HPP
