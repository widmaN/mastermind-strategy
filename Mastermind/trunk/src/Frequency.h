#pragma once

#include "MMConfig.h"

typedef void (*FREQUENCY_COUNTING_ROUTINE)(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[MM_FEEDBACK_COUNT]);

extern FREQUENCY_COUNTING_ROUTINE CountFrequencies_Impl;

typedef struct FrequencyCountingRoutineEntry 
{
	const char *name;
	const char *description;
	FREQUENCY_COUNTING_ROUTINE routine;
} FrequencyCountingRoutineEntry;

extern FrequencyCountingRoutineEntry CountFrequencies_Impls[];

void CountFrequencies_SelectImpl(const char *name);

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

