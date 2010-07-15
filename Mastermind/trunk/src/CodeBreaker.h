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
		virtual void BuildStrategyTree(StrategyTree *tree);

		/// Not implemented.
		virtual void BuildStrategyTree(StrategyTree *tree, const Codeword& first_guess) = 0;

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
		void FillStrategy(StrategyTree *tree, CodewordList possibilities, const Codeword &force_guess);

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
		virtual void BuildStrategyTree(StrategyTree *tree, const Codeword& first_guess);
	};

	typedef void ProgressReport(double percentage, void *tag);

	/// Heuristic code breaker that evaluates potential guesses according 
	/// to a scoring criteria and picks the one with the higherest score.
	class HeuristicCodeBreaker : public CodeBreaker
	{
	public:

		/// Criteria used by <code>HeuristicCodeBreaker</code> to evaluate
		/// potential guesses. 
		///
		/// Suppose there are
		/// <code>N</code> remaining possibilities. For a given guess,
		/// these possibilities are divided into <code>p</code> partitions,
		/// where the possibilities in each partition have the same feedback.
		/// That is, given that guess, there is no way to distinguish 
		/// one possibility from another in the same partition. 
		///
		/// The heuristic code breaker evaluates each guess by computing
		/// a score on the partitions it produces. Let <code>n[i]</code>
		/// be the number of possibilities in partition <code>i</code>,
		/// where <code>i=1..p</code>, and <code>n[1]+...+n[p]=N</code>. 
		/// The scoring formulas are described below. In these formulas
		/// we assume that the possibilities are uniformly distributed.
		enum HeuristicCriteria
		{
			/// Minimize the number of remaining possibilities in the worst-case
			/// scenario (Knuth, 1976). The score to minimize is 
			/// <code>Max{ n[i] }</code>, i.e. the number of elements in
			/// the largest partition.
			MinimizeWorstCase = 1,

			/// Minimize the expected number of remaining possibilities
			/// (Irving, 1978). The score to minimize is
			/// <code>Sum{ n[i] * (n[i]/N) }</code>, or equivalently 
			/// <code>Sum{ n[i]^2 }</code>, i.e. the sum of squares of
			/// the number of elements in each partition.
			MinimizeAverage = 2,

			/// Maximize entropy, which is roughly equivalently to minimizing
			/// the expected number of further guesses needed (Neuwirth, 1982).
			/// The score to maximize is 
			/// <code>-Sum{ (n[i]/N) * log(n[i]/N) }</code>,
			/// which is equivalent to minimizing 
			/// <code>Sum{ (n[i]/N) * log(n[i]) }</code>. If we interpret
			/// <code>log(n[i])</code> as an estimate of the number of 
			/// further guesses needed for a partition of size <code>n[i]</code>, 
			/// then we can interpret the the objective function as an 
			/// estimate of the expected number of further guesses needed. 
			/// By the way, note that the base of the logrithm doesn't matter 
			/// in computing the score.
			MaximizeEntropy = 3,

			/// Maximize the number of partitions (Kooi, 2005).
			/// The score to maximize is <code>p</code>.
			MaximizeParts = 4,

			/// Use the default criteria; in this implementation, the default 
			/// criteria is chosen as <code>MinimizeAverage</code>.
			DefaultCriteria = 0,
		};

	private:
		/// Criteria used to evaluate and score each potential guess.
		HeuristicCriteria m_criteria;

		/// Whether to pick a guess only from remaining possibilities.
		/// If set to false, a guess is picked from all codewords.
		bool m_posonly;

		void FillStrategy(
			StrategyTree *tree, 
			CodewordList possibilities,
			unsigned short unguessed_mask,
			unsigned short impossible_mask,
			const Codeword& first_guess,
			int *progress);

	public:

		/// Creates a heuristic code breaker.
		HeuristicCodeBreaker(
			CodewordRules rules,
			HeuristicCriteria criteria = DefaultCriteria,
			bool posonly = false) 
			: CodeBreaker(rules), m_criteria(criteria), m_posonly(posonly) 
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

		/// Not implemented.
		virtual void BuildStrategyTree(StrategyTree *tree, const Codeword& first_guess);
	};

	/// Code breaker that finds the optimal guess by Depth-First Search.
	class OptimalCodeBreaker : public CodeBreaker
	{
	private:
		int SearchLowestSteps(
			CodewordList possibilities,
			unsigned short unguessed_mask,
			unsigned short impossible_mask,
			int *depth,
			Codeword *choice);

		void FillStrategy(
			StrategyTree *tree, 
			CodewordList possibilities,
			unsigned short unguessed_mask,
			unsigned short impossible_mask,
			const Codeword& first_guess,
			int *progress);

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
		virtual void BuildStrategyTree(StrategyTree *tree, const Codeword& first_guess);
	};

}