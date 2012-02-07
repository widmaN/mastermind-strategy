/// \file Compare.h
/// Declaration of codeword comparison routines.

#if 0

#pragma once

#include <emmintrin.h>
#include "MMConfig.h"
#include "RoutineSelector.h"

/// Compares an array of guesses to the secret.
/// @param[in]	secret	The secret codeword
/// @param[in]	guesses	An array of guesses
/// @param[in]	count	Number of guesses in the array
/// @param[out]	results	An array to store feedbacks
typedef void COMPARISON_ROUTINE(
	__m128i secret, 
	const __m128i guesses[], 
	unsigned int count, 
	unsigned char results[]);

/// Codeword comparison implementation selector.
typedef Utilities::RoutineSelector<COMPARISON_ROUTINE> ComparisonRoutineSelector;

/// Routine table for codeword comparison (allowing repetition).
extern ComparisonRoutineSelector *CompareRepImpl;

/// Routine table for codeword comparison (without repetition).
extern ComparisonRoutineSelector *CompareNoRepImpl;

/// Compares every pair of codewords from an array of secrets and an array
/// of guesses.
/// @param[in]	secret		Pointer to an array of secrets
/// @param[in]	nsecrets	Number of elements in the secret array
/// @param[in]	guesses		Pointer to an array of guesses
/// @param[in]	nguesses	Number of elements in the guess array
/// @param[out]	results		Pointer to an array to store feedbacks.
///							The results are stored in guess-major order.
typedef void CROSS_COMPARISON_ROUTINE(
	const __m128i *secrets,
	unsigned int nsecrets,
	const __m128i *guesses,
	unsigned int nguesses,
	unsigned char *results);

/// Codeword cross-comparison implementation selector.
typedef Utilities::RoutineSelector<CROSS_COMPARISON_ROUTINE> CrossComparisonRoutineSelector;

/// Routine table for codeword cross comparison (allowing repetition).
extern CrossComparisonRoutineSelector *CrossCompareRepImpl;

#endif
