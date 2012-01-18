/// \file MMConfig.h
/// Stores compile-time configuration data.

#pragma once

#include <emmintrin.h>

/// The maximum number of pegs supported by the program.
/// For performance reasons, certain data structures or algorithms 
/// pose a limit on the maximum number of pegs allowed in a codeword. 
/// If this constant is defined higher than the limit, the program 
/// will produce a compile-time error.
#define MM_MAX_PEGS 6

/// The maximum number of colors supported by the program.
/// For performance reasons, certain data structures or algorithms 
/// pose a limit on the maximum number of colors allowed in a codeword. 
/// If this constant is defined higher than the limit, the program 
/// will produce a compile-time error.
#define MM_MAX_COLORS 10

#if (MM_MAX_PEGS + MM_MAX_COLORS) != 16
# error Invalid combination of MM_MAX_PEGS and MM_MAX_COLORS; they must add up to 16.
#endif

//alignof
//__declspec(align(16))
union codeword_t {
	__m128i value;
	struct {
		unsigned char counter[MM_MAX_COLORS];
		unsigned char digit[MM_MAX_PEGS];
	};
};

typedef union codeword_t codeword_t;

// Whether to store feedback in compact format
#define MM_FEEDBACK_COMPACT 1

#if MM_FEEDBACK_COMPACT
#else
#define MM_FEEDBACK_BITS 6
#define MM_FEEDBACK_COUNT (1 << MM_FEEDBACK_BITS)
#define MM_FEEDBACK_ASHIFT (MM_FEEDBACK_BITS / 2)
#define MM_FEEDBACK_BMASK ((1<<MM_FEEDBACK_ASHIFT)-1)
#endif

extern const unsigned char feedback_count[MM_MAX_PEGS+1];
extern const unsigned char feedback_map[256];
extern const unsigned char feedback_revmap[256];

// Whether to compile call counter
#define ENABLE_CALL_COUNTER 0
