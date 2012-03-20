#include "Codeword.hpp"
#include "Algorithm.hpp"
#include "util/simd.hpp"

using namespace Mastermind;

// Routine to compute the color mask of an array of codewords.

// This implementation works with "long" codeword.
// The performance is slightly (5-10%) worse than v2,
// but the implementation is much more readable!
// Plus, this routine is not the bottleneck!
// So we'll use v1 in production.
static unsigned short ScanDigitMask_v1(
	const Codeword *_first,
	const Codeword *_last)
{
	typedef util::simd::simd_t<uint8_t,16> simd_t;

	simd_t mask = simd_t::zero();
	const simd_t *first = (const simd_t *)_first;
	const simd_t *last = (const simd_t *)_last;
	for (const simd_t *it = first; it != last; ++it)
	{
		mask |= *it;
	}

	mask = (mask == simd_t::zero());
	unsigned short result = (unsigned short)util::simd::byte_mask(mask);
	result = (~result) & ((1 << MM_MAX_COLORS) - 1);
	return result;
}

#if 0
/// This implementation works with "long" codeword. 4-Parallel.
unsigned short ScanDigitMask_v2(
	const Codeword *first,
	const Codeword *last)
{
	const __m128i *codewords = (const __m128i *)first;
	unsigned int count = last - first;

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
#endif

///////////////////////////////////////////////////////////////////////////
// Interface Routines
//

REGISTER_ROUTINE(MaskRoutine, "generic", ScanDigitMask_v1)
#if 0
REGISTER_ROUTINE(MaskRoutine, "unrolled", ScanDigitMask_v2)
#endif
