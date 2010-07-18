/// \file Compare.h
/// Declaration of codeword comparison routines.

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

