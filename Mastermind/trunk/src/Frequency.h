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

void count_freq_v2(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[256]);

void count_freq_v3(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[256]);

void count_freq_v4(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[256]);

void count_freq_v5(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[128]);

void count_freq_v6(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[MM_FEEDBACK_COUNT]);

void count_freq_v7(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int _freq[128]);

void count_freq_v8(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int _freq[128]);

void count_freq_v9(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

void count_freq_v10(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

void count_freq_v11(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64]);

#endif
