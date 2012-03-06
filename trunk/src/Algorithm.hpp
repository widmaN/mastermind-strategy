#ifndef MASTERMIND_ALGORITHM_HPP
#define MASTERMIND_ALGORITHM_HPP

#include <string>
#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"
#include "Registry.hpp"

namespace Mastermind {

/**
 * Prototype of a function that compares a codeword to a list of codewords
 * and stores the feedbacks and/or frequencies of the comparision.
 *
 * @param secret  The secret.
 * @param guesses Array of guesses.
 * @param count   Number of guesses.
 * @param result  If not null, stores the feedback on return.
 * @param freq    If not null, stores the frequencies on return.
 * @remarks The frequencies are not initialized in this function.
 *      The caller must ensure the frequencies are set to zero before
 *      calling the function.
 */
typedef void ComparisonRoutine(
	const Codeword &secret,
	const Codeword *guesses,
	size_t count,
	Feedback *result,
	unsigned int *freq);

/// Pointer to the routine used for comparing generic codewords.
extern ComparisonRoutine* const compare_codewords_generic;

/// Pointer to the routine used for comparing codewords without repetition.
extern ComparisonRoutine* const compare_codewords_norepeat;


/// Generates all codewords that conforms to the given set of rules.
typedef size_t (*GenerationRoutine)(
	const Rules &rules,
	Codeword *results);



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
