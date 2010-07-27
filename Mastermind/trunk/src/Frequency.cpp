#include <assert.h>
#include <memory.h>
#include <emmintrin.h>
#include <stdio.h>
#include <intrin.h>
#include <smmintrin.h>

#include "CpuInfo.h"
#include "MMConfig.h"
#include "Frequency.h"
#include "CallCounter.h"

///////////////////////////////////////////////////////////////////////////
// Calling counter statistics
//

#if ENABLE_CALL_COUNTER
static Utilities::CallCounter _call_counter("CountFrequencies", true);
#endif

static inline void UpdateCallCounter(unsigned int comp)
{
#if ENABLE_CALL_COUNTER
	_call_counter.AddCall(comp);
#endif
}

void PrintFrequencyStatistics()
{
#if ENABLE_CALL_COUNTER
	_call_counter.DebugPrint();
#endif
}

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

///////////////////////////////////////////////////////////////////////////
// Frequency counting routines
//

// Simplistic implementation in C. In practice, if the feedback list is small,
// this routine actually performs better than more complicated Out-of-order Execution
// routines. So we use this as the choice routine.
static void count_freq_c(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[],
	unsigned char max_fb)
{
	UpdateCallCounter(count);

	memset(freq, 0, sizeof(unsigned int)*((int)max_fb+1));
	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++);
		assert(fb <= max_fb);
		++freq[fb];
	}
}

#if 0
// Implementation in C, with loop unfolding.
static void count_freq_c_luf4(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64])
{
	UpdateCallCounter(count);

	memset(freq, 0, sizeof(unsigned int)*MM_FEEDBACK_COUNT);

	for (; count >= 4; count -= 4) {
		++freq[*(feedbacks++) & 0x3f];
		++freq[*(feedbacks++) & 0x3f];
		++freq[*(feedbacks++) & 0x3f];
		++freq[*(feedbacks++) & 0x3f];
	}
	for (; count > 0; count--) {
		++freq[*(feedbacks++) & 0x3f];
	}
}
#endif

#if 0
// This is a asm-free implementation of the freqency counting procedure.
// It works fine on my AMD64 processor with a 32-bit OS, though not as fast as the
// ASM implementation. The INTERLACED version works better.
static void count_freq_v9(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64])
{
	// We build four frequency tables in parallel.
	// Suppose the input is:
	//    b0  b1  b2  b3  b4  b5  b6  b7  ...
	// They will be scanned interlaced and built into four frequency tables:
	//    table1: b0, b4, b8, ...
	//    table2: b1, b5, b9, ...
	//    table3: b2, b6, b10, ...
	//    table4: b3, b7, b11, ...
	// Since we need to build four tables simultaneously, we allocate
	// four times the memory on stack:
	//    unsigned int freq[1024];
	// There are two possible ways to store the frequencies:
	// Possibility 1 (sequential):
	//    table1: in freq[0..256]
	//    table2: in freq[256..512]
	//    table3: in freq[512..768]
	//    table4: in freq[768..1024]
	// Possibility 2 (interlaced):
	//    table1: in freq[0,4,8,...]
	//    table2: in freq[1,5,9,...]
	//    table3: in freq[2,6,10,...]
	//    table4: in freq[3,7,11,...]
	// We use the second approach, because it is much faster when adding
	// up the four tables to get the final result.

	// Note: we need to allocate more memory in case there are 
	// feedback >= 0x80. However, doing so degrades performance 
	// significantly (like 10%). So we need to figure out a better way,
	// probably mapping them to smaller memory further!

	UpdateCallCounter(count);

	int i;

#define INTERLACED 1

#if INTERLACED
	//unsigned int big_freq[64*4];
	//memset(big_freq, 0, sizeof(big_freq));
	unsigned int matrix[64][4];
#else
	unsigned int matrix[4][64];
#endif

	memset(matrix, 0, sizeof(matrix));
	for (; count >= 4; count -= 4) {
		unsigned int a = *(int *)feedbacks; // a contains 4 feedbacks
		a &= 0x3f3f3f3f;

#if INTERLACED
		/*if (1) {
			++big_freq[4*(a & 0xff)];
			++big_freq[4*((a>>8)&0xff)+1];
			++big_freq[4*((a>>16)&0xff)+2];
			++big_freq[4*((a>>24)&0xff)+3];
		} else {
			++big_freq[4*((unsigned char)a)];
			++big_freq[4*((unsigned char)(a>>8))+1];
			++big_freq[4*((unsigned char)(a>>16))+2];
			++big_freq[4*((unsigned char)(a>>24))+3];
		}
		*/
		++matrix[a & 0xff][0];
		++matrix[(a>>8) & 0xff][1];
		++matrix[(a>>16) & 0xff][2];
		++matrix[(a>>24) & 0xff][3];
#else
		++matrix[0][a & 0xff];
		++matrix[1][(a>>8) & 0xff];
		++matrix[2][(a>>16) & 0xff];
		++matrix[3][(a>>24) & 0xff];
#endif
		feedbacks += 4;
	}

	// Add up four frequency tables to get the final result
#if INTERLACED
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[i][0]+matrix[i][1]+matrix[i][2]+matrix[i][3];
	}
#else
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i];
	}
#endif

	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++);
		fb &= 0x3f;
		++freq[fb];
	}
	freq[63] = 0;

#undef INTERLACED
}
#endif

#if 0
// This is the choice implementation for my Intel Core i5 processor. It is as fast
// as the ASM version. The non-INTERLACED version works better.
static void count_freq_v10(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64])
{
	UpdateCallCounter(count);

	// We build eight frequency tables in parallel.
	// Note: we could probably improve performance marginally by mapping feedback
	// to a smaller integer, and allocate smaller memory for 'matrix'.
	int i;

	// UpdateStatistics(count);

#define INTERLACED 1

#if INTERLACED
	unsigned int matrix[64][8];
#else
	unsigned int matrix[8][64];
#endif

	memset(matrix, 0, sizeof(matrix));
	for (; count >= 8; count -= 8) {
		unsigned int a1 = ((int *)feedbacks)[0];	// a1 contains 4 feedbacks
		unsigned int a2 = ((int *)feedbacks)[1];	// a2 contains 4 feedbacks
		a1 &= 0x3f3f3f3f;
		a2 &= 0x3f3f3f3f;

#if INTERLACED
		++matrix[a1 & 0xff][0];
		++matrix[(a1>>8) & 0xff][1];
		++matrix[(a1>>16) & 0xff][2];
		++matrix[(a1>>24) & 0xff][3];

		++matrix[a2 & 0xff][4];
		++matrix[(a2>>8) & 0xff][5];
		++matrix[(a2>>16) & 0xff][6];
		++matrix[(a2>>24) & 0xff][7];
#else
		++matrix[0][a1 & 0xff];
		++matrix[1][(a1>>8) & 0xff];
		++matrix[2][(a1>>16) & 0xff];
		++matrix[3][(a1>>24) & 0xff];

		++matrix[4][a2 & 0xff];
		++matrix[5][(a2>>8) & 0xff];
		++matrix[6][(a2>>16) & 0xff];
		++matrix[7][(a2>>24) & 0xff];
#endif
		feedbacks += 8;
	}

	// Add up the eight frequency tables to get the final result
#if INTERLACED
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[i][0]+matrix[i][1]+matrix[i][2]+matrix[i][3]
				+ matrix[i][4]+matrix[i][5]+matrix[i][6]+matrix[i][7];
	}
#else
	//for (i = 0; i < 63; i++) {
	//	freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i]
	//			+ matrix[4][i]+matrix[5][i]+matrix[6][i]+matrix[7][i];
	//}
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i];
	}
	for (i = 0; i < 63; i++) {
		freq[i] += matrix[4][i]+matrix[5][i]+matrix[6][i]+matrix[7][i];
	}

#endif

#undef INTERLACED

	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++);
		fb &= 0x3f;
		++freq[fb];
	}
	freq[63] = 0;
}
#endif

#if 0
static void count_freq_v11(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64])
{
	if (count <= 160) {
		memset(freq, 0, sizeof(unsigned int)*64);
		for (; count > 0; count--) {
			unsigned char fb = *(feedbacks++);
			fb &= 0x3f;
			++freq[fb];
		}
		freq[63] = 0;
		return;
	}

	// We build eight frequency tables in parallel.
	// Note: we could probably improve performance marginally by mapping feedback
	// to a smaller integer, and allocate smaller memory for 'matrix'.
	int i;

	// UpdateStatistics(count);

#define INTERLACED 1

#if INTERLACED
	unsigned int matrix[64][8];
#else
	unsigned int matrix[8][64];
#endif

	memset(matrix, 0, sizeof(matrix));
	for (; count >= 8; count -= 8) {
		unsigned int a1 = ((int *)feedbacks)[0];	// a1 contains 4 feedbacks
		unsigned int a2 = ((int *)feedbacks)[1];	// a2 contains 4 feedbacks
		a1 &= 0x3f3f3f3f;
		a2 &= 0x3f3f3f3f;

#if INTERLACED
		++matrix[a1 & 0xff][0];
		++matrix[(a1>>8) & 0xff][1];
		++matrix[(a1>>16) & 0xff][2];
		++matrix[(a1>>24) & 0xff][3];

		++matrix[a2 & 0xff][4];
		++matrix[(a2>>8) & 0xff][5];
		++matrix[(a2>>16) & 0xff][6];
		++matrix[(a2>>24) & 0xff][7];
#else
		++matrix[0][a1 & 0xff];
		++matrix[1][(a1>>8) & 0xff];
		++matrix[2][(a1>>16) & 0xff];
		++matrix[3][(a1>>24) & 0xff];

		++matrix[4][a2 & 0xff];
		++matrix[5][(a2>>8) & 0xff];
		++matrix[6][(a2>>16) & 0xff];
		++matrix[7][(a2>>24) & 0xff];
#endif
		feedbacks += 8;
	}

	// Add up the eight frequency tables to get the final result
#if INTERLACED
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[i][0]+matrix[i][1]+matrix[i][2]+matrix[i][3]
				+ matrix[i][4]+matrix[i][5]+matrix[i][6]+matrix[i][7];
	}
#else
	//for (i = 0; i < 63; i++) {
	//	freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i]
	//			+ matrix[4][i]+matrix[5][i]+matrix[6][i]+matrix[7][i];
	//}
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i];
	}
	for (i = 0; i < 63; i++) {
		freq[i] += matrix[4][i]+matrix[5][i]+matrix[6][i]+matrix[7][i];
	}

#endif

#undef INTERLACED

	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++);
		fb &= 0x3f;
		++freq[fb];
	}
	freq[63] = 0;
}
#endif

static unsigned int ComputeSumOfSquares_v1(
	const unsigned int freq[],
	unsigned char max_fb)
{
	unsigned int ret = 0;
	for (int i = 0; i <= (int)max_fb; i++) {
		ret += freq[i] * freq[i];
	}
	return ret;
}

#if 0
static unsigned int ComputeSumOfSquares_v2(const unsigned int freq[MM_FEEDBACK_COUNT])
{
	unsigned int ret = 0;
	for (int i = 0; i < MM_FEEDBACK_COUNT; i += 2) {
		unsigned int v1 = freq[i];
		unsigned int v2 = freq[i+1];
		v1 *= v1;
		v2 *= v2;
		ret += v1;
		ret += v2;
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
// Interface routine
//

static FrequencyCountingRoutineSelector::RoutineEntry CountFrequencies_Entries[] = {
	{ "c", "Simple implementation", count_freq_c },
	//{ "c_luf4", "Simple implementation with loop unfolding", count_freq_c_luf4 },
	//{ "c_p8_il", "Standard implementation (8-parallel, interlaced)", count_freq_v10 },
	//{ "c_p8_il_os", "Standard implementation (8-parallel, interlaced, optimized for small list)", count_freq_v11 },
	{ NULL, NULL, NULL },
};

FrequencyCountingRoutineSelector *CountFrequenciesImpl = 
	new FrequencyCountingRoutineSelector(CountFrequencies_Entries, "c");

static FrequencySumSquaresRoutineSelector::RoutineEntry GetSumOfSquares_Entries[] = {
	{ "c", "Simple implementation", ComputeSumOfSquares_v1 },
	//{ "c_p2", "Simple implementation with 2-parallel", ComputeSumOfSquares_v2 },
	//{ "sse4", "SIMD implementation (requires SSE4 instruction set)", ComputeSumOfSquares_v3 },
	{ NULL, NULL, NULL },
};

FrequencySumSquaresRoutineSelector *GetSumOfSquaresImpl =
	new FrequencySumSquaresRoutineSelector(GetSumOfSquares_Entries, 
	//(Utilities::CpuInfo::Features.WithSSE41)? "sse4" : "c");
	"c");
