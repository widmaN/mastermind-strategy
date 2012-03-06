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
//#include <bitset>
#include "util/aligned_allocator.hpp"
#include "util/pool_allocator.hpp"
#include "util/frequency_table.hpp"
#include "util/range.hpp"
#include "util/bitmask.hpp"

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

//typedef util::frequency_table<Feedback,unsigned int,256> FeedbackFrequencyTable;
typedef 
	util::frequency_table<Feedback,unsigned int,Feedback::MaxOutcomes>
	FeedbackFrequencyTable;

///////////////////////////////////////////////////////////////////////////
// Definition of ColorMask.

typedef util::bitmask<MM_MAX_COLORS> ColorMask;

///////////////////////////////////////////////////////////////////////////
// Definition of Engine.

class Engine
{
	Rules _rules;
	//ComparisonRoutine _compare;
	FrequencyRoutine _freq;
	GenerationRoutine _generate;
	MaskRoutine _mask;
	CodewordList _all;

public:

	Engine(const Rules &rules) : _rules(rules),
		//_compare(RoutineRegistry<ComparisonRoutine>::get(
		//	_rules.repeatable()? "generic" : "norepeat")),
		_freq(RoutineRegistry<FrequencyRoutine>::get("generic")),
		_generate(RoutineRegistry<GenerationRoutine>::get("generic")),
		_mask(RoutineRegistry<MaskRoutine>::get("generic")),
		_all(generateCodewords())
	{
	}

	const Rules& rules() const { return _rules; }

	CodewordConstRange universe() const { return _all; }

	//void select(ComparisonRoutine f) { _compare = f; }

	/// Compares two codewords and returns the feedback.
	Feedback compare(const Codeword& guess, const Codeword& secret) const
	{
		Feedback feedback;
		compare_codewords(_rules, guess, &secret, 1, &feedback);
		return feedback;
	}

#if 0
	/// Compares a codeword to a list of codewords and returns the feedbacks.
	FeedbackList compare(const Codeword &guess, CodewordConstRange codewords) const
	{
		size_t count = codewords.size();
		FeedbackList feedbacks(count);
		if (count > 0)
		{
			const Codeword *first = &(*codewords.begin());
			compare_codewords(_rules, guess, first, count, feedbacks.data());
		}
		return feedbacks;
	}
#endif

	/// Compares a codeword to a list of codewords and returns the feedback
	/// frequencies. Optionally stores the feedbacks.
	FeedbackFrequencyTable compare(
		const Codeword &guess, 
		CodewordConstRange secrets,
		Feedback *feedbacks) const
	{
		FeedbackFrequencyTable freq;
		size_t fb_size = Feedback::size(rules());
		freq.resize(fb_size);
		size_t count = secrets.size();
		const Codeword *first = &(*secrets.begin());
		if (feedbacks)
		{
			compare_codewords(_rules, guess, first, count, 
				feedbacks, freq.data(), freq.size());
		}
		else
		{
			compare_codewords(_rules, guess, first, count, 
				freq.data(), freq.size());
		}
		return freq;
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
		const Feedback &feedback) const;

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
	FeedbackFrequencyTable partition(
		CodewordRange codewords, 
		const Codeword &guess) const;

#if 0
	/// Counts the frequencies of each feedback in a feedback list.
	void countFrequencies(
		FeedbackList::const_iterator first,
		FeedbackList::const_iterator last,
		FeedbackFrequencyTable &freq) const;

	FeedbackFrequencyTable frequencies(
		const Codeword &guess, 
		CodewordConstRange codewords) const;
#endif

#if 0
	FeedbackFrequencyTable frequency(const FeedbackList &feedbacks) const
	{
		FeedbackFrequencyTable table;
		countFrequencies(feedbacks.begin(), feedbacks.end(), table);
		return table;
	}
#endif

	/// Returns a bit-mask of the colors that are present in the codeword.
	ColorMask colorMask(const Codeword &c) const
	{
		return ColorMask(_mask(&c, &c + 1));
	}

	/// Returns a bit-mask of the colors that are present in a list of
	/// codewords.
	ColorMask colorMask(CodewordConstRange codewords) const
	{
		if (codewords.empty())
		{
			return ColorMask();
		}
		else
		{
			const Codeword *first = &(*codewords.begin());
			return ColorMask(_mask(first, first + codewords.size()));
		}
	}
};

} // namespace Mastermind

#endif // MASTERMIND_ENGINE_HPP
