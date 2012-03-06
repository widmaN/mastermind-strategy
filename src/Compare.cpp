///////////////////////////////////////////////////////////////////////////
// Codeword comparison routines.

#include <cassert>
#include <utility>
#include "util/simd.hpp"
#include "util/intrinsic.hpp"
#include "util/call_counter.hpp"
#include "Algorithm.hpp"

REGISTER_CALL_COUNTER(Comparison)

namespace Mastermind {

/// Codeword comparer for generic codewords (with or without repetition).
class GenericComparer
{
	// Lookup table that converts (nA<<4|nAB) -> feedback.
	// Both nA and nAB must be >= 0 and <= 15.
	struct lookup_table_t
	{
		Feedback table[0x100];

		lookup_table_t()
		{
			for (int i = 0; i < 0x100; i++)
			{
				int nA = i >> 4;
				int nAB = i & 0xF;
				table[i] = Feedback(nA, nAB - nA);
			}
		}
	};

	static const lookup_table_t lookup;

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

		Feedback nAnB = lookup.table[(nA<<4)|nAB];
		return nAnB;
	}
};

const GenericComparer::lookup_table_t GenericComparer::lookup;

/// Specialized codeword comparer for codewords without repetition.
class NoRepeatComparer
{
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
	struct lookup_table_t
	{
		Feedback table[0x10000];

		lookup_table_t()
		{
			for (int i = 0; i < 0x10000; i++)
			{
				int nA = util::intrinsic::pop_count((unsigned short)(i >> MM_MAX_COLORS));
				int nAB = util::intrinsic::pop_count((unsigned short)(i & ((1<<MM_MAX_COLORS)-1)));
				table[i] = Feedback(nA, nAB - nA);
			}
		}
	};

	static const lookup_table_t lookup;

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
		return lookup.table
			[ util::simd::byte_mask(simd_t(guess.value()) == secret) ];
	}
};

const NoRepeatComparer::lookup_table_t NoRepeatComparer::lookup;

/// Function object that appends a feedback to a feedback list.
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

/// Function object that increments the frequency statistic of a feedback.
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

/// Function object that invokes two functions.
template <class T1, class T2>
struct CompositeUpdater
{
	T1 u1;
	T2 u2;

	CompositeUpdater(T1 updater1, T2 updater2) 
		: u1(updater1), u2(updater2) { }

	void operator () (const Feedback &fb)
	{
		u1(fb);
		u2(fb);
	}
};

#if 0
/// Specialized function object that keeps track of frequency statistics 
/// for no more than 16 distinct feedbacks and no more than 15 frequencies.
class TinyFrequencyUpdater
{
	unsigned int * _freq;
	size_t _size;
	unsigned long long _counter;

	TinyFrequencyUpdater& operator = (const TinyFrequencyUpdater &) { }

public:
	
	TinyFrequencyUpdater(unsigned int *freq, size_t size) 
		: _freq(freq), _size(size), _counter(0)
	{
		assert(size <= 16);
	}

	TinyFrequencyUpdater(TinyFrequencyUpdater &u)
		: _freq(u._freq), _size(u._size), _counter(u._counter)
	{
		u._freq = 0;
	}

	~TinyFrequencyUpdater()
	{
		if (_freq)
		{
			for (size_t i = 0; i < _size; ++i)
			{
				_freq[i] = (unsigned int)(_counter >> (i * 4)) & 0x0FU;
			}
		}
	}

	void operator () (const Feedback &fb) 
	{
		assert(fb.value() < 16);
		_counter += (1ULL << (fb.value() * 4));
	}
};
#endif

/// Compares a secret to a list of codewords using @c Comparer, and 
/// processes each feedback using @c Updater.
template <class Comparer, class Updater>
static void compare_codewords(
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Updater _update)
{
	// The following redundant copy is to make VC++ happy treat the value
	// of @c _update as constant in the loop.
	Updater update(_update);

	Comparer compare(secret);
	size_t count = last - first;
	const Codeword *guesses = first;
	for (; count > 0; --count)
	{
		Feedback nAnB = compare(*guesses++);
		update(nAnB);
	}
}

template <class Comparer>
static void compare_codewords_impl(
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Feedback *result,
	unsigned int *freq,
	size_t size)
{
	if (result && freq)
	{
		CompositeUpdater<FeedbackUpdater,FrequencyUpdater> 
			update(FeedbackUpdater(result), FrequencyUpdater(freq, size));
		compare_codewords<Comparer>(secret, first, last, update);
	}
	else if (freq)
	{
		FrequencyUpdater update(freq, size);
		compare_codewords<Comparer>(secret, first, last, update);
	}
	else if (result)
	{
		FeedbackUpdater update(result);
		compare_codewords<Comparer>(secret, first, last, update);
	}
}

// SSE2-based implementation for comparing generic codewords.
// Repeated colors are allowed.
static void compare_long_codeword_generic(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[])
{
#if 0
	FeedbackUpdater update(result);
	compare_codewords<GenericComparer>(secret, first, last, update);
#else
	compare_codewords_impl<GenericComparer>(secret, first, last, result, 0, 0);
#endif
}

// Comparison routine for codewords without repetition.
static void compare_long_codeword_norepeat(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	Feedback result[])
{
#if 0
	FeedbackUpdater update(result);
	compare_codewords<NoRepeatComparer>(secret, first, last, update);
#else
	compare_codewords_impl<NoRepeatComparer>(secret, first, last, result, 0, 0);
#endif
}

void compare_frequencies_generic(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	unsigned int freq[],
	size_t size)
{
#if 0
	FrequencyUpdater update(freq, size);
	compare_codewords<GenericComparer>(secret, first, last, update);
#else
	compare_codewords_impl<GenericComparer>(secret, first, last, 0, freq, size);
#endif
}

void compare_frequencies_norepeat(
	const Rules & /* rules */,
	const Codeword &secret,
	const Codeword *first,
	const Codeword *last,
	unsigned int freq[],
	size_t size)
{
#if 0
	size_t count = last - first;
	if (count <= 15 && size <= 16)
	{
		TinyFrequencyUpdater update(freq, size);
		compare_codewords<NoRepeatComparer>(secret, first, last, update);
	}
	else
#endif
	{
#if 0
		FrequencyUpdater update(freq, size);
		compare_codewords<NoRepeatComparer>(secret, first, last, update);
#else
		compare_codewords_impl<NoRepeatComparer>(secret, first, last, 0, freq, size);
#endif
	}
}

///////////////////////////////////////////////////////////////////////////
// Routine registration.

REGISTER_ROUTINE(ComparisonRoutine, "generic", compare_long_codeword_generic)
REGISTER_ROUTINE(ComparisonRoutine, "norepeat", compare_long_codeword_norepeat)

} // namespace Mastermind
