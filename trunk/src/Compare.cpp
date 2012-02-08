///////////////////////////////////////////////////////////////////////////
// Codeword comparison routines.

#include "util/simd.hpp"
#include "util/intrinsic.hpp"
#include "Algorithm.hpp"

// Define NTEST to exclude non-essential routines from compilation.
#define NTEST

#if ENABLE_CALL_COUNTER
#include "CallCounter.h"

static Utilities::CallCounter _call_counter("CompareCodewords", true);

static inline void UpdateCallCounter(unsigned int comp)
{
	_call_counter.AddCall(comp);
}

void PrintCompareStatistics()
{
	_call_counter.DebugPrint();
}
#endif

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// Generic comparison routine for repeatable codewords.

// Precompute a conversion table that converts (nA<<4|nAB) -> feedback.
// Both nA and nAB must be >= 0 and <= 15.
struct generic_feedback_mapping_t
{
	unsigned char table[0x100];

	generic_feedback_mapping_t()
	{
		for (int i = 0; i < 0x100; i++) 
		{
			int nA = i >> 4;
			int nAB = i & 0xF;
#if MM_FEEDBACK_COMPACT
			table[i] = nAB*(nAB+1)/2+nA;
#else
			table[i] = (nA << MM_FEEDBACK_ASHIFT) | (nAB - nA);
#endif
		}
	}
};

static const generic_feedback_mapping_t generic_feedback_mapping;

// SSE2-based implementation for comparing generic codewords.
// Repeated colors are allowed.
static void compare_long_codeword_generic(
	const CodewordRules &rules,
	const Codeword &_secret,
	const Codeword *_first,
	const Codeword *_last,
	Feedback result[])
{
	// UpdateCallCounter(count);
	using namespace util::simd;
	typedef util::simd::simd_t<uint8_t,16> simd_t;

	simd_t secret = _secret.value();

	// Change 0xff in secret to 0x0f.
	secret &= (uint8_t)0x0f;

	// Create mask for use later.
	const simd_t mask_pegs = fill_left<MM_MAX_PEGS>((uint8_t)0x01);
	const simd_t mask_colors = fill_right<MM_MAX_COLORS>((uint8_t)0xff);

	// Note: we write an explicit loop since std::transform() is too 
	// slow, because VC++ does not inline the lambda expression, thus
	// making each iteration a CALL with arguments passed on the stack.
	const simd_t *first = reinterpret_cast<const simd_t *>(_first);
	const simd_t *last = reinterpret_cast<const simd_t *>(_last);
	for (const simd_t *guesses = first; guesses != last; ++guesses)
	{
		const simd_t &guess = *guesses;

		// Compute nA.
		// It turns out that there's a significant (20%) performance
		// hit if <code>nA = sum_high(...)</code> is changed to 
		// <code>nA = sum(...)</code>. In the latter case, VC++
		// VC++ generates an extra (redundant) memory read.
		// The reason is unclear. (Obviously there are still free
		// XMM registers.)
#if MM_MAX_PEGS <= 8
		int nA = sum_high((secret == guess) & mask_pegs);
#else
		int nA = sum((secret == guess) & mask_pegs);
#endif

		// Compute nAB.
#if MM_MAX_COLORS <= 8
		int nAB = sum_low(min(secret, guess) & mask_colors);
#else
		int nAB = sum(min(secret, guess) & mask_colors);
#endif

		unsigned char nAnB = generic_feedback_mapping.table[(nA<<4)|nAB];
		*(result++) = nAnB;
	}
}

///////////////////////////////////////////////////////////////////////////
// Specialized comparison routine for NON-repeatable codewords.

// Pre-computed table that converts a comparison bitmask of
// non-repeatable codewords into a feedback.
//
// Note that the performance impact of the table lookup can be subtle. 
// The VC++ compiler tends to generate a MOVZX (32-bit) or MOVSXD (64-bit)
// instruction if the integer sizes do not match. What's worse, because
// an SSE2 intrinsic function in use returns an <code>int</code>, there
// is no way to get rid of the MOVSXD instruction which deteriates 
// performance by 30%!
//
// Hence, the best we can do is to compile under 32-bit, and access 
// the table directly.
struct norepeat_feedback_mapping_t
{
	unsigned char table[0x10000];

	norepeat_feedback_mapping_t()
	{
		for (int i = 0; i < 0x10000; i++) 
		{
			int nA = util::intrinsic::pop_count(i >> MM_MAX_COLORS);
			int nAB = util::intrinsic::pop_count(i & ((1<<MM_MAX_COLORS)-1));
#if MM_FEEDBACK_COMPACT
			table[i] = nAB*(nAB+1)/2+nA;
#else
			table[i] = (nA << MM_FEEDBACK_ASHIFT) | (nAB - nA);
#endif
		}
	}
};

static const norepeat_feedback_mapping_t norepeat_feedback_mapping;

// Comparison routine for <i>non-repeatable</i> codewords.
// We build a cache that pre-computes feedback from bitmask.
// This is much (3x) faster than counting the bits each time.
static void compare_long_codeword_norepeat(
	const CodewordRules &rules,
	const Codeword &_secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[])
{
	//UpdateCallCounter(count);
	typedef util::simd::simd_t<char,16> simd_t;
	simd_t secret = _secret.value();

	// Change 0xFF in secret to 0x0F
	secret &= (char)0x0f;

	// Set zero counters in secret to 0xFF, so that if a counter in the
	// guess and secret are both zero, they won't compare equal.
	secret |= util::simd::keep_right<MM_MAX_COLORS>(secret == simd_t::zero());

	// Scan codeword array.
	const simd_t *guesses = (const simd_t *)first;
	size_t count = last - first;
	for (; count > 0; --count)
	{
#if 0
		const simd_t &guess = *guesses;
		const simd_t &cmp = (guess == secret);
		int mask = util::simd::byte_mask(cmp);
		unsigned char nAnB = norepeat_feedback_mapping.table[mask];
		*(result++) = nAnB;
		++guesses;
#else
		*(result++) = norepeat_feedback_mapping.table
			[ util::simd::byte_mask(*(guesses++) == secret) ];
#endif
	}
}

#ifndef NTEST
// Comparison routine for non-repeatable codewords.
// It is a manually-unrolled version of compare_long_codeword_norepeat().
static void compare_long_codeword_norepeat_p4(
	const CodewordRules &rules,
	const Codeword& _secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[])
{
	//UpdateCallCounter(count);
	__m128i secret = _secret.value();

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

	const __m128i *guesses = (const __m128i *)first;
	size_t count = last - first;
	for (; count >= 4; count -= 4) 
	{
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

		*(unsigned int *)result = t;
		result += 4;
	}

	// Scan the remaining codewords
	for (; count > 0; count--) 
	{
		__m128i guess = *(guesses++);
		__m128i cmp = _mm_cmpeq_epi8(guess, secret);
		int mask = _mm_movemask_epi8(cmp);
		unsigned char nAnB = counter[mask];
		*(result++) = nAnB;
	}
}
#endif // NTEST

///////////////////////////////////////////////////////////////////////////
// Routine registration.

REGISTER_ROUTINE(ComparisonRoutine, "generic", compare_long_codeword_generic)
REGISTER_ROUTINE(ComparisonRoutine, "norepeat", compare_long_codeword_norepeat)
#ifndef NTEST
REGISTER_ROUTINE(ComparisonRoutine, "norepeat_p4", compare_long_codeword_norepeat_p4)
#endif
