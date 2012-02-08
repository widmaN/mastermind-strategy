#ifndef MASTERMIND_UTIL_SIMD_HPP
#define MASTERMIND_UTIL_SIMD_HPP

#include <cstdint>
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
struct simd_t 
{
	static_assert(sizeof(T)*N == sizeof(__m128i), 
		"Total size of SIMD vector must be 128.");

	__m128i value;

	simd_t() : value(_mm_setzero_si128()) { }
	simd_t(__m128i v) : value(v) { }
	simd_t(char b) : value(_mm_set1_epi8(b)) { }
	operator __m128i() const { return value; }
	static simd_t zero() { return _mm_setzero_si128(); }
};

/// Element-wise bit-AND.
template <class T, int N>
simd_t<T,N> operator & (const simd_t<T,N> &a, const simd_t<T,N> &b)
{
	return _mm_and_si128(a, b);
}

template <class T, int N>
simd_t<T,N>& operator &= (simd_t<T,N> &a, const simd_t<T,N> &b)
{
	return (a = _mm_and_si128(a, b));
}

template <class T, int N>
simd_t<T,N>& operator &= (simd_t<T,N> &a, T b)
{
	return (a &= simd_t<T,N>(b));
}

/// Element-wise bit-OR.
template <class T, int N>
simd_t<T,N>& operator |= (simd_t<T,N> &a, const simd_t<T,N> &b)
{
	return (a = _mm_or_si128(a, b));
}

/// Element-wise equality testing. 
/// Equal elements are set to -1.
/// Unequal elements are set to 0.
inline simd_t<char,16> 
operator == (const simd_t<char,16> &a, const simd_t<char,16> &b)
{
	return _mm_cmpeq_epi8(a, b);
}

/// Element-wise equality testing. 
/// Equal elements are set to 0xFF.
/// Unequal elements are set to 0.
inline simd_t<uint8_t,16> 
operator == (const simd_t<uint8_t,16> &a, const simd_t<uint8_t,16> &b)
{
	return _mm_cmpeq_epi8(a, b);
}

/// Creates a 16-bit mask from the most significant bits of the 16 bytes,
/// and zero extends the upper bits in the result.
inline int byte_mask(const simd_t<char,16> &a)
{
	return _mm_movemask_epi8(a);
}

/// Creates a 16-bit mask from the most significant bits of the 16 bytes,
/// and zero extends the upper bits in the result.
inline int byte_mask(const simd_t<uint8_t,16> &a)
{
	return _mm_movemask_epi8(a);
}

/// Shift elements (not bits) left.
template <int Count>
simd_t<uint8_t,16> shift_elements_left(const simd_t<uint8_t,16> &a)
{
	return _mm_slli_si128(a, Count);
}

/// Shift elements (not bits) right.
template <int Count>
simd_t<uint8_t,16> shift_elements_right(const simd_t<uint8_t,16> &a)
{
	return _mm_srli_si128(a, Count);
}

/// Fill the left @c Count bytes with <code>b</code>.
template <int Count>
simd_t<uint8_t,16> fill_left(uint8_t b)
{
	return _mm_slli_si128(_mm_set1_epi8(b), 16-Count);
}

/// Fill the right @c Count bytes with <code>b</code>.
template <int Count>
simd_t<uint8_t,16> fill_right(uint8_t b)
{
	return _mm_srli_si128(_mm_set1_epi8(b), 16-Count);
}

/// Keeps @c Count bytes on the right, and sets the remaining bytes on 
/// the left to zero.
template <int Count>
simd_t<char,16> keep_right(const simd_t<char,16> &a)
{
	return _mm_srli_si128(_mm_slli_si128(a,16-Count),16-Count);
}

/// Extracts the selected 16-bit word, and zero extends the result.
template <int Index>
int extract(const simd_t<uint16_t,8> &a)
{
	return _mm_extract_epi16(a, Index);
}

/// Computes the absolute difference of the upper 8 unsigned bytes
/// and stores the result in the upper quadword; computes the same
/// for the lower 8 unsigned bytes and stores the result in the 
/// lower quadword.
inline simd_t<uint16_t,8> 
sad(const simd_t<uint8_t,16> &a, const simd_t<uint8_t,16> &b)
{
	return _mm_sad_epu8(a, b);
}

/// Element-wise minimum.
inline simd_t<uint8_t,16>
min(const simd_t<uint8_t,16> &a, const simd_t<uint8_t,16> &b)
{
	return _mm_min_epu8(a, b);
}

/// Returns the sum of bytes.
inline int sum(const simd_t<uint8_t,16> &a)
{
	const simd_t<uint16_t,8> &t = sad(a, simd_t<uint8_t,16>::zero());
	return extract<0>(t) + extract<4>(t);
}

/// Returns the sum of the upper half bytes.
inline int sum_high(const simd_t<uint8_t,16> &a)
{
	const simd_t<uint16_t,8> &t = sad(a, simd_t<uint8_t,16>::zero());
	return extract<4>(t);
}

/// Returns the sum of the lower half bytes.
inline int sum_low(const simd_t<uint8_t,16> &a)
{
	const simd_t<uint16_t,8> &t = sad(a, simd_t<uint8_t,16>::zero());
	return extract<0>(t);
}


} } // namespace util::simd

#endif // MASTERMIND_UTIL_SIMD_HPP
