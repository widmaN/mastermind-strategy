//////////////////////////////////////////////////////////////
// Higher-level routines for manipulating codewords.
//

#ifndef MASTERMIND_ENGINE_HPP
#define MASTERMIND_ENGINE_HPP

#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"
#include "Algorithm.hpp"

#include <vector>
#include "util/aligned_allocator.hpp"
#include "util/pool_allocator.hpp"
#include "util/frequency_table.hpp"
#include "util/range.hpp"

namespace Mastermind {

///////////////////////////////////////////////////////////////////////////
// Definition of CodewordList and related types.

typedef 	std::vector<Codeword,util::aligned_allocator<Codeword,16>> CodewordList;

typedef CodewordList::iterator CodewordIterator;
typedef CodewordList::const_iterator CodewordConstIterator;

typedef util::range<CodewordList::iterator> CodewordRange;
typedef util::range<CodewordList::const_iterator> CodewordConstRange;

///////////////////////////////////////////////////////////////////////////
// Definition of FeedbackList.

typedef std::vector<Feedback,util::pool_allocator<Feedback>> FeedbackList;

///////////////////////////////////////////////////////////////////////////
// Definition of FeedbackFrequencyTable.

typedef util::frequency_table<Feedback,unsigned int,256> FeedbackFrequencyTable;

///////////////////////////////////////////////////////////////////////////
// Definition of Engine.

class Engine
{
	Rules _rules;
	ComparisonRoutine _compare;
	FrequencyRoutine _freq;
	GenerationRoutine _generate;
	MaskRoutine _mask;

public:

	Engine(const Rules &rules) : _rules(rules),
		_compare(RoutineRegistry<ComparisonRoutine>::get("generic")),
		_freq(RoutineRegistry<FrequencyRoutine>::get("generic")),
		_generate(RoutineRegistry<GenerationRoutine>::get("generic")),
		_mask(RoutineRegistry<MaskRoutine>::get("generic"))
	{
	}

	const Rules& rules() const { return _rules; }

	void select(ComparisonRoutine f) { _compare = f; }

#if 0
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
#else
	
	/// Compares a codeword to a list of codewords.
	/// @returns A list of feedbacks.
	/// @timecomplexity <code>O(N)</code>.
	/// @spacecomplexity <code>O(N)</code>.
	FeedbackList compare(
		const Codeword &guess, 
		CodewordConstRange codewords) const
	{
		size_t count = codewords.size();
		FeedbackList feedbacks(count);
		if (count > 0)
		{
			const Codeword *first = &(*codewords.begin());
			_compare(_rules, guess, first, first + count, feedbacks.data());
		}
		return feedbacks;
	}
#endif

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

	/// Generates all codewords that conforms to the given set of rules.
	CodewordList generateCodewords() const 
	{
		// Call the generation routine once to get the count.
		const size_t count = _generate(_rules, NULL);

		// Create an empty list.
		// @todo: do not initialize Codewords.
		CodewordList list(count);

		// Call the generation routine again to fill in the codewords.
		_generate(_rules, list.data());
		return list;
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

	FeedbackFrequencyTable frequency(const FeedbackList &feedbacks) const
	{
		FeedbackFrequencyTable table;
		countFrequencies(feedbacks.begin(), feedbacks.end(), table);
		return table;
	}

	/// Returns a bit-mask of the colors that are present in the codeword.
	unsigned short getDigitMask(const Codeword &c) const
	{
		return _mask(&c, &c + 1);
	}

#if 0
	/// Returns a bit-mask of the colors that are present in a list of
	/// codewords.
	unsigned short getDigitMask(
		CodewordList::const_iterator first, 
		CodewordList::const_iterator last) const
	{
		size_t count = last - first;
		if (count == 0)
			return 0;
		else
			return _mask(&(*first), &(*first) + count);
	}
#endif

	/// Returns a bit-mask of the colors that are present in a list of
	/// codewords.
	unsigned short color_mask(CodewordConstRange codewords) const
	{
		if (codewords.empty())
		{
			return 0;
		}
		else
		{
			const Codeword *first = &(*codewords.begin());
			return _mask(first, first + codewords.size());
		}
	}
};

} // namespace Mastermind

#endif // MASTERMIND_ENGINE_HPP
