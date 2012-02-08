#ifndef MASTERMIND_HEURISTIC_CODE_BREAKER_HPP
#define MASTERMIND_HEURISTIC_CODE_BREAKER_HPP

#include <string>
#include "CodeBreaker.h"

namespace Mastermind {

typedef void ProgressReport(double percentage, void *tag);

/// Code breaker that makes a guess that produces the highest heuristic 
/// score.
///
/// The heuristic code breaker makes a guess by scoring each potential 
/// guess according to a heuristic function. The heuristic function 
/// works on the partitioning of the possibility set by the guess.
/// Then the code breaker picks the guess that gets the highest score. 
/// If more than one guesses have the same score, the guess that is 
/// in the possibility set is preferred. If more than one such guesses
/// exist, it is picked in lexicographical order.
///
/// The heuristic function used by the code breaker is supplied in
/// the template parameter <code>Heuristic</code>. It accepts the 
/// frequency table of the partition as input, and returns a heuristic
/// score. The partition is defined as below.
///
/// Suppose there are <code>N</code> remaining possibilities. For 
/// a given guess, these possibilities are divided into <code>p</code> 
/// partitions, where the possibilities in each partition have the 
/// same feedback. That is, given that guess, there is no way to 
/// distinguish one possibility from another in the same partition. 
///
// The heuristic code breaker evaluates each guess by computing
// a score on the partitions it produces. Let <code>n[i]</code>
// be the number of possibilities in partition <code>i</code>,
// where <code>i=1..p</code>, and <code>n[1]+...+n[p]=N</code>. 
// The scoring formulas are described below. In these formulas
// we assume that the possibilities are uniformly distributed.
template <class Heuristic>
class HeuristicCodeBreaker : public CodeBreaker
{
private:
		
	/// Whether to pick a guess only from remaining possibilities.
	/// If set to false, a guess is picked from all codewords.
	bool m_posonly;

private:

	StrategyTreeNode* FillStrategy(
		CodewordList::iterator first_possibility,
		CodewordList::iterator last_possibility,
		unsigned short unguessed_mask,
		unsigned short impossible_mask,
		const Codeword &first_guess,
		int *progress);

	/*
	StrategyTreeNode* FillStrategy(
		CodewordList possibilities,
		unsigned short unguessed_mask,
		unsigned short impossible_mask,
		const Codeword& first_guess,
		int *progress);
		*/

	void MakeGuess(
		//CodewordList possibilities,
		CodewordList::const_iterator first_possibility,
		CodewordList::const_iterator last_possibility,
		unsigned short unguessed_mask,
		unsigned short impossible_mask,
		StrategyTreeState *state);

public:

	/// Creates a heuristic code breaker.
	HeuristicCodeBreaker(
		Environment &e,
		bool posonly = false) 
		: CodeBreaker(e), m_posonly(posonly) 
	{ 
	}

	virtual Codeword MakeGuess(
		CodewordList possibilities,
		unsigned short unguessed_mask,
		unsigned short impossible_mask);

	virtual Codeword MakeGuess();

	virtual std::string name() const { return Heuristic::name(); }

	virtual std::string description() const { return Heuristic::name(); }

	virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess);
};

namespace Heuristics {
		
/// A conservative heuristic that scores a guess as the worst-case 
/// number of remaining possibilities (Knuth, 1976). 
/// The score to minimize is <code>Max{ n[i] }</code>, where 
/// <code>n[i]</code> is the number of elements in partition 
/// <code>i</code>.
struct MinimizeWorstCase
{
	/// Data type of the score (unsigned integer).
	typedef unsigned int score_t;

	/// Short identifier of the heuristic function.
	static std::string name() { return "minmax"; }

	/// Computes the heuristic score - size of the largest partition.
	static score_t compute(const FeedbackFrequencyTable &freq)
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
	static std::string name() { return "minavg"; }

	/// Computes the heuristic score - sum of squares of the size
	/// of each partition.
	static score_t compute(const FeedbackFrequencyTable &freq)
	{
		//return freq.getSumSquares();
		return 1;
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
struct MaximizeEntropy
{
	/// Data type of the score (double precision).
	typedef double score_t;

	/// Short identifier of the heuristic function.
	static std::string name() { return "	entropy"; }

	/// Computes the heuristic score - negative of the entropy.
	static score_t compute(const FeedbackFrequencyTable &freq)
	{
		return freq.entropy();
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
	static std::string name() { return "	maxparts"; }

	/// Computes the heuristic score - negative of the number of
	/// partitions.
	static score_t compute(const FeedbackFrequencyTable &freq)
	{
		return -(int)freq.nonzero_count();
	}
};

#if 0
class MinimizeSteps
{
public:
	/// Data type of the score (signed integer).
	typedef int score_t;

	/// Short identifier of the heuristic function.
	static std::string name() { return "	minsteps"; }

	/// Computes the heuristic score
	static score_t compute(const FeedbackFrequencyTable &freq);

public:
	static int partition_score[10000];
};
#endif

} // namespace Heuristics

} // namespace Mastermind

#endif // MASTERMIND_HEURISTIC_CODE_BREAKER_HPP
