/// \file Compare.cpp
/// Implementation of codeword comparison routines.
///

#include <assert.h>
#include <memory.h>
#include <intrin.h>
#include <emmintrin.h>

#include "MMConfig.h"
#include "Compare.h"
#include "CallCounter.h"

#ifndef NDEBUG
static bool _update_stat = true;
static Utilities::CallCounter _call_counter("CompareCodewords");
#endif

static inline void UpdateCallCounter(unsigned int comp)
{
#ifndef NDEBUG
	if (_update_stat) {
		_call_counter.AddCall(comp);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////
// Codeword comparison routines
//

// This is the benchmark routine for codeword comparison
// which ALLOWS REPETITION.
// The implementation is very simple and clean.
static void compare_long_codeword_r1(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	UpdateCallCounter(count);

	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);	
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i zero = _mm_setzero_si128();

	for (unsigned int i = 0; i < count; i++) {
		__m128i guess = *(guesses++);

		// count nA
		__m128i tA = _mm_cmpeq_epi8(secret, guess);
		tA = _mm_and_si128(tA, mask_high6);
		tA = _mm_sad_epu8(tA, zero);
		
		// count nB
		__m128i tB = _mm_min_epu8(secret, guess);
		tB = _mm_and_si128(tB, mask_low10);
		tB = _mm_sad_epu8(tB, zero);

		int nA = _mm_extract_epi16(tA, 4);
		int nB = _mm_extract_epi16(tB, 0) + _mm_extract_epi16(tB, 4);
		unsigned char nAnB = (unsigned char)((nA << (MM_FEEDBACK_BITS / 2)) | (nB - nA));
		*(results++) = nAnB;
	}
}

// This is the chosen implementation for "long" codeword comparison
// which ALLOWS REPETITION.
// It is a tiny improvement over v1.
static void compare_long_codeword_r2(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	UpdateCallCounter(count);

	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);	
	__m128i zero = _mm_setzero_si128();

	// Keep low 10-bytes of secret, while setting high 6 bytes to zero
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i secret_low10 = _mm_and_si128(mask_low10, secret);

	for (; count > 0; count--) {
		__m128i guess = *(guesses++);

		// count nA
		__m128i tA = _mm_cmpeq_epi8(secret, guess);
		tA = _mm_and_si128(tA, mask_high6);
		tA = _mm_sad_epu8(tA, zero);
		
		// count nB
		__m128i tB = _mm_min_epu8(secret_low10, guess);
		tB = _mm_sad_epu8(tB, zero);

		int nA = _mm_extract_epi16(tA, 4);
		int nB = _mm_extract_epi16(tB, 4) + _mm_cvtsi128_si32(tB);
		unsigned char nAnB = (unsigned char)((nA << MM_FEEDBACK_ASHIFT) | (nB - nA));
		*(results++) = nAnB;
		//unsigned char nAnB = (unsigned char)((nA << 4) | (nB - nA));
		//*(results++) = feedback_revmap[nAnB];
	}
}

// ALLOWS REPETITION.
// Small improvement over v2.
static void compare_long_codeword_r3(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	UpdateCallCounter(count);

	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);	
	__m128i zero = _mm_setzero_si128();

	// Keep low 10-bytes of secret, while setting high 6 bytes to zero
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i secret_low10 = _mm_and_si128(mask_low10, secret);

	unsigned int count0 = count;
	unsigned char *results0 = results;

	for (; count > 0; count -= 4) {
		if (1) {
		__m128i guess1 = *(guesses++);
		__m128i guess2 = *(guesses++);

		// count nA
		__m128i tA1 = _mm_cmpeq_epi8(secret, guess1);
		__m128i tA2 = _mm_cmpeq_epi8(secret, guess2);

		tA1 = _mm_and_si128(tA1, mask_high6);
		tA2 = _mm_and_si128(tA2, mask_high6);
			
		tA1 = _mm_sad_epu8(tA1, zero);
		tA2 = _mm_sad_epu8(tA2, zero);
			
		// count nB
		__m128i tB1 = _mm_min_epu8(secret_low10, guess1);
		__m128i tB2 = _mm_min_epu8(secret_low10, guess2);

		tB1 = _mm_sad_epu8(tB1, zero);
		tB2 = _mm_sad_epu8(tB2, zero);

		int nA1 = _mm_extract_epi16(tA1, 4);// + _mm_cvtsi128_si32(tA);
		int nA2 = _mm_extract_epi16(tA2, 4);// + _mm_cvtsi128_si32(tA);

		int nB1 = _mm_extract_epi16(tB1, 4) + _mm_cvtsi128_si32(tB1);
		int nB2 = _mm_extract_epi16(tB2, 4) + _mm_cvtsi128_si32(tB2);

		unsigned char nAnB1 = (unsigned char)((nA1 << MM_FEEDBACK_ASHIFT) | (nB1 - nA1));
		unsigned char nAnB2 = (unsigned char)((nA2 << MM_FEEDBACK_ASHIFT) | (nB2 - nA2));
		*(results++) = nAnB1;		
		*(results++) = nAnB2;
		}
		if (1) {
		__m128i guess1 = *(guesses++);
		__m128i guess2 = *(guesses++);

		// count nA
		__m128i tA1 = _mm_cmpeq_epi8(secret, guess1);
		__m128i tA2 = _mm_cmpeq_epi8(secret, guess2);

		tA1 = _mm_and_si128(tA1, mask_high6);
		tA2 = _mm_and_si128(tA2, mask_high6);
			
		tA1 = _mm_sad_epu8(tA1, zero);
		tA2 = _mm_sad_epu8(tA2, zero);
			
		// count nB
		__m128i tB1 = _mm_min_epu8(secret_low10, guess1);
		__m128i tB2 = _mm_min_epu8(secret_low10, guess2);

		tB1 = _mm_sad_epu8(tB1, zero);
		tB2 = _mm_sad_epu8(tB2, zero);

		int nA1 = _mm_extract_epi16(tA1, 4);// + _mm_cvtsi128_si32(tA);
		int nA2 = _mm_extract_epi16(tA2, 4);// + _mm_cvtsi128_si32(tA);

		int nB1 = _mm_extract_epi16(tB1, 4) + _mm_cvtsi128_si32(tB1);
		int nB2 = _mm_extract_epi16(tB2, 4) + _mm_cvtsi128_si32(tB2);

		// MM_FEEDBACK_ASHIFT
		unsigned char nAnB1 = (unsigned char)((nA1 << MM_FEEDBACK_ASHIFT) | (nB1 - nA1));
		unsigned char nAnB2 = (unsigned char)((nA2 << MM_FEEDBACK_ASHIFT) | (nB2 - nA2));
		*(results++) = nAnB1;
		*(results++) = nAnB2;
		}
	}
	return;
	count = count0;
	results = results0;
	for (; count >= 4; count -= 4) {
		results[0] = feedback_revmap[results[0]];
		results[1] = feedback_revmap[results[1]];
		results[2] = feedback_revmap[results[2]];
		results[3] = feedback_revmap[results[3]];
		results += 4;
	}
}

static void compare_long_codeword_r4(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	UpdateCallCounter(count);

	unsigned int count0 = count;
	unsigned char *results0 = results;

	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));
	__m128i zero = _mm_setzero_si128();

	//__m128i mask_high6 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	//__m128i secret_high6 = _mm_or_si128(mask_high6, secret);
	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);

	// Keep low 10-bytes of secret, while setting high 6 bytes to zero
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i secret_low10 = _mm_and_si128(mask_low10, secret);

#define ONE_OP() do { \
		__m128i guess = *(guesses++); \
		__m128i tA = _mm_cmpeq_epi8(secret, guess); \
		tA = _mm_and_si128(tA, mask_high6); \
		tA = _mm_sad_epu8(tA, zero); \
		__m128i tB = _mm_min_epu8(secret_low10, guess); \
		tB = _mm_sad_epu8(tB, zero); \
		unsigned char nA = _mm_extract_epi16(tA, 4); \
		unsigned char nAB = _mm_extract_epi16(tB, 4) + _mm_cvtsi128_si32(tB); \
		unsigned char nAnB = (unsigned char)((nA << MM_FEEDBACK_ASHIFT) | (nAB - nA)); \
		*(results++) = nAnB; \
	} while (0)

	for (; count >= 4; count -= 4) {
		ONE_OP();
		ONE_OP();
		ONE_OP();
		ONE_OP();
	}
	for (; count > 0; count--) {
		ONE_OP();
	}
	return;

	count = count0;
	results = results0;
	for (; count >= 4; count -= 4) {
		*results = feedback_revmap[*results]; results++;
		*results = feedback_revmap[*results]; results++;
		*results = feedback_revmap[*results]; results++;
		*results = feedback_revmap[*results]; results++;
	}
}

static int count_bits(unsigned short a)
{
	int n = 0;
	for (; a; a >>= 1) {
		if (a & 1)
			n++;
	}
	return n;
}

// This is the benchmark routine for "long" codeword comparasion 
// that assumes NO REPETITION!!!
// It illustrate the algorithm used.
// It uses a memory table to lookup.
// Would have been much faster if we had the POPCNT instruction
// (which is part of SSE 4.2)
static void compare_long_codeword_nr1(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	UpdateCallCounter(count);

	// Change 0xFF in secret to 0x0F
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	// Set 0 counter in secret to 0xFF
	__m128i t1 = _mm_cmpeq_epi8(secret, _mm_setzero_si128());
	t1 = _mm_slli_si128(t1, 6);
	t1 = _mm_srli_si128(t1, 6);
	secret = _mm_or_si128(secret, t1);	
	
	// Define a static mapping from bitmask to nAnB
	static unsigned char counter[0x10000];
	static bool counter_ready = false;
	if (!counter_ready) {
		for (int i = 0; i < 0x10000; i++) {
			int nA = count_bits(i >> MM_MAX_COLORS);
			int nB = count_bits(i & ((1<<MM_MAX_COLORS)-1)) - nA;
			counter[i] = (nA << MM_FEEDBACK_ASHIFT) | nB;
		}
		counter_ready = true;
	}

	// Scan codeword array
	for (; count > 0; count--) {
		__m128i guess = *(guesses++);
		__m128i cmp = _mm_cmpeq_epi8(guess, secret);
		int mask = _mm_movemask_epi8(cmp);
		unsigned char nAnB = counter[mask];
		*(results++) = nAnB;
	}
}

// This is the chosen implementation for "long" codeword comparison
// that assumes NO REPETITION!!!
// It is a 4-parallel version of v3, and doubles performance because
// it can do something at the expensive memory access time.
static void compare_long_codeword_nr2(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	UpdateCallCounter(count);

	// Change 0xFF in secret to 0x0F
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	// Set 0 counter in secret to 0xFF
	__m128i t1 = _mm_cmpeq_epi8(secret, _mm_setzero_si128());
	t1 = _mm_slli_si128(t1, MM_MAX_PEGS);
	t1 = _mm_srli_si128(t1, MM_MAX_PEGS);
	secret = _mm_or_si128(secret, t1);	
	
	// Define a static mapping from bitmask to nAnB
	static unsigned char counter[0x10000];
	static bool counter_ready = false;
	if (!counter_ready) {
		for (int i = 0; i < 0x10000; i++) {
			int nA = count_bits(i >> MM_MAX_COLORS);
			int nB = count_bits(i & ((1<<MM_MAX_COLORS)-1)) - nA;
			counter[i] = (nA << MM_FEEDBACK_ASHIFT) | nB;
		}
		counter_ready = true;
	}

	for (; count >= 4; count -= 4) {
		__m128i guess1 = *(guesses++);
		__m128i guess2 = *(guesses++);
		__m128i guess3 = *(guesses++);
		__m128i guess4 = *(guesses++);

		__m128i cmp1 = _mm_cmpeq_epi8(guess1, secret);
		__m128i cmp2 = _mm_cmpeq_epi8(guess2, secret);
		__m128i cmp3 = _mm_cmpeq_epi8(guess3, secret);
		__m128i cmp4 = _mm_cmpeq_epi8(guess4, secret);

		int mask1 = _mm_movemask_epi8(cmp1);
		int mask2 = _mm_movemask_epi8(cmp2);
		int mask3 = _mm_movemask_epi8(cmp3);
		int mask4 = _mm_movemask_epi8(cmp4);

		unsigned char nAnB1 = counter[mask1];
		unsigned char nAnB2 = counter[mask2];
		unsigned char nAnB3 = counter[mask3];
		unsigned char nAnB4 = counter[mask4];

		unsigned int t = (unsigned int)nAnB1
			| ((unsigned int)nAnB2 << 8)
			| ((unsigned int)nAnB3 << 16)
			| ((unsigned int)nAnB4 << 24);

		*(unsigned int *)results = t;
		results += 4;
	}

	// Scan the remaining codewords
	for (; count > 0; count--) {
		__m128i guess = *(guesses++);
		__m128i cmp = _mm_cmpeq_epi8(guess, secret);
		int mask = _mm_movemask_epi8(cmp);
		unsigned char nAnB = counter[mask];
		*(results++) = nAnB;
	}

}

///////////////////////////////////////////////////////////////////////////
// Interface routines
//

void PrintCompareStatistics()
{
#ifndef NDEBUG
	_call_counter.DebugPrint();
#endif
}

static ComparisonRoutineSelector::RoutineEntry CompareRep_Entries[] = {
	{ "r_p1", "Allow repetition - simple implementation", compare_long_codeword_r1 },
	{ "r_p1a", "Allow repetition - improved implementation", compare_long_codeword_r2 },
	{ "r_p1b", "Allow repetition - four parallel", compare_long_codeword_r3 },
	{ "r_p1c", "Allow repetition - feedback_revmap", compare_long_codeword_r4 },
	{ NULL, NULL, NULL },
};

static ComparisonRoutineSelector::RoutineEntry CompareNoRep_Entries[] = {
	{ "nr_p1", "No repetition - simple implementation", compare_long_codeword_nr1 },
	{ "nr_p4", "No repetition - four parallel", compare_long_codeword_nr2 },
	{ NULL, NULL, NULL },
};

ComparisonRoutineSelector *CompareRepImpl = 
	new ComparisonRoutineSelector(CompareRep_Entries, "r_p1a");

ComparisonRoutineSelector *CompareNoRepImpl = 
	new ComparisonRoutineSelector(CompareNoRep_Entries, "nr_p4");

/*
void Compare_Rep(
	const codeword_t& secret,
	const codeword_t guesses[],
	unsigned int count,
	unsigned char results[])
{
	return compare_long_codeword_v1(secret.value, (__m128i*)guesses, count, results);
}

void Compare_NoRep(
	const codeword_t& secret,
	const codeword_t guesses[],
	unsigned int count,
	unsigned char results[])
{
	return compare_long_codeword_v4(secret.value, (__m128i*)guesses, count, results);
}
*/