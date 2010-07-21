/// \file Frequency.h
/// Declaration of feedback frequency counting routines.

#pragma once

#include "MMConfig.h"
#include "RoutineSelector.h"

///////////////////////////////////////////////////////////////////////////
// Frequency counting routines
//

/// Counts the frequencies that each feedback occurs in a feedback list.
/// @param[in]	feedbacks	A list of feedbacks to count frequencies on
/// @param[in]	count		Number of feedbacks in the list
/// @param[out]	freq		The frequency table
typedef void FREQUENCY_COUNTING_ROUTINE(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[MM_FEEDBACK_COUNT]);

/// Frequency counting implementation selector.
typedef Utilities::RoutineSelector<FREQUENCY_COUNTING_ROUTINE> FrequencyCountingRoutineSelector;

/// Routine table for feedback frequency counting.
extern FrequencyCountingRoutineSelector *CountFrequenciesImpl;

///////////////////////////////////////////////////////////////////////////
// Frequency statistic routines
//

/// Computes the sum of squares of the frequencies.
/// @param[in]	freq	The frequency table to compute statistic on
typedef unsigned int FREQUENCY_SUMSQUARES_ROUTINE(const unsigned int freq[MM_FEEDBACK_COUNT]);

/// Frequency sum-of-squares implementation selector.
typedef Utilities::RoutineSelector<FREQUENCY_SUMSQUARES_ROUTINE> FrequencySumSquaresRoutineSelector;

/// Routine table for feedback frequency counting.
extern FrequencySumSquaresRoutineSelector *GetSumOfSquaresImpl;
