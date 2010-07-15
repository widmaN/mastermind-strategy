/// \file Compare.cpp
/// Implementation of codeword comparison routines.
///

#include <assert.h>
#include <memory.h>
#include <intrin.h>
#include <emmintrin.h>

#include "MMConfig.h"
#include "Compare.h"

///////////////////////////////////////////////////////////////////////////
// Codeword comparison routines
//

// This is the benchmark routine for codeword comparison
// which ALLOWS REPETITION.
// The implementation is very simple and clean.
MM_IMPLEMENTATION
void compare_long_codeword_v1(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
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
MM_IMPLEMENTATION
void compare_long_codeword_v2(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);	
	__m128i zero = _mm_setzero_si128();

	// Keep low 10-bytes of secret, while setting high 6 bytes to zero
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i secret_low10 = _mm_and_si128(mask_low10, secret);

	for (unsigned int i = 0; i < count; i++) {
		__m128i guess = *(guesses++);

		// count nA
		__m128i tA = _mm_cmpeq_epi8(secret, guess);
		tA = _mm_and_si128(tA, mask_high6);
		tA = _mm_sad_epu8(tA, zero);
		
		// count nB
		__m128i tB = _mm_min_epu8(secret_low10, guess);
		tB = _mm_sad_epu8(tB, zero);

		int nAnB = _mm_extract_epi16(tA, 4);
		nAnB <<= 4;
		nAnB += _mm_extract_epi16(tB, 4); // BUG: nA not deducted from nB
		nAnB += _mm_cvtsi128_si32(tB); // _mm_extract_epi16(tB, 0);
		*(results++) = nAnB;
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
MM_IMPLEMENTATION
void compare_long_codeword_v3(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
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
MM_IMPLEMENTATION
void compare_long_codeword_v4(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
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
