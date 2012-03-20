//////////////////////////////////////////////////////////////
// Higher-level routines for manipulating codewords.
//

#ifndef MASTERMIND_ENGINE_HPP
#define MASTERMIND_ENGINE_HPP

#include <cassert>
#include <vector>

#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"
#include "Algorithm.hpp"

#include "util/aligned_allocator.hpp"
#include "util/frequency_table.hpp"
#include "util/range.hpp"
#include "util/partition.hpp"
#include "util/bitmask.hpp"
#include "util/simd.hpp"

namespace Mastermind {

///////////////////////////////////////////////////////////////////////////
// Definition of CodewordList and related types.

typedef std::vector<Codeword,util::aligned_allocator<Codeword,16>> CodewordList;

typedef CodewordList::iterator CodewordIterator;
typedef CodewordList::const_iterator CodewordConstIterator;

typedef util::range<CodewordList::iterator> CodewordRange;
typedef util::range<CodewordList::const_iterator> CodewordConstRange;

///////////////////////////////////////////////////////////////////////////
// Definition of FeedbackList.

typedef std::vector<Feedback> FeedbackList;

///////////////////////////////////////////////////////////////////////////
// Definition of FeedbackFrequencyTable.

typedef util::frequency_table<Feedback,unsigned int,Feedback::MaxOutcomes>
	FeedbackFrequencyTable;

///////////////////////////////////////////////////////////////////////////
// Definition of FeedbackFrequencyTable.

typedef util::partition_cells<CodewordIterator,Feedback::MaxOutcomes>
	CodewordPartition;

///////////////////////////////////////////////////////////////////////////
// Definition of ColorMask.

#if 1 // no performance difference
typedef util::bitmask<unsigned short, MM_MAX_COLORS> ColorMask;
#else
typedef util::bitmask<unsigned int, MM_MAX_COLORS> ColorMask;
#endif

///////////////////////////////////////////////////////////////////////////
// Definition of Engine.

/// Defines a set of algorithms associated with a specific set of rules.
/// @ingroup algo
class Engine
{
	Rules _rules;
	ComparisonRoutine _compare;
	GenerationRoutine _generate;
	MaskRoutine _mask;
	CodewordList _all;

public:

	/// Constructs an algorithm engine for the given rules.
	Engine(const Rules &rules) : _rules(rules),
#if 0
		_compare(rules.repeatable()? 
			compare_codewords_generic : compare_codewords_norepeat),
#else
		_compare(RoutineRegistry<ComparisonRoutine>::get(
			rules.repeatable()? "generic" : "norepeat")),
#endif
		_generate(RoutineRegistry<GenerationRoutine>::get("generic")),
		_mask(RoutineRegistry<MaskRoutine>::get("generic")),
		_all(generateCodewords())
	{
	}

	/// Returns the underlying rules of this engine.
	const Rules& rules() const { return _rules; }

	/// Returns a range of all codewords for the underlying rules.
	CodewordConstRange universe() const { return _all; }

	/// Compares two codewords and returns the feedback.
	Feedback compare(const Codeword& guess, const Codeword& secret) const
	{
		Feedback feedback;
		_compare(guess, &secret, 1, &feedback, 0);
		return feedback;
	}

	/// Compares a codeword to a list of codewords and returns the feedback
	/// frequencies.
	FeedbackFrequencyTable compare(
		const Codeword &guess, 
		CodewordConstRange secrets) const
	{
		// Note: it is critical to structure the code in a way to enable
		// "Named Return Value Optimization" for the compier. This can 
		// lead to 7-10% performance difference.
		assert(!secrets.empty());
		FeedbackFrequencyTable freq(Feedback::size(rules()));
		_compare(guess, &secrets[0], secrets.size(), 0, freq.data());
		return freq;
	}

	/// Compares a codeword to a list of codewords and returns the feedbacks
	/// as well as their frequencies.
	FeedbackFrequencyTable compare(
		const Codeword &guess, 
		CodewordConstRange secrets,
		FeedbackList &feedbacks) const
	{
		assert(!secrets.empty());
		feedbacks.resize(secrets.size());
		FeedbackFrequencyTable freq(Feedback::size(rules()));
		_compare(guess, &secrets[0], secrets.size(), feedbacks.data(), freq.data());
		return freq;
	}

	/// Generates all codewords for the underlying set of rules.
	CodewordList generateCodewords() const 
	{
		// Call the generation routine once to get the count.
		const size_t count = _generate(_rules, NULL);

		// Create an empty list.
		CodewordList list(count);

		// Call the generation routine again to fill in the codewords.
		_generate(_rules, list.data());
		return list;
	}

	/// Returns the elements from match the given response when compared
	/// to the given guess.
	CodewordList filterByFeedback(
		const CodewordList &list,
		const Codeword &guess, 
		const Feedback &response) const;

	/**
	 * Partitions a list of codewords by their response when compared to
	 * a given guess. The codewords in the list are re-ordered in-place
	 * such that codewords with the same feedback are stored consecutively.
	 * In addition, the partition is stable, i.e. any two codewords with 
	 * the same feedback will retain the same relative order.
	 */
	CodewordPartition partition(
		CodewordRange codewords, 
		const Codeword &guess) const;

	/// Returns a bit-mask of the colors that are present in the codeword.
	ColorMask colorMask(const Codeword &c) const
	{
		int mask_absent = util::simd::byte_mask(
			*(util::simd::simd_t<uint8_t,16>*)(&c) == (uint8_t)0);
		int mask_present = ~mask_absent & ((1 << MM_MAX_COLORS) - 1);
		return ColorMask((ColorMask::value_type)mask_present);
	}

	/// Returns a bit-mask of the colors that are present in any of the
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
