#ifndef MASTERMIND_ALGORITHM_HPP
#define MASTERMIND_ALGORITHM_HPP

#include <string>
#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"
#include "Registry.hpp"

namespace Mastermind {

/// Compares a secret to a list of guesses and stores the feedbacks.
void compare_codewords(
	const Rules &rules,
	const Codeword &secret,
	const Codeword *guesses,
	size_t count,
	Feedback *result);

/// Compares a secret to a list of guesses and stores the feedback 
/// frequencies.
void compare_codewords(
	const Rules &rules,
	const Codeword &secret,
	const Codeword *guesses,
	size_t count,
	unsigned int *freq,
	size_t size);

/// Compares a secret to a list of guesses and stores the feedback 
/// as well as their frequencies.
void compare_codewords(
	const Rules &rules,
	const Codeword &secret,
	const Codeword *guesses,
	size_t count,
	Feedback *result,
	unsigned int *freq,
	size_t size);

/// Compares an array of guesses to the secret.
/// @param[in]  rules   Game rules.
/// @param[in]	secret  The secret.
/// @param[in]	first   First guess.
/// @param[in]	last    Last guess.
/// @param[out]	result  If not null, stores the feedback on return.
/// @param[out] freq    If not null, stores the frequencies on return.
/// @param[in]  size    Size of freq.
typedef void (*ComparisonRoutine)(
	const Rules &rules,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Feedback *result,
	unsigned int *freq,
	size_t size);

/**
 * Counts the frequencies of each feedback in a feedback list.
 * @param first Begin of the feedback list.
 * @param last  End of the feedback list.
 * @param freq  Feedback frequency table.
 * @param size  Size of the frequency table.
 */
typedef void (*FrequencyRoutine)(
	const unsigned char *first,
	const unsigned char *last,
	unsigned int freq[],
	size_t size);

/// Computes the sum of squares of a frequency table.
/// @param[in]	freq	The frequency table to compute statistic on
/// @param[in]	max_fb	The maximum feedback value allowed
typedef unsigned int (*SumSquaresRoutine)(
	const unsigned int *first,
	const unsigned int *last);

/// Generates all codewords that conforms to the given set of rules.
typedef size_t (*GenerationRoutine)(
	const Rules &rules,
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

/**
 * Scans an array of codewords and returns a 16-bit mask of present 
 * colors.
 *
 * In the bit-mask returned, a bit is set if the corresponding color
 * is present in at least one of the codewords. A bit is cleared if 
 * the corresponding color never appears in any of the codewords. 
 * The bits are numbered from LSB to MSB. 
 *
 * For example, if the codeword is <code>4169</code>, then bits 1, 4, 
 * 6, and 9 are set, and the rest are unset. The mask returned will  
 * be <code>0000-0010-0101-0010</code>, or <code>0x0252</code>.
 *
 * Note that the highest bit (corresponding to color 0xF) is never set
 * in the returned mask, even if it exists in the codeword, because
 * <code>0xF</code> is reserved for special use.
 *
 * @param first Begin of the codeword list.
 * @param last  End of the codeword list.
 */
typedef unsigned short (*MaskRoutine)(
	const Codeword *first,
	const Codeword *last);

template <class Routine>
struct RoutineRegistry : public Utilities::Registry<std::string, Routine>
{
};

#define REGISTER_ROUTINE(type,id,item) REGISTER_ITEM(type,id,item)

} // namespace Mastermind

#endif // MASTERMIND_ALGORITHM_HPP
