/// \file Compare.h
/// Declaration of codeword comparison routines.

#pragma once

#include <emmintrin.h>
#include "MMConfig.h"
#include "RoutineSelector.h"

/// Compares an array of guesses to the secret.
typedef void COMPARISON_ROUTINE(
	/// [in] The secret codeword
	__m128i secret, 
	/// [in] An array of guesses
	const __m128i guesses[], 
	/// [in] Number of guesses in the array
	unsigned int count, 
	/// [out] An array to store feedbacks
	unsigned char results[]);

typedef Utilities::RoutineSelector<COMPARISON_ROUTINE> ComparisonRoutineSelector;

extern ComparisonRoutineSelector *CompareRepImpl;
extern ComparisonRoutineSelector *CompareNoRepImpl;

