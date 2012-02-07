#ifndef MASTERMIND_ALGORITHM_HPP
#define MASTERMIND_ALGORITHM_HPP

#include "Codeword.hpp"
#include "CodewordRules.hpp"
#include "CodewordList.hpp"
#include "Feedback.h"
#include "Registry.hpp"

namespace Mastermind {

/// Compares an array of guesses to the secret.
/// @param[in]	secret	The secret codeword
/// @param[in]	guesses	An array of guesses
/// @param[in]	count	Number of guesses in the array
/// @param[out]	results	An array to store feedbacks
typedef void (*ComparisonRoutine)(
	const CodewordRules &rules,
	const Codeword& secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[]);

/// Returns a bit-mask of the colors that are present in the codeword.
unsigned short getDigitMask(const Codeword &c);

/// Returns a bit-mask of all the colors that are present in a list 
/// of codewords.
unsigned short getDigitMask(
	CodewordList::const_iterator first, 
	CodewordList::const_iterator last);

#if 0
/// Returns a bit-mask of all the colors that are present in a list 
/// of codewords.
unsigned short getDigitMask(const CodewordList &list);
#endif

/// Compares two codewords.
/// @returns The feedback.
Feedback compare(
	const CodewordRules &rules, 
	const Codeword& guess, 
	const Codeword& secret);

/// Compares a codeword to a list of codewords.
/// @returns A list of feedbacks.
FeedbackList compare(
	const CodewordRules &rules, 
	const Codeword& guess,
	CodewordList::const_iterator first,
	CodewordList::const_iterator last);

/// ???
CodewordList filterByFeedback(
	const CodewordList &list,
	const CodewordRules &rules, 
	const Codeword &guess, 
	Feedback feedback);

/// Partitions a list of codewords by their feedback when compared to
/// a given guess.
///
/// The codewords in the list are re-ordered in-place, such that codewords
/// with the same feedback when compared to @c guess are stored 
/// consecutively.
///
/// The feedback frequency table is returned as a by-product.
void partition(
	CodewordList::iterator first, 
	CodewordList::iterator last,
	const CodewordRules &rules,
	const Codeword &guess, 
	FeedbackFrequencyTable &freq);

/// Counts the frequencies of each feedback in a feedback list.
void countFrequencies(
	const CodewordRules &rules,
	FeedbackList::const_iterator first,
	FeedbackList::const_iterator last,
	FeedbackFrequencyTable &freq);

template <class Routine>
struct RoutineRegistry : public Utilities::Registry<std::string, Routine>
{
};

#define REGISTER_ROUTINE(type,id,item) REGISTER_ITEM(type,id,item)

} // namespace Mastermind

#endif // MASTERMIND_ALGORITHM_HPP
