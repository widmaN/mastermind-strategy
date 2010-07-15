#pragma once

#include "MMConfig.h"

void CountFrequencies_Impl(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[MM_FEEDBACK_COUNT]);

#ifndef NTEST

void count_freq_v1(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[256]);

void count_freq_v3(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[256]);

void count_freq_v9(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

void count_freq_v10(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

#endif
