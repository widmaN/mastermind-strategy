#pragma once

#include "MMConfig.h"
#include "RoutineSelector.h"

typedef void FREQUENCY_COUNTING_ROUTINE(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[MM_FEEDBACK_COUNT]);

typedef Utilities::RoutineSelector<FREQUENCY_COUNTING_ROUTINE> FrequencyCountingRoutineSelector;

extern FrequencyCountingRoutineSelector *CountFrequenciesImpl;

/*
void count_freq_c(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

void count_freq_c_luf4(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

void count_freq_v9(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

void count_freq_v10(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

	*/