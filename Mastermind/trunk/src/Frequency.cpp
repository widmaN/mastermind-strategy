#include <assert.h>
#include <memory.h>
#include <emmintrin.h>

#include "MMConfig.h"
#include "Frequency.h"

// Implementation in C.
// It's very astonishing that by changing the parameter <code>count</code>
// from unsigned to signed, it degrades performance significantly.
// The compiler generates an extra instruction to test the loop 
// condition <code>count < 0</code>, 
// probably because the DEC instruction doesn't modify the CF flag.
MM_IMPLEMENTATION 
void count_freq_v1(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64])
{
	assert(feedbacks != 0);
	assert(freq != 0);

	if (0) {
		memset(freq, 0, 64*4);
		while (count-- > 0) {
			++freq[*(feedbacks++)];
		}
	} else {
		unsigned char cache = 0xff;
		int cache_count = 0;
		int switch_count = 0;
		memset(freq, 0, 64*4);
		while (count-- > 0) {
			unsigned char fb = *(feedbacks++);
			if (fb == cache) {
				cache_count++;
			} else {
				switch_count++;
				freq[cache] += cache_count;
				cache_count = 1;
				cache = fb;
			}
		}
		if (cache_count > 0) {
			freq[cache] += cache_count;
		}
		//printf("Switch count: %d\n", switch_count);
	}
}

// Implementation in C, with an attempt to enable Out-of-Order execution.
MM_IMPLEMENTATION
void count_freq_v3(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[256])
{
	assert(feedbacks != 0);
	assert(freq != 0);
	assert(count >= 0);

	memset(freq, 0, 1024);

	for (; count > 0; count -= 4) {
		++freq[feedbacks[0]];
		++freq[feedbacks[1]];
		++freq[feedbacks[2]];
		++freq[feedbacks[3]];
		feedbacks += 4;
	}
}

// This is a asm-free implementation of the freqency counting procedure.
// It works fine on my AMD64 processor with a 32-bit OS, though not as fast as the
// ASM implementation. The INTERLACED version works better.
void count_freq_v9(
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

// This is the choice implementation for my Intel Core i5 processor. It is as fast
// as the ASM version. The non-INTERLACED version works better.
void count_freq_v10(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[64])
{
	// We build eight frequency tables in parallel.
	// Note: we could probably improve performance marginally by mapping feedback
	// to a smaller integer, and allocate smaller memory for 'matrix'.
	int i;

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
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i]
				+ matrix[4][i]+matrix[5][i]+matrix[6][i]+matrix[7][i];
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

unsigned int ComputeSumOfSquares_v1(const unsigned int freq[MM_FEEDBACK_COUNT])
{
	unsigned int ret = 0;
	for (int i = 0; i < MM_FEEDBACK_COUNT; i++) {
		ret += freq[i] * freq[i];
	}
	return ret;
}

unsigned int ComputeSumOfSquares_v2(const unsigned int freq[MM_FEEDBACK_COUNT])
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

///////////////////////////////////////////////////////////////////////////
// Interface routine
//

void CountFrequencies_Impl(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[MM_FEEDBACK_COUNT])
{
#if THIS_IS_AMD_PROCESSOR
	count_freq_v9(feedbacks, count, freq);
#else
	count_freq_v10(feedbacks, count, freq);
	//count_freq_v9(feedbacks, count, freq);
#endif
}
