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

// Simple implementation in ASM. This implementation allocates an extra
// frequency array on the stack to parallel the execution. The routine
// doesn't handle unaligned input.
MM_IMPLEMENTATION
void count_freq_v2(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[256])
{
#if MM_ALLOW_ASM
	unsigned int freq2[256];
	int i;

	assert(feedbacks != 0);
	assert(freq != 0);
	assert(count >= 0);

	memset(freq, 0, 1024);
	memset(freq2, 0, 1024);
	if (count == 0)
		return;

	__asm {
		mov esi, feedbacks;          // ESI:=feedback list
		mov edi, freq;			     // EDI:=frequency table (256 DWORDs)
		mov ecx, dword ptr [count];  // ECX:=loop counter
		lea edx, [freq2];

loop_start:
		movzx eax, byte ptr [esi];   // EAX:=feedback 1
		movzx ebx, byte ptr [esi+1]; // EBX:=feedback 2
		inc dword ptr [edi+eax*4];   // increment freq[EAX]
		inc dword ptr [edx+ebx*4];   // increment freq[EBX]

		add esi, 2;
		sub ecx, 2;
		jg  loop_start;
	}

	// add freq and freq2
	for (i = 0; i < 256; i++) {
		freq[i] += freq2[i];
	}
#endif // MM_ALLOW_ASM
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


// ASM implementation, with loop unrolling. This routine processes 
// four feedbacks at a time. The routine doesn't handle unaligned input.
MM_IMPLEMENTATION
void count_freq_v4(
	const unsigned char *feedbacks,
	int count,
	unsigned int freq[256])
{
#if MM_ALLOW_ASM
	assert(feedbacks != 0);
	assert(freq != 0);
	assert(count >= 0);

	memset(freq, 0, 1024);
	if (count == 0)
		return;

	__asm {
		mov esi, feedbacks;          // ESI:=feedback list
		mov edi, freq;			     // EDI:=frequency table (256 DWORDs)
		mov ecx, dword ptr [count];  // ECX:=loop counter

loop_start:
		mov edx, dword ptr [esi];
		movzx eax, dl;
		shr edx, 8;
		inc dword ptr [edi+eax*4];   // increment freq[EAX]
		movzx eax, dl;
		shr edx, 8;
		inc dword ptr [edi+eax*4];   // increment freq[EAX]
		movzx eax, dl;
		shr edx, 8;
		inc dword ptr [edi+eax*4];   // increment freq[EAX]
		movzx eax, dl;
		inc dword ptr [edi+eax*4];   // increment freq[EAX]

		add esi, 4;
		sub ecx, 4;
		jg  loop_start;
	}
#endif // MM_ALLOW_ASM
}

// Improved ASM, using four temporary tables to parallel. This routine
// doesn't support unaligned input.
MM_IMPLEMENTATION
void count_freq_v5(
	const unsigned char *feedbacks,
	int count,
	unsigned int _freq[128])
{
#if MM_ALLOW_ASM
	unsigned int freq[1024];
	int i;

	assert(feedbacks != 0);
	assert(_freq != 0);
	assert(count >= 0);

	memset(freq, 0, sizeof(freq));
	if (count == 0)
		return; //  TBD

	__asm {
		mov esi, dword ptr [feedbacks];  // ESI:=feedback list
		mov edi, dword ptr [count];      // EDI:=loop counter

loop_start:
		// move dword into EAX,EBX,ECX,EDX.
		// make use of as much Out-of-Order as possible.
		mov eax, dword ptr [esi];
		mov ecx, eax;
		movzx ebx, ah;
		movzx eax, al;
		bswap ecx;
		movzx edx, ch;
		movzx ecx, cl;

		shl eax, 2; // interlaced
		shl ebx, 2; 
		shl ecx, 2; 
		shl edx, 2;

		inc dword ptr [freq+eax*4+0];  // increment freq[0,EAX]
		inc dword ptr [freq+ebx*4+4];  // increment freq[1,EBX]
		inc dword ptr [freq+ecx*4+8];  // increment freq[2,ECX]
		inc dword ptr [freq+edx*4+12]; // increment freq[3,EDX]

		add esi, 4;
		sub edi, 4;
		jg  loop_start;
	}

	// add freq and freq2
	for (i = 0; i < 0x80; i++) {
		_freq[i] = freq[i*4+0] + freq[i*4+1] + freq[i*4+2] + freq[i*4+3];
	}
#endif // MM_ALLOW_ASM
}

/// Improved ASM from v5 to handle unaligned input. However, this
/// routine requires that any feedback be smaller than 0x7F.
/// If a feedback is greater than or equal to 0x7F, the frequency table
/// is undefined.
/// \bug This implementation assumes data segment <code>DS</code> is
/// equal to stack segment <code>SS</code>. If they are not equal,
/// the behavior is undefined (and will most likely crash).
MM_IMPLEMENTATION
void count_freq_v6(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int _freq[MM_FEEDBACK_COUNT])
{
#if MM_ALLOW_ASM
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
	unsigned int freq[MM_FEEDBACK_COUNT][4];

	// Use a dummy buffer for unaligned count. 
	// Fill the buffer with 7A15B, which is impossible because we constraint
	// the maximum length of the codeword to be 7.
	unsigned int dummy = (MM_FEEDBACK_COUNT-1)
		| ((MM_FEEDBACK_COUNT-1) << 8)
		| ((MM_FEEDBACK_COUNT-1) << 16)
		| ((MM_FEEDBACK_COUNT-1) << 24);

	assert(feedbacks != 0);
	assert(_freq != 0);
	assert(count >= 0);

	//memset(freq, 0, sizeof(unsigned int)*0x80*4);
	memset(freq, 0, sizeof(freq));

	__asm {
		mov edi, dword ptr [count];      // EDI:=loop counter
		mov esi, dword ptr [feedbacks];  // ESI:=feedback list
		cld;  // increment ESI after LODSD instruction

loop_start:

		// If fewer than 3 elements, go to unaligned hanler
		sub edi, 4;
		jb  unaligned_handler;

		// Move dword into EAX,EBX,ECX,EDX.
		// The required operation is as follows:
		//   byte3   byte2   byte1   byte0
		//     |       |       |       |
		//    EAX     EBX     ECX     EDX
		// Then each register is multiplied by 4 to locate to its
		// position after interlacing.

		// The implementation below optimizes for Out-of-Order execution.
		mov edx, (MM_FEEDBACK_COUNT-1);
		mov ecx, ((MM_FEEDBACK_COUNT-1) << 8);
		mov ebx, ((MM_FEEDBACK_COUNT-1) << 16);

		lodsd;           // EAX=[ESI], ESI+=4
		and edx, eax;
		and ecx, eax;
		and ebx, eax;

		shl eax, (8-MM_FEEDBACK_BITS);
		shr eax, (8-MM_FEEDBACK_BITS)+24-2;
		shr ebx, (16-2);
		shr ecx, (8-2);
		shl edx, 2;

		inc dword ptr [freq+eax*4+0];  // increment freq[0,EAX]
		inc dword ptr [freq+ebx*4+4];  // increment freq[1,EBX]
		inc dword ptr [freq+ecx*4+8];  // increment freq[2,ECX]
		inc dword ptr [freq+edx*4+12]; // increment freq[3,EDX]

		jmp loop_start;

unaligned_handler:
		add edi, 4;
		jz addup_result;

		// Copy the unaligned bytes into dummy variable
		mov ecx, edi;
		lea edi, [dummy];
		rep movsb; // move CX bytes from ESI(feedbacks) to EDI(dummy)

		mov edi, 4;
		lea esi, [dummy];
		jmp loop_start;

addup_result:

	}

	// Add up four frequency tables to get the final result
	for (int i = 0; i < MM_FEEDBACK_COUNT-1; i++) {
		_freq[i] = freq[i][0] + freq[i][1] + freq[i][2] + freq[i][3];
	}
	_freq[MM_FEEDBACK_COUNT-1] = 0;

#endif // MM_ALLOW_ASM
}

// Doesn't work; 5 times slower than v6
void count_freq_v7(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int freq[128])
{
	// Find out all possible feedbacks
	unsigned char all_fbs[16] = {
		0x00, 0x01, 0x02, 0x03, 
		0x04, 0x10, 0x11, 0x12, 
		0x13, 0x20, 0x21, 0x22,
		0x30, 0x40, 0xff, 0xff, };
	__m128i vall = _mm_loadu_si128((__m128i*)all_fbs);
	__m128i counter = _mm_setzero_si128();

	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++);
		__m128i vfb = _mm_set1_epi8(fb);
		__m128i cmp = _mm_cmpeq_epi8(vfb, vall);
		counter = _mm_sub_epi8(counter, cmp);
	}

	// Copy results
	_mm_storeu_si128((__m128i*)freq, counter);
}

void count_freq_v8(
	const unsigned char *feedbacks,
	unsigned int count,
	unsigned int _freq[64])
{
	const unsigned char bytes[16] = {
		0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15 };
	__m128i counter0 = _mm_setzero_si128();
	__m128i counter1 = _mm_setzero_si128();
	__m128i counter2 = _mm_setzero_si128();
	__m128i counter3 = _mm_setzero_si128();
	__m128i mask = _mm_loadu_si128((const __m128i*)bytes);

	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++) & 0x3f;
		__m128i vfb = _mm_set1_epi8(fb);
		__m128i bit = _mm_cmpeq_epi8(vfb, mask);

		switch (fb >> 4) {
		case 0:
			counter0 = _mm_sub_epi8(counter0, bit);
			break;
		case 1:
			counter1 = _mm_sub_epi8(counter1, bit);
			break;
		case 2:
			counter2 = _mm_sub_epi8(counter2, bit);
			break;
		case 3:
			counter3 = _mm_sub_epi8(counter3, bit);
			break;
		}
	}

	// Save result
	_mm_storeu_si128((__m128i*)&_freq[0], counter0);
	_mm_storeu_si128((__m128i*)&_freq[16], counter1);
	_mm_storeu_si128((__m128i*)&_freq[32], counter2);
	_mm_storeu_si128((__m128i*)&_freq[48], counter3);

}

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

#define INTERLACED 0

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
	freq[63] = 0;
#else
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i];
	}
	freq[63] = 0;
#endif

#undef INTERLACED
}

void count_freq_v10(
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

#define INTERLACED 0

#if INTERLACED
	//unsigned int big_freq[64*4];
	//memset(big_freq, 0, sizeof(big_freq));
	unsigned int matrix[64][4];
#else
	//unsigned int matrix[4][64];
	unsigned int matrix[8][64];
#endif

	memset(matrix, 0, sizeof(matrix));
	for (; count >= 8; count -= 8) {
		unsigned int a = *(int *)feedbacks; // a contains 4 feedbacks
		unsigned int a2 = *((int *)feedbacks+1); // a contains 4 feedbacks
		a &= 0x3f3f3f3f;
		a2 &= 0x3f3f3f3f;

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

		++matrix[4][a2 & 0xff];
		++matrix[5][(a2>>8) & 0xff];
		++matrix[6][(a2>>16) & 0xff];
		++matrix[7][(a2>>24) & 0xff];
#endif
		feedbacks += 8;
	}

	// Add up four frequency tables to get the final result
#if INTERLACED
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[i][0]+matrix[i][1]+matrix[i][2]+matrix[i][3];
	}
	freq[63] = 0;
#else
	for (i = 0; i < 64; i++) {
		//freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i];
		freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i]
				+ matrix[4][i]+matrix[5][i]+matrix[6][i]+matrix[7][i];
	}
	freq[63] = 0;
#endif

#undef INTERLACED

	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++);
		fb &= 0x3f;
		++freq[fb];
	}
	freq[63] = 0;

}

void count_freq_v11(
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

#define INTERLACED 0

#if INTERLACED
	//unsigned int big_freq[64*4];
	//memset(big_freq, 0, sizeof(big_freq));
	unsigned int matrix[64][8];
#else
	//unsigned int matrix[4][64];
	unsigned int matrix[8][64];
#endif

	memset(matrix, 0, sizeof(matrix));
	for (; count >= 8; count -= 8) {
		__int64 a = *(__int64 *)feedbacks; // a contains 8 feedbacks
		a &= 0x3f3f3f3f3f3f3f3f;

#if INTERLACED
		++matrix[a & 0xff][0];
		++matrix[(a>>8) & 0xff][1];
		++matrix[(a>>16) & 0xff][2];
		++matrix[(a>>24) & 0xff][3];

		++matrix[a2 & 0xff][0];
		++matrix[(a2>>8) & 0xff][1];
		++matrix[(a2>>16) & 0xff][2];
		++matrix[(a2>>24) & 0xff][3];

#else
		++matrix[0][a & 0xff];
		++matrix[1][(a>>8) & 0xff];
		++matrix[2][(a>>16) & 0xff];
		++matrix[3][(a>>24) & 0xff];
		++matrix[4][(a>>32) & 0xff];
		++matrix[5][(a>>40) & 0xff];
		++matrix[6][(a>>48) & 0xff];
		++matrix[7][(a>>56) & 0xff];
#endif
		feedbacks += 8;
	}

	// Add up four frequency tables to get the final result
#if INTERLACED
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[i][0]+matrix[i][1]+matrix[i][2]+matrix[i][3];
	}
	freq[63] = 0;
#else
	for (i = 0; i < 63; i++) {
		freq[i] = matrix[0][i]+matrix[1][i]+matrix[2][i]+matrix[3][i]
				+ matrix[4][i]+matrix[5][i]+matrix[6][i]+matrix[7][i];
	}
	freq[63] = 0;
#endif

	for (; count > 0; count--) {
		unsigned char fb = *(feedbacks++);
		fb &= 0x3f;
		++freq[fb];
	}
	freq[63] = 0;

#undef INTERLACED

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
#if MM_ALLOW_ASM
	count_freq_v6(feedbacks, count, freq);
#else
	count_freq_v1(feedbacks, count, freq);
#endif
}
