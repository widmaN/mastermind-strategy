#ifndef MASTERMIND_ENVIRONMENT_HPP
#define MASTERMIND_ENVIRONMENT_HPP

#include "CodewordRules.hpp"
#include "Codeword.hpp"
#include "CodewordList.hpp"
#include "Feedback.h"
#include "Registry.hpp"
#include "Algorithm.hpp"

namespace Mastermind {

class Environment
{
	CodewordRules _rules;
	ComparisonRoutine _compare;
	FrequencyRoutine _freq;

public:

	Environment(const CodewordRules &rules)	: _rules(rules),
		_compare(RoutineRegistry<ComparisonRoutine>::get("generic")),
		_freq(RoutineRegistry<FrequencyRoutine>::get("generic"))
	{
	}

	const CodewordRules& rules() const { return _rules; }

	void select(ComparisonRoutine f) { _compare = f; }

	/// Compares a codeword to a list of codewords.
	/// @returns A list of feedbacks.
	/// @timecomplexity <code>O(N)</code>.
	/// @spacecomplexity <code>O(N)</code>.
	FeedbackList compare(
		const Codeword &guess, 
		CodewordList::const_iterator first,
		CodewordList::const_iterator last) const
	{
		size_t count = last - first;
		FeedbackList feedbacks(count);
		if (count > 0)
		{
			_compare(_rules, guess, &(*first), &(*first)+count, feedbacks.data());
		}
		return feedbacks;
	}

	/// Compares two codewords.
	/// @returns The feedback.
	/// @timecomplexity Constant.
	/// @spacecomplexity Constant.
	Feedback compare(
		const Codeword& guess,
		const Codeword& secret) const
	{
		Feedback feedback;
		_compare(_rules, guess, &secret, &secret + 1, &feedback);
		return feedback;
	}

	CodewordList filterByFeedback(
		const CodewordList &list,
		const Codeword &guess, 
		Feedback feedback) const;

	/**
	 * Partitions a list of codewords by their feedback when compared to
	 * a given guess.
	 *
	 * The codewords in the list are re-ordered in-place, such that codewords
	 * with the same feedback when compared to @c guess are stored 
	 * consecutively.
	 *
	 * The feedback frequency table is returned as a by-product.
	 */
	void partition(
		CodewordList::iterator first,
		CodewordList::iterator last,
		const Codeword &guess,
		FeedbackFrequencyTable &freq) const;

	/// Counts the frequencies of each feedback in a feedback list.
	void countFrequencies(
		FeedbackList::const_iterator first,
		FeedbackList::const_iterator last,
		FeedbackFrequencyTable &freq) const;
};

} // namespace Mastermind

#endif // MASTERMIND_ENVIRONMENT_HPP
