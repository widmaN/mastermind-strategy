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
	ComparisonRoutine* _compare;
	GenerationRoutine _generate;
	MaskRoutine _mask;
	CodewordList _all;

public:

	Engine(const Rules &rules) : _rules(rules),
		_compare(rules.repeatable()? 
			compare_codewords_generic : compare_codewords_norepeat),
		_generate(RoutineRegistry<GenerationRoutine>::get("generic")),
		_mask(RoutineRegistry<MaskRoutine>::get("generic")),
		_all(generateCodewords())
	{
	}

	const Rules& rules() const { return _rules; }

	CodewordConstRange universe() const { return _all; }

	/// Compares two codewords and returns the feedback.
	Feedback compare(const Codeword& guess, const Codeword& secret) const
	{
		Feedback feedback;
		_compare(guess, &secret, 1, &feedback, 0);
		return feedback;
	}

	/// Compares a codeword to a list of codewords and returns the 
	/// feedback frequencies.
	FeedbackFrequencyTable compare(
		const Codeword &guess, 
		CodewordConstRange secrets) const
	{
		if (secrets.empty())
			return FeedbackFrequencyTable();

		FeedbackFrequencyTable freq(Feedback::size(rules()));
		_compare(guess, &secrets[0], secrets.size(), 0, freq.data());
		return freq;
	}

	/// Compares a codeword to a list of codewords and returns the 
	/// feedbacks as well as their frequencies.
	FeedbackFrequencyTable compare(
		const Codeword &guess, 
		CodewordConstRange secrets,
		FeedbackList &feedbacks) const
	{
		feedbacks.resize(secrets.size());
		if (secrets.empty())
			return FeedbackFrequencyTable();

		FeedbackFrequencyTable freq(Feedback::size(rules()));
		_compare(guess, &secrets[0], secrets.size(), feedbacks.data(), freq.data());
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
