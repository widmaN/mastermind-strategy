#include <cassert>
#include <memory.h>
//#include <emmintrin.h>
//#include <stdio.h>

#include "MMConfig.h"
#include "Frequency.h"
#include "Algorithm.hpp"

#if ENABLE_CALL_COUNTER
#include "CallCounter.h"

///////////////////////////////////////////////////////////////////////////
// Calling counter statistics

// The count_freq() calling statistics for 4 pegs, 10 colors, no rep
//List length >=  4096 : 9
//List length >=  1024 : 1682
//List length >=   512 : 841
//List length >=   256 : 22893
//List length >=   128 : 24938
//List length >=    64 : 135980
//List length >=    32 : 222630
//List length >=    16 : 652336
//List length >=     8 : 1111720
//List length >=     4 : 1678683
//List length >=     2 : 825598
//List length >=     1 : 17404
//count_freq() called times: 4694714
//Number of codes compared:  73869707
//Average comparison per call: 15.73

static Utilities::CallCounter _call_counter("CountFrequencies", true);

static inline void UpdateCallCounter(unsigned int comp)
{
	_call_counter.AddCall(comp);
}

void PrintFrequencyStatistics()
{
	_call_counter.DebugPrint();
}
#endif

// todo: These routines are quite simple, so it is unnecessary to create
// a dedicate source file for it.

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// Frequency counting routines
//

// Straightforward (and natural) implementation in C. In practice, 
// if the feedback list is small, this routine actually performs better 
// than other more complicated implementations that include manual loop
// unrolling and out-of-order execution. Therefore, we select this 
// routine as our choice.
static void count_freq_c(
	const unsigned char *first,
	const unsigned char *last,
	unsigned int freq[],
	unsigned char max_fb)
{
#if ENABLE_CALL_COUNTER
	UpdateCallCounter(count);
#endif

	memset(freq, 0, sizeof(unsigned int)*((int)max_fb+1));
	for (const unsigned char *feedback = first; feedback != last; ++feedback)
	{
		assert(*feedback <= max_fb);
		++freq[*feedback];
	}
}

///////////////////////////////////////////////////////////////////////////
// Sum-of-squares computation routines.
//

static unsigned int sum_squares_v1(
	const unsigned int *first,
	const unsigned int *last)
{
	unsigned int ret = 0;
	for (const unsigned int *p = first; p != last; ++p)
	{
		ret += (*p) * (*p);
	}
	return ret;
}

#if 0
static unsigned int sum_squares_v2(
	const unsigned int freq[],
	unsigned char max_fb)
{
	unsigned int ret = 0;
	unsigned int count = (unsigned int)max_fb + 1;
	for (; count >= 4; count -= 4) {
		ret += (freq[0]*freq[0] + freq[1]*freq[1])
			+ (freq[2]*freq[2] + freq[3]*freq[3]);
		freq += 4;
	}
	for (; count > 0; count--) {
		ret += freq[0]*freq[0];
		freq++;
	}
	return ret;
}
#endif

#if 0
// Require SSE4
static unsigned int ComputeSumOfSquares_v3(const unsigned int _freq[MM_FEEDBACK_COUNT])
{
	const unsigned int *freq = _freq;
	__m128i ss1 = _mm_setzero_si128();
	__m128i ss2 = _mm_setzero_si128();
	for (int n = MM_FEEDBACK_COUNT; n > 0; n -= 8) {
		__m128i fb1 = _mm_loadu_si128((const __m128i *)freq);
		__m128i fb2 = _mm_loadu_si128((const __m128i *)(freq+4));
		fb1 = _mm_mullo_epi32(fb1, fb1);
		fb2 = _mm_mullo_epi32(fb2, fb2);
		ss1 = _mm_add_epi32(ss1, fb1);
		ss2 = _mm_add_epi32(ss2, fb2);
		freq += 8;
	}
	__m128i ss = _mm_add_epi32(ss1, ss2);
	ss = _mm_hadd_epi32(ss, ss);
	ss = _mm_hadd_epi32(ss, ss);
	return _mm_cvtsi128_si32(ss);
}
#endif

#if 0
// This routine uses four paralle. However, the performance is actually slower
// than the two-parallel implementation. So, we don't use it.
// Require SSE4
unsigned int ComputeSumOfSquares_v4(const unsigned int _freq[MM_FEEDBACK_COUNT])
{
	const unsigned int *freq = _freq;
	unsigned int ret = 0;
	__m128i ss1 = _mm_setzero_si128();
	__m128i ss2 = _mm_setzero_si128();
	__m128i ss3 = _mm_setzero_si128();
	__m128i ss4 = _mm_setzero_si128();
	for (int n = MM_FEEDBACK_COUNT; n > 0; n -= 16) {
		__m128i fb1 = _mm_loadu_si128((const __m128i *)freq);
		__m128i fb2 = _mm_loadu_si128((const __m128i *)(freq+4));
		__m128i fb3 = _mm_loadu_si128((const __m128i *)(freq+8));
		__m128i fb4 = _mm_loadu_si128((const __m128i *)(freq+12));

		fb1 = _mm_mullo_epi32(fb1, fb1);
		fb2 = _mm_mullo_epi32(fb2, fb2);
		fb3 = _mm_mullo_epi32(fb3, fb3);
		fb4 = _mm_mullo_epi32(fb4, fb4);

		ss1 = _mm_add_epi32(ss1, fb1);
		ss2 = _mm_add_epi32(ss2, fb2);
		ss3 = _mm_add_epi32(ss3, fb3);
		ss4 = _mm_add_epi32(ss4, fb4);

		freq += 16;
	}
	ss1 = _mm_add_epi32(ss1, ss2);
	ss3 = _mm_add_epi32(ss3, ss4);
	__m128i ss = _mm_add_epi32(ss1, ss3);
	ss = _mm_hadd_epi32(ss, ss);
	ss = _mm_hadd_epi32(ss, ss);
	return _mm_cvtsi128_si32(ss);
	// return ret;
}
#endif

///////////////////////////////////////////////////////////////////////////
// Routine registration.

REGISTER_ROUTINE(FrequencyRoutine, "generic", count_freq_c)

REGISTER_ROUTINE(SumSquaresRoutine, "generic", sum_squares_v1)
//REGISTER_ROUTINE(SumSquaresRoutine, "generic_p4", sum_squares_v2)


#if 0
static FrequencySumSquaresRoutineSelector::RoutineEntry GetSumOfSquares_Entries[] = {
	{ "c", "Simple implementation", ComputeSumOfSquares_v1 },
	{ "c_p4", "Simple implementation with 2-parallel", ComputeSumOfSquares_v2 },
	//{ "sse4", "SIMD implementation (requires SSE4 instruction set)", ComputeSumOfSquares_v3 },
	{ NULL, NULL, NULL },
};

FrequencySumSquaresRoutineSelector *GetSumOfSquaresImpl =
	new FrequencySumSquaresRoutineSelector(GetSumOfSquares_Entries,
	//(Utilities::CpuInfo::Features.WithSSE41)? "sse4" : "c");
	"c_p4");
#endif
