#ifndef MASTERMIND_UTIL_SIMD_HPP
#define MASTERMIND_UTIL_SIMD_HPP

#include <emmintrin.h>

namespace util { namespace simd {

/**
 * Represents a fixed-size vector of elements wrapped in an SIMD data type.
 *
 * The type of the element must be one of <code>char, short, int, long, 
 * single, double</code> and their unsigned counterparts.
 *
 * The total size of the vector must be 64 (for MMX), 128 (for XMM), or
 * 256 (for AVX).
 */
template <class T, int N>
struct simd_t { };

/// Specialization for 16 bytes wrapped in a 128-bit XMM type.
template <>
struct simd_t<char, 16>
{
	__m128i value;

	typedef simd_t<char, 16> self_type;
	typedef char value_type;

	simd_t() : value(_mm_setzero_si128()) { }
	simd_t(__m128i v) : value(v) { }
	simd_t(char b) : value(_mm_set1_epi8(b)) { }
	operator __m128i() const { return value; }
	static simd_t zero() { return _mm_setzero_si128(); }
};

/// Element-wise bit-AND.
simd_t<char,16>& operator &= (simd_t<char,16> &a, const simd_t<char,16> &b)
{
	return (a = _mm_and_si128(a, b));
}

/// Element-wise bit-OR.
simd_t<char,16>& operator |= (simd_t<char,16> &a, const simd_t<char,16> &b)
{
	return (a = _mm_or_si128(a, b));
}

/// Element-wise equality testing. 
/// Equal elements are set to -1.
/// Unequal elements are set to 0.
simd_t<char,16> operator == (const simd_t<char,16> &a, const simd_t<char,16> &b)
{
	return _mm_cmpeq_epi8(a, b);
}

/// Creates a 16-bit mask from the most significant bits of the 16 bytes,
/// and zero extends the upper bits in the result.
int byte_mask(const simd_t<char,16> &a)
{
	return _mm_movemask_epi8(a);
}

/// Keeps @c Count bytes on the right, and sets the remaining bytes on 
/// the left to zero.
template <int Count>
simd_t<char,16> keep_right(const simd_t<char,16> &a)
{
	return _mm_srli_si128(_mm_slli_si128(a,16-Count),16-Count);
}

} } // namespace util::simd

#endif // MASTERMIND_UTIL_SIMD_HPP
