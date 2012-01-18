/// \file CodeBreaker.h
/// Declaration of CodeBreaker classes.

#pragma once

#include <string>
#include <cassert>
#include <emmintrin.h>

#include "StrategyTree.h"

namespace Mastermind 
{
	/// Bases class for code breakers; provides common functionatility
	class CodeBreaker
	{
	private:
		void *m_fp;

	protected:

		/// Rules of the game that this code breaker works on.
		CodewordRules m_rules;

		/// List of all codewords conforming to the rules of the game.
		CodewordList m_all;

		/// List of all possible secrets given the feedback information
		/// so far. At the start of the game, this list is the same as
		/// <code>m_all</code>. But as the code breaker makes guesses and
		/// gets feedback, this list narrows.
		CodewordList m_possibilities;

		//CodewordList m_candidates;

		/// Bit-mask of digits guessed so far, excluding impossible digits.
		unsigned short m_guessed;
		
		/// Bit-mask of digits that have not been guessed so far.
		unsigned short m_unguessed;
		
		/// Bit-mask of digits that are impossible to be in the secret, 
		/// based on the feedback so far.
		unsigned short m_impossible;

	public:
		/// Creates a code breaker for the given rules.
		/// The base implementation initializes all members. In particular,
		/// it fills codeword list <code>m_all</code> with all codewords 
		/// conforming to the rules.
		CodeBreaker(CodewordRules rules);

		/// Destructor.
		/// The base implementation cleans up member variables.
		virtual ~CodeBreaker();

		/// Resets the code breaker so that it is ready to play a new game.
		/// The base implementation resets member variables to the state
		/// when the code breaker is newly created. In particular, codeword
		/// list <code>m_possibilities</code> is set to <code>m_all</code>.
		virtual void Reset();

		/// Provides the code breaker with a piece of feedback for a given
		/// guess.
		/// 
		/// The base implementation updates <code>m_possibilities</code> 
		/// and several other digit masks based on the feedback information
		/// provided.
		///
		/// Note that the guess provided here may not be the guess that the 
		/// code breaker suggests previously (via <code>MakeGuess()</code>).
		/// It might even happen that the code breaker has not been asked 
		/// to make a guess at all. If a particular implementation does not
		/// support such behavior (e.g. one that makes guesses according to
		/// a pre-built strategy tree), it must throw an exception.
		virtual void AddFeedback(Codeword &guess, Feedback fb);

		/// Returns the number of remaining possibilities.
		virtual int GetPossibilityCount() { return m_possibilities.GetCount(); }

		/// Makes a guess.
		/// This function must be overridden by an inherited class. It
		/// must return a codeword as the guess it makes. This function is
		/// the most important function of a code breaker.
		virtual Codeword MakeGuess() = 0;

		/// Not implemented.
		virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess) = 0;

		/// Returns a name identifying the code breaker.
		/// This function must be overridden by an inherited class. It
		/// should return a short description of its algorithm, like
		/// "simple".
		virtual const char* GetName() const = 0;

		/// Returns a description of the code breaker.
		/// This function must be overridden by an inherited class. It
		/// should return a descriptive message of its algorithm, like
		/// "guesses the first remaining possibility".
		virtual std::string GetDescription() const = 0;
	};

	/// Naive code breaker that always guesses the first codeword from
	/// the remaining possibilities.
	class SimpleCodeBreaker : public CodeBreaker
	{
	private:
		Codeword MakeGuess(CodewordList &possibilities);
		StrategyTreeNode* FillStrategy(CodewordList possibilities, const Codeword &force_guess);

	public:
		/// Creates a simple code breaker.
		SimpleCodeBreaker(CodewordRules rules) 
			: CodeBreaker(rules) 
		{ 
		}

		/// Makes a guess.
		/// The simple code breaker always guesses the first codeword from
		/// the remaining possibilities.
		virtual Codeword MakeGuess();

		/// Returns a name identifying the simple code breaker.
		virtual const char* GetName() const;

		/// Returns a description of the simple code breaker.
		virtual std::string GetDescription() const;

		/// Not implemented.
		virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess);
	};

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
	/// exist, it is picked in order.
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
			CodewordList possibilities,
			unsigned short unguessed_mask,
			unsigned short impossible_mask,
			const Codeword& first_guess,
			int *progress);

		void MakeGuess(
			CodewordList possibilities,
			unsigned short unguessed_mask,
			unsigned short impossible_mask,
			StrategyTreeState *state);

	public:

		/// Creates a heuristic code breaker.
		HeuristicCodeBreaker(
			CodewordRules rules,
			bool posonly = false) 
			: CodeBreaker(rules), m_posonly(posonly) 
		{ 
		}

		virtual Codeword MakeGuess(
			CodewordList possibilities,
			unsigned short unguessed_mask,
			unsigned short impossible_mask);

		virtual Codeword MakeGuess();

		/// Returns a description of the heuristic code breaker.
		virtual std::string GetDescription() const;

		virtual const char* GetName() const;

		virtual StrategyTree* BuildStrategyTree(const Codeword& first_guess);
	};

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

}

namespace Mastermind
{
	namespace Heuristics
	{
		/// A conservative heuristic that scores a guess as the worst-case 
		/// number of remaining possibilities (Knuth, 1976). 
		/// The score to minimize is <code>Max{ n[i] }</code>, where 
		/// <code>n[i]</code> is the number of elements in partition 
		/// <code>i</code>.
		class MinimizeWorstCase
		{
		public:
			/// Data type of the score (unsigned integer).
			typedef unsigned int score_t;
			/// Short identifier of the heuristic function.
			static const char *name;
			/// Computes the heuristic score - size of the largest partition.
			static score_t compute(const FeedbackFrequencyTable &freq);
		};

		/// A balanced heuristic that scores a guess as the expected number 
		/// of remaining possibilities (Irving, 1978). The score to minimize
		/// is <code>Sum{ n[i] * (n[i]/N) }</code>, or equivalently 
		/// <code>Sum{ n[i]^2 }</code>, where <code>n[i]</code> is the number
		/// of elements in partition <code>i</code>.
		class MinimizeAverage
		{
		public:
			/// Data type of the score (unsigned integer).
			typedef unsigned int score_t;
			/// Short identifier of the heuristic function.
			static const char *name;
			/// Computes the heuristic score - sum of squares of the size
			/// of each partition.
			static score_t compute(const FeedbackFrequencyTable &freq);
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
		class MaximizeEntropy
		{
		public:
			/// Data type of the score (double precision).
			typedef double score_t;
			/// Short identifier of the heuristic function.
			static const char *name;
			/// Computes the heuristic score - negative of the entropy.
			static score_t compute(const FeedbackFrequencyTable &freq);
		};

		/// An aggressive heuristic that scores a guess as the number of
		/// partitions it produces. The rationale is that more partitions,
		/// fewer steps.
		class MaximizePartitions
		{
		public:
			/// Data type of the score (signed integer).
			typedef int score_t;
			/// Short identifier of the heuristic function.
			static const char *name;
			/// Computes the heuristic score - negative of the number of
			/// partitions.
			static score_t compute(const FeedbackFrequencyTable &freq);
		};

		class MinimizeSteps
		{
		public:
			/// Data type of the score (signed integer).
			typedef int score_t;
			/// Short identifier of the heuristic function.
			static const char *name;
			/// Computes the heuristic score
			static score_t compute(const FeedbackFrequencyTable &freq);

		public:
			static int partition_score[10000];
		};
	}
}
