/// \file Compare.cpp
/// Implementation of codeword comparison routines.
///

#include <assert.h>
#include <memory.h>
// #include <intrin.h>
#include <emmintrin.h>
#include <omp.h>

#include "MMConfig.h"
#include "Compare.h"
#include "CallCounter.h"
#include "Registry.hpp"
#include "Algorithm.hpp"
#include "Codeword.hpp"

#if ENABLE_CALL_COUNTER
static Utilities::CallCounter _call_counter("CompareCodewords", true);
#endif

static inline void UpdateCallCounter(unsigned int comp)
{
#if ENABLE_CALL_COUNTER
	_call_counter.AddCall(comp);
#endif
}

void PrintCompareStatistics()
{
#if ENABLE_CALL_COUNTER
	_call_counter.DebugPrint();
#endif
}

///////////////////////////////////////////////////////////////////////////
// Codeword comparison routines
//

using namespace Mastermind;

// This is the benchmark routine for codeword comparison
// which ALLOWS REPETITION.
// The implementation is very simple and clean.
static void compare_codeword_rep_p1(
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
#if MM_FEEDBACK_COMPACT
		unsigned char nAnB = (nB*nB+nB)/2+nA;
#else
		unsigned char nAnB = (unsigned char)((nA << (MM_FEEDBACK_BITS / 2)) | (nB - nA));
#endif
		*(results++) = nAnB;
	}
}

// This is the chosen implementation for "long" codeword comparison
// which ALLOWS REPETITION.
// It is a tiny improvement over v1.
static void compare_codeword_rep_p1a(
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
#if MM_FEEDBACK_COMPACT
		unsigned char nAnB = (nB*nB+nB)/2+nA;
#else
		unsigned char nAnB = (unsigned char)((nA << MM_FEEDBACK_ASHIFT) | (nB - nA));
#endif
		*(results++) = nAnB;
		//unsigned char nAnB = (unsigned char)((nA << 4) | (nB - nA));
		//*(results++) = feedback_revmap[nAnB];
	}
}

// OpenMP version 1
static void compare_codeword_rep_p1a_omp1(
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

	#pragma omp parallel for
	for (int i = 0; i < (int)count; i++) {
		__m128i guess = guesses[i];

		// count nA
		__m128i tA = _mm_cmpeq_epi8(secret, guess);
		tA = _mm_and_si128(tA, mask_high6);
		tA = _mm_sad_epu8(tA, zero);

		// count nB
		__m128i tB = _mm_min_epu8(secret_low10, guess);
		tB = _mm_sad_epu8(tB, zero);

		int nA = _mm_extract_epi16(tA, 4);
		int nB = _mm_extract_epi16(tB, 4) + _mm_cvtsi128_si32(tB);
#if MM_FEEDBACK_COMPACT
		unsigned char nAnB = (nB*nB+nB)/2+nA;
#else
		unsigned char nAnB = (unsigned char)((nA << MM_FEEDBACK_ASHIFT) | (nB - nA));
#endif
		results[i] = nAnB;
	}
}

/*
static void compare_codeword_block_rep(
	const __m128i *secrets,
	const __m128i *guesses,
	int isecret,
	int iguess,
	int count,
	unsigned char *results)
{
	assert(count >= 0);
	assert(count % 8 == 0);


	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);
	__m128i zero = _mm_setzero_si128();

	// Keep low 10-bytes of secret, while setting high 6 bytes to zero
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i secret_low10 = _mm_and_si128(mask_low10, secret);

	// #pragma omp parallel for
	for (int i = 0; i < (int)count; i++) {
		__m128i guess = guesses[i];

		// count nA
		__m128i tA = _mm_cmpeq_epi8(secret, guess);
		tA = _mm_and_si128(tA, mask_high6);
		tA = _mm_sad_epu8(tA, zero);

		// count nB
		__m128i tB = _mm_min_epu8(secret_low10, guess);
		tB = _mm_sad_epu8(tB, zero);

		int nA = _mm_extract_epi16(tA, 4);
		int nB = _mm_extract_epi16(tB, 4) + _mm_cvtsi128_si32(tB);
#if MM_FEEDBACK_COMPACT
		unsigned char nAnB = (nB*nB+nB)/2+nA;
#else
		unsigned char nAnB = (unsigned char)((nA << MM_FEEDBACK_ASHIFT) | (nB - nA));
#endif
		results[i] = nAnB;
	}
}
*/

// OpenMP version 2
static void cross_compare_codewords_rep_omp1(
	const __m128i *secrets,
	unsigned int nsecrets,
	const __m128i *guesses,
	unsigned int nguesses,
	unsigned char *results)
{
	// UpdateCallCounter(count);

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i zero = _mm_setzero_si128();

	for (int j = 0; j < (int)nguesses; j++) {
		__m128i guess = guesses[j];

		// Change 0xff in guess to 0x0f
		guess = _mm_and_si128(guess, _mm_set1_epi8(0x0f));

		// Keep low 10-bytes of guess, while setting high 6 bytes to zero
		__m128i guess_low10 = _mm_and_si128(mask_low10, guess);

		// #pragma omp parallel for
		for (int i = 0; i < (int)nsecrets; i++) {
			__m128i secret = secrets[i];

			// count nA
			__m128i tA = _mm_cmpeq_epi8(secret, guess);
			tA = _mm_and_si128(tA, mask_high6);
			tA = _mm_sad_epu8(tA, zero);

			// count nB
			__m128i tB = _mm_min_epu8(guess_low10, secret);
			tB = _mm_sad_epu8(tB, zero);

			int nA = _mm_extract_epi16(tA, 4);
			int nB = _mm_extract_epi16(tB, 4) + _mm_cvtsi128_si32(tB);
#if MM_FEEDBACK_COMPACT
			unsigned char nAnB = (nB*nB+nB)/2+nA;
#else
			unsigned char nAnB = (unsigned char)((nA << MM_FEEDBACK_ASHIFT) | (nB - nA));
#endif
			results[j*nguesses+i] = nAnB;
		}
	}
}

static void compare_codeword_rep_p8(
	__m128i secret,
	const __m128i *guesses,
	unsigned int count,
	unsigned char *results)
{
	// TODO: This function works not well for small list.
	// Should find a way to integrate this with compare_codeword_rep_p1a to
	// speed up the actual speed.
	// We could also try out storing codewords in 8-bytes, so that we can
	// save a bunch of SAD instructions.
	UpdateCallCounter(count);

	if (count == 0)
		return;

	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);
	__m128i zero = _mm_setzero_si128();

	// Keep low 10-bytes of secret, while setting high 6 bytes to zero
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i secret_low10 = _mm_and_si128(mask_low10, secret);

	for (; count >= 8; count -= 8) {
		__m128i bigA = _mm_setzero_si128();
		__m128i bigB = _mm_setzero_si128();

#define COMPUTE_AND_SHIFT(i) do { \
		__m128i guess = guesses[(i)]; \
		__m128i tA = _mm_sad_epu8(_mm_and_si128(_mm_cmpeq_epi8(secret, guess), mask_high6), zero); \
		__m128i tB = _mm_sad_epu8(_mm_min_epu8(secret_low10, guess), zero); \
		bigA = _mm_or_si128(_mm_slli_si128(bigA, 1), tA); \
		bigB = _mm_or_si128(_mm_slli_si128(bigB, 1), tB); \
	} while (0)

		COMPUTE_AND_SHIFT(7); // 1
		COMPUTE_AND_SHIFT(6);
		COMPUTE_AND_SHIFT(5);
		COMPUTE_AND_SHIFT(4);
		COMPUTE_AND_SHIFT(3); // 5
		COMPUTE_AND_SHIFT(2);
		COMPUTE_AND_SHIFT(1);
		COMPUTE_AND_SHIFT(0);
		guesses += 8;

#undef COMPUTE_AND_SHIFT

		bigA = _mm_add_epi8(bigA, _mm_srli_si128(bigA, 8));
		bigB = _mm_add_epi8(bigB, _mm_srli_si128(bigB, 8));

#if MM_FEEDBACK_COMPACT
		bigA = _mm_unpacklo_epi8(bigA, zero);
		bigB = _mm_unpacklo_epi8(bigB, zero);
		__m128i bigAB = _mm_add_epi16(_mm_avg_epu16(_mm_mullo_epi16(bigB, bigB), bigB), bigA);
		bigAB = _mm_packs_epi16(bigAB, bigAB);
#else
		bigB = _mm_sub_epi8(bigB, bigA);
		__m128i bigAB = _mm_or_si128(_mm_slli_epi16(bigA, MM_FEEDBACK_ASHIFT), bigB);
#endif

		_mm_storel_epi64((__m128i*)results, bigAB);
		results += 8;
	}

	// Process the unaligned codewords
	for (; count > 0; count--) {
		__m128i guess = *(guesses++);
		__m128i tA = _mm_sad_epu8(_mm_and_si128(_mm_cmpeq_epi8(secret, guess), mask_high6), zero);
		__m128i tB = _mm_sad_epu8(_mm_min_epu8(secret_low10, guess), zero);
		int nA = _mm_extract_epi16(tA, 4) + _mm_cvtsi128_si32(tA);
		int nB = _mm_extract_epi16(tB, 4) + _mm_cvtsi128_si32(tB);
#if MM_FEEDBACK_COMPACT
		unsigned char nAnB = (nB*nB+nB)/2+nA;
#else
		unsigned char nAnB = (unsigned char)((nA << MM_FEEDBACK_ASHIFT) | (nB - nA));
#endif
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
			int nAB = count_bits(i & ((1<<MM_MAX_COLORS)-1));
#if MM_FEEDBACK_COMPACT
			counter[i] = (nAB*nAB+nAB)/2+nA;
#else
			counter[i] = (nA << MM_FEEDBACK_ASHIFT) | (nAB - nA);
#endif
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
			int nAB = count_bits(i & ((1<<MM_MAX_COLORS)-1));
#if MM_FEEDBACK_COMPACT
			counter[i] = (nAB*nAB+nAB)/2+nA;
#else
			counter[i] = (nA << MM_FEEDBACK_ASHIFT) | (nAB - nA);
#endif
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

static ComparisonRoutineSelector::RoutineEntry CompareRep_Entries[] = {
	{ "r_p1", "Allow repetition - simple implementation", compare_codeword_rep_p1 },
	{ "r_p1a", "Allow repetition - improved implementation", compare_codeword_rep_p1a },
	{ "r_p1a_omp1", "Allow repetition - OpenMP implementation 1", compare_codeword_rep_p1a_omp1 },
	//{ "r_p1a_omp2", "Allow repetition - OpenMP implementation 2", compare_codeword_rep_p1a_omp2 },
	{ "r_p8", "Allow repetition - 8-parallel", compare_codeword_rep_p8 },
	{ NULL, NULL, NULL },
};

static ComparisonRoutineSelector::RoutineEntry CompareNoRep_Entries[] = {
	{ "nr_p1", "No repetition - simple implementation", compare_long_codeword_nr1 },
	{ "nr_p4", "No repetition - four parallel", compare_long_codeword_nr2 },
	{ NULL, NULL, NULL },
};

ComparisonRoutineSelector *CompareRepImpl =
	new ComparisonRoutineSelector(CompareRep_Entries,
	"r_p8");
	//"r_p1a_omp");

ComparisonRoutineSelector *CompareNoRepImpl =
	new ComparisonRoutineSelector(CompareNoRep_Entries, "nr_p4");


static CrossComparisonRoutineSelector::RoutineEntry CrossCompareRep_Entries[] = {
	{ "cr_omp1", "Allow repetition - OMP 1", cross_compare_codewords_rep_omp1 },
	{ NULL, NULL, NULL },
};

CrossComparisonRoutineSelector *CrossCompareRepImpl =
	new CrossComparisonRoutineSelector(CrossCompareRep_Entries, "cr_omp1");

//////////////////////////////////////////////////////////////////////
// New interface

// SSE2-enabled generic routine for comparing codewords.
// Repeated colors are allowed.
static void compare_long_codeword_default(
	const CodewordRules &rules,
	const Codeword& _secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[])
{
	// UpdateCallCounter(count);
	__m128i secret = _secret.value();

	// Change 0xff in secret to 0x0f
	secret = _mm_and_si128(secret, _mm_set1_epi8(0x0f));

	__m128i mask_high6 = _mm_slli_si128(_mm_set1_epi8((char)0x01), 10);
	__m128i mask_low10 = _mm_srli_si128(_mm_set1_epi8((char)0xff), 6);
	__m128i zero = _mm_setzero_si128();

	for (const __m128i *guesses = (const __m128i *)first;
		guesses != (const __m128i *)last;
		guesses++)
	{
		__m128i guess = *guesses;

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
#if MM_FEEDBACK_COMPACT
		unsigned char nAnB = (nB*nB+nB)/2+nA;
#else
		unsigned char nAnB = (unsigned char)((nA << (MM_FEEDBACK_BITS / 2)) | (nB - nA));
#endif
		*(result++) = nAnB;
	}
}

REGISTER_ITEM(ComparisonRoutine, "default", compare_long_codeword_default)
