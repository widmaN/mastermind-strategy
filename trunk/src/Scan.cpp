////////////////////////////////////////////////////////////////////////////
/// \file Scan.cpp
/// Routines to scan an array of codewords for digit mask.
///
/// If the codewords are stored as nibbles in 16-bit or 32-bit integer,
/// the algorithm is simple: scan each nibble to get a bit-mask, and then
/// <code>OR</code> all the masks together. Note that nibble <code>0xF</code>
/// is never set in the mask because it is reserved for padding use.
///
/// If the codewords are stored in "long" format, the algorithm counts
/// the digit count range directly.
///

#include <assert.h>
#include <emmintrin.h>

#include "MMConfig.h"
#include "Scan.h"

////////////////////////////////////////////////////////////////////////////
// Implementation routines
//

// This implementation works with "long" codeword.
// The performance is slightly worse than long_v2 (15% worse),
// but the implementation is much simpler!
// Plus, this routine is not the bottleneck!
// So we'll use v1 in production.
unsigned short ScanDigitMask_long_v1(
	const __m128i *codewords,
	unsigned int count)
{
	unsigned short result;
	__m128i mask = _mm_setzero_si128();

	for (; count > 0; count--) {
		__m128i cw = *codewords++;
		mask = _mm_or_si128(mask, cw);
	}
	
	mask = _mm_cmpeq_epi8(mask, _mm_setzero_si128());
	result = _mm_movemask_epi8(mask);
	result = (~result) & ((1 << MM_MAX_COLORS) - 1);
	return result;
}

/// This implementation works with "long" codeword. 4-Parallel.
unsigned short ScanDigitMask_long_v2(
	const __m128i *codewords,
	unsigned int count)
{
	__m128i mask1 = _mm_setzero_si128();
	__m128i mask2 = _mm_setzero_si128();
	__m128i mask3 = _mm_setzero_si128();
	__m128i mask4 = _mm_setzero_si128();
	unsigned short result;

	for (; count >= 4; count -= 4) {
		__m128i cw1 = codewords[0];
		__m128i cw2 = codewords[1];
		__m128i cw3 = codewords[2];
		__m128i cw4 = codewords[3];
		codewords += 4;
		mask1 = _mm_or_si128(mask1, cw1);
		mask2 = _mm_or_si128(mask2, cw2);
		mask3 = _mm_or_si128(mask3, cw3);
		mask4 = _mm_or_si128(mask4, cw4);
	}
	
	mask1 = _mm_or_si128(mask1, mask2);
	mask3 = _mm_or_si128(mask3, mask4);
	mask1 = _mm_or_si128(mask1, mask3);
	mask1 = _mm_cmpeq_epi8(mask1, _mm_setzero_si128());
	result = _mm_movemask_epi8(mask1);
	result = (~result) & ((1 << MM_MAX_COLORS) - 1);
	return result;
}

///////////////////////////////////////////////////////////////////////////
// Interface Routines
//

unsigned short ScanDigitMask(const codeword_t *codewords, unsigned int count)
{
	return ScanDigitMask_long_v1((__m128i*)codewords, count);
}
