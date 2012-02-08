#ifndef MASTERMIND_ALGORITHM_HPP
#define MASTERMIND_ALGORITHM_HPP

#include <string>
#include "Codeword.hpp"
#include "Feedback.hpp"
#include "Registry.hpp"

namespace Mastermind {

/// Compares an array of guesses to the secret.
/// @param[in]	secret	The secret codeword
/// @param[in]	guesses	An array of guesses
/// @param[in]	count	Number of guesses in the array
/// @param[out]	results	An array to store feedbacks
typedef void (*ComparisonRoutine)(
	const CodewordRules &rules,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[]);

/// Counts the frequencies of each feedback in a feedback list.
/// Counts the frequencies that each feedback occurs in a feedback list.
/// @param[in]	feedbacks	A list of feedbacks to count frequencies on
/// @param[in]	count		Number of feedbacks in the list
/// @param[out]	freq		The frequency table
/// @param[in]	max_fb		The maximum feedback value allowed
typedef void (*FrequencyRoutine)(
	const unsigned char *first,
	const unsigned char *last,
	unsigned int freq[],
	unsigned char max_fb);

/// Computes the sum of squares of a frequency table.
/// @param[in]	freq	The frequency table to compute statistic on
/// @param[in]	max_fb	The maximum feedback value allowed
typedef unsigned int (*SumSquaresRoutine)(
	const unsigned int *first,
	const unsigned int *last);

/// Generates all codewords that conforms to the given set of rules.
typedef size_t (*GenerationRoutine)(
	const CodewordRules &rules,
	Codeword *results);

/**
 * Filters a list of codewords <code>[first,last)</code> by removing
 * duplicate elements according
 * to equivalence class <code>eqclass</code>.
 * Returns the number of (canonical) elements remaining.
 *
 * _eqclass_ is a 16-byte array where each element specifies the next digit
 * in the same equivalent class. Hence, each equivalence class is chained
 * through a loop. For example: (assume only 10 digits)
 *                  0  1  2  3  4  5  6  7  8  9
 *  all-different:  0  1  2  3  4  5  6  7  8  9
 *  all-same:       1  2  3  4  5  6  7  8  9  0
 *  1,3,5 same:     0  3  2  5  4  1  6  7  8  9
 *
 * This function keeps the lexicographical-minimum codeword of each
 * equivalence class. This process is known as "canonical labeling". 
 * Note that the minimum codeword for each equivalence class must exist 
 * in the list in order for the function to work correctly.
 */
typedef size_t (*EquivalenceRoutine)(
	const Codeword *first,
	const Codeword *last,
	const unsigned char eqclass[16],
	Codeword *filtered);

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

#if 0
FeedbackFrequencyTable frequency(const FeedbackList &feedbacks);
{
	FeedbackFrequencyTable table;
	countFrequencies(feedbacks.begin(), feedbacks.end(), table);
	return table;
}
#endif

template <class Routine>
struct RoutineRegistry : public Utilities::Registry<std::string, Routine>
{
};

#define REGISTER_ROUTINE(type,id,item) REGISTER_ITEM(type,id,item)

} // namespace Mastermind

#endif // MASTERMIND_ALGORITHM_HPP
