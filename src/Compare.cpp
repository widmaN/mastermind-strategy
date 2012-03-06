///////////////////////////////////////////////////////////////////////////
// Codeword comparison routines.

#include "util/simd.hpp"
#include "util/intrinsic.hpp"
#include "util/call_counter.hpp"
#include "Algorithm.hpp"

REGISTER_CALL_COUNTER(Comparison)

namespace Mastermind {

///////////////////////////////////////////////////////////////////////////
// Generic comparison routine for repeatable codewords.

// Precompute a conversion table that converts (nA<<4|nAB) -> feedback.
// Both nA and nAB must be >= 0 and <= 15.
struct generic_feedback_mapping_t
{
	Feedback table[0x100];

	generic_feedback_mapping_t()
	{
		for (int i = 0; i < 0x100; i++)
		{
			int nA = i >> 4;
			int nAB = i & 0xF;
			table[i] = Feedback(nA, nAB - nA);
		}
	}
};

static const generic_feedback_mapping_t generic_feedback_mapping;


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
// Hence, the best we could do is probably to compile under 32-bit, and 
// access the table directly.
struct norepeat_feedback_mapping_t
{
	Feedback table[0x10000];

	norepeat_feedback_mapping_t()
	{
		for (int i = 0; i < 0x10000; i++)
		{
			int nA = util::intrinsic::pop_count((unsigned short)(i >> MM_MAX_COLORS));
			int nAB = util::intrinsic::pop_count((unsigned short)(i & ((1<<MM_MAX_COLORS)-1)));
#if 0
			table[i] = nAB*(nAB+1)/2+nA;
#else
			table[i] = Feedback(nA, nAB - nA);
#endif
		}
	}
};

static const norepeat_feedback_mapping_t norepeat_feedback_mapping;


class GenericComparer
{
	typedef util::simd::simd_t<uint8_t,16> simd_t;

	simd_t secret;

public:
	GenericComparer(const Codeword &_secret) : secret(_secret.value())
	{
		// Change 0xff in secret to 0x0f.
		secret &= (uint8_t)0x0f;
	}

	/**
	 * In VC++, it's extremely subtle how the C++ code will compile into 
	 * assembly. Effectively equivalent code may lead to significantly 
	 * different performance. Usually the performance difference comes
	 * from redundant memory reads/writes. Hence it's always a good idea
	 * to examine the ASM after compiling the code.
	 *
	 * The following C++ code is hand-crafted so that the generated ASM
	 * by VC++ is almost optimal. The generated instructions for a loop
	 * that compares codewords and increment the frequencies are:
	 *
	 *   movdqa      xmm0,xmmword ptr [r10]  ; xmm0 := *guesses
	 *   movdqa      xmm1,xmm2               ; xmm1 := xmm2
	 *   lea         r10,[r10+10h]           ; ++guesses
	 *   pcmpeqb     xmm1,xmm0 
	 *   pminub      xmm0,xmm2  
	 *   pand        xmm0,xmm4  
	 *   pand        xmm1,xmm3  
	 *   psadbw      xmm0,xmm5  
	 *   psadbw      xmm1,xmm5  
	 *   pextrw      eax,xmm0,0  
	 *   pextrw      ecx,xmm0,4  
	 *   add         ecx,eax  
	 *   pextrw      eax,xmm1,4  
	 *   shl         eax,4  
	 *   movsxd      rdx,ecx  
	 *   cdqe  
	 *   or          rdx,rax  
	 *   movsx       rax,byte ptr [rdx+r11]  ; rax := lookup_table[mask]
	 *   inc         dword ptr [r9+rax*4]    ; ++freq[Feedback]
	 *   dec         r8                      ; --count
	 *   jne         ...                     ; continue loop
	 */
	Feedback operator () (const Codeword &_guess) const
	{
		// Create mask for use later.
		const simd_t mask_pegs = util::simd::fill_left<MM_MAX_PEGS>((uint8_t)0x01);
		const simd_t mask_colors = util::simd::fill_right<MM_MAX_COLORS>((uint8_t)0xff);

		const simd_t guess(_guess.value());

		// Compute nA.
		// It turns out that there's a significant (20%) performance
		// hit if <code>nA = sum_high(...)</code> is changed to
		// <code>nA = sum(...)</code>. In the latter case, VC++
		// VC++ generates an extra (redundant) memory read.
		// The reason is unclear. (Obviously there are still free
		// XMM registers.)
#if MM_MAX_PEGS <= 8
		int nA = sum_high((guess == secret) & mask_pegs);
#else
		int nA = sum((guess == secret) & mask_pegs);
#endif

		// Compute nAB.
#if MM_MAX_COLORS <= 8
		int nAB = sum_low(min(guess, secret) & mask_colors);
#else
		int nAB = sum(min(guess, secret) & mask_colors);
#endif

		Feedback nAnB = generic_feedback_mapping.table[(nA<<4)|nAB];
		return nAnB;
	}
};

class NoRepeatComparer
{
	typedef util::simd::simd_t<char,16> simd_t;
	
	simd_t secret;

public:

	NoRepeatComparer(const Codeword &_secret)
		: secret(_secret.value()) 
	{
		// Change 0xFF in secret to 0x0F
		secret &= (char)0x0f;

		// Set zero counters in secret to 0xFF, so that if a counter in the
		// guess and secret are both zero, they won't compare equal.
		secret |= util::simd::keep_right<MM_MAX_COLORS>(secret == simd_t::zero());
	}

	/**
	 * The following code translates into the following ASM:
	 *
	 *   movdqa      xmm0,xmmword ptr [rbx]  ; xmm0 := *guesses
	 *   movdqa      xmm1,xmm2               ; xmm1 := this->secret
	 *   lea         rbx,[rbx+10h]           ; ++guesses
	 *   pcmpeqb     xmm1,xmm0  
	 *   pmovmskb    eax,xmm1  
	 *   movsxd      rcx,eax                 ; *
	 *   movsx       rax,byte ptr [rcx+rdx]  ; rax := lookup_table[mask]
	 *   inc         dword ptr [rbp+rax*4]   ; ++freq[Feedback]
	 *   dec         rsi                     ; --count
	 *   jne         ...                     ; continue loop
	 *
	 * Note that we could save the instruction (*) if the intrinsic
	 * function for @c pmovmskb has return type int64 instead of int.
	 */
	Feedback operator () (const Codeword &guess) const 
	{
		return norepeat_feedback_mapping.table
			[ util::simd::byte_mask(simd_t(guess.value()) == secret) ];
	}
};

class FeedbackUpdater
{
	Feedback * feedbacks;

public:
	
	FeedbackUpdater(Feedback *fbs) : feedbacks(fbs) { }

	void operator () (const Feedback &fb) 
	{
		*(feedbacks++) = fb;
	}
};

class FrequencyUpdater
{
	unsigned int * freq;

public:
	
	FrequencyUpdater(unsigned int *_freq, size_t size) : freq(_freq) 
	{
		memset(freq, 0, sizeof(unsigned int)*size);
	}

	void operator () (const Feedback &fb) 
	{
		++freq[fb.value()];
	}
};

template <class Comparer, class Updater>
void compare_codewords(
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Updater _update)
{
	Comparer compare(secret);
	Updater update(_update); // this redundant copy is to make VC++ happy that
	                         // the value of update will not change in the loop
	size_t count = last - first;
	const Codeword *guesses = first;
	for (; count > 0; --count)
	{
		Feedback nAnB = compare(*guesses++);
		update(nAnB);
	}
}

// SSE2-based implementation for comparing generic codewords.
// Repeated colors are allowed.
#if 0
static void compare_long_codeword_generic(
	const Rules & /* rules */,
	const Codeword &_secret,
	const Codeword *_first,
	const Codeword *_last,
	Feedback result[])
{
	UPDATE_CALL_COUNTER(Comparison, (unsigned int)(_last - _first));
#if 0
	if (_last - _first == 1)
	{
		int kk = 1;
	}
#endif

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

		Feedback nAnB = generic_feedback_mapping.table[(nA<<4)|nAB];
		*(result++) = nAnB;
	}
}
#else
static void compare_long_codeword_generic(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[])
{
	FeedbackUpdater update(result);
	compare_codewords<GenericComparer>(secret, first, last, update);
}
#endif

// Comparison routine for codewords without repetition.
static void compare_long_codeword_norepeat(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[])
{
	FeedbackUpdater update(result);
	compare_codewords<NoRepeatComparer>(secret, first, last, update);
}

void compare_frequencies_generic(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	unsigned int freq[],
	size_t size)
{
	FrequencyUpdater update(freq, size);
	compare_codewords<GenericComparer>(secret, first, last, update);
}

void compare_frequencies_norepeat(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	unsigned int freq[],
	size_t size)
{
	FrequencyUpdater update(freq, size);
	compare_codewords<NoRepeatComparer>(secret, first, last, update);
}

///////////////////////////////////////////////////////////////////////////
// Routine registration.

REGISTER_ROUTINE(ComparisonRoutine, "generic", compare_long_codeword_generic)
REGISTER_ROUTINE(ComparisonRoutine, "norepeat", compare_long_codeword_norepeat)

} // namespace Mastermind
