/**
 * @defgroup SIMD SIMD Operations
 * @ingroup util
 */

#ifndef UTILITIES_SIMD_HPP
#define UTILITIES_SIMD_HPP

#include <cassert>
#include <cstdint>
#include <emmintrin.h>

namespace util { namespace simd {

/**
 * Represents a fixed-size vector of elements wrapped in an SIMD data type.
 *
 * The type of the element must be one of <code>char, short, int, long, 
 * single, double</code> or their unsigned counterparts.
 * The total size of the vector must be 64 (for MMX), 128 (for XMM), or
 * 256 (for AVX).
 *
 * @ingroup SIMD
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
/// @ingroup SIMD
template <class T, int N>
inline simd_t<T,N> 
operator & (const simd_t<T,N> &a, const simd_t<T,N> &b)
{
	return _mm_and_si128(a, b);
}

template <class T, int N>
inline simd_t<T,N>& 
operator &= (simd_t<T,N> &a, const simd_t<T,N> &b)
{
	return (a = _mm_and_si128(a, b));
}

template <class T, int N>
inline simd_t<T,N>& 
operator &= (simd_t<T,N> &a, T b)
{
	return (a &= simd_t<T,N>(b));
}

/// Element-wise bit-OR.
/// @ingroup SIMD
template <class T, int N>
inline simd_t<T,N>& 
operator |= (simd_t<T,N> &a, const simd_t<T,N> &b)
{
	return (a = _mm_or_si128(a, b));
}

/// Element-wise equality testing of two @c char vectors.
/// @returns A result vector where equal elements are set to @c -1
///      and unequal elements are set to @c 0.
/// @ingroup SIMD
inline simd_t<char,16> 
operator == (const simd_t<char,16> &a, const simd_t<char,16> &b)
{
	return _mm_cmpeq_epi8(a, b);
}

/// Element-wise equality testing of two <code>unsigned char</code>
/// vectors.
/// @returns A result vector where equal elements are set to @c 0xFF
///      and unequal elements are set to @c 0.
/// @ingroup SIMD
inline simd_t<uint8_t,16> 
operator == (const simd_t<uint8_t,16> &a, const simd_t<uint8_t,16> &b)
{
	return _mm_cmpeq_epi8(a, b);
}

/// Creates a 16-bit mask from the most significant bits of the 16 bytes,
/// and zero extends the upper bits in the result.
/// @ingroup SIMD
inline int byte_mask(const simd_t<char,16> &a)
{
	return _mm_movemask_epi8(a);
}

/// Creates a 16-bit mask from the most significant bits of the 16 bytes,
/// and zero extends the upper bits in the result.
/// @ingroup SIMD
inline int byte_mask(const simd_t<uint8_t,16> &a)
{
	return _mm_movemask_epi8(a);
}

/// Shift elements (not bits) left.
/// @ingroup SIMD
template <int Count>
inline simd_t<uint8_t,16> 
shift_elements_left(const simd_t<uint8_t,16> &a)
{
	return _mm_slli_si128(a, Count);
}

/// Shift elements (not bits) right.
/// @ingroup SIMD
template <int Count>
inline simd_t<uint8_t,16> 
shift_elements_right(const simd_t<uint8_t,16> &a)
{
	return _mm_srli_si128(a, Count);
}

/// Fills the left @c Count bytes with <code>b</code>.
/// @ingroup SIMD
template <int Count>
inline simd_t<uint8_t,16> fill_left(uint8_t b)
{
	return _mm_slli_si128(_mm_set1_epi8(b), 16-Count);
}

/// Fills the right @c Count bytes with <code>b</code>.
/// @ingroup SIMD
template <int Count>
inline simd_t<uint8_t,16> fill_right(uint8_t b)
{
	return _mm_srli_si128(_mm_set1_epi8(b), 16-Count);
}

/// Keeps @c Count bytes on the right, and sets the remaining bytes on 
/// the left to zero.
/// @ingroup SIMD
template <int Count>
inline simd_t<char,16> keep_right(const simd_t<char,16> &a)
{
	return _mm_srli_si128(_mm_slli_si128(a,16-Count),16-Count);
}

/// Keeps @c Count bytes on the right, and sets the remaining bytes on 
/// the left to zero.
/// @ingroup SIMD
template <int Count>
simd_t<uint8_t,16> keep_right(const simd_t<uint8_t,16> &a)
{
	return _mm_srli_si128(_mm_slli_si128(a,16-Count),16-Count);
}

/// Keeps @c Count bytes on the left, and sets the remaining bytes on 
/// the right to zero.
/// @ingroup SIMD
template <int Count>
simd_t<uint8_t,16> keep_left(const simd_t<uint8_t,16> &a)
{
	return _mm_slli_si128(_mm_srli_si128(a,16-Count),16-Count);
}

/// Extracts the selected 16-bit word, and zero extends the result.
/// @ingroup SIMD
template <int Index>
inline int extract(const simd_t<uint16_t,8> &a)
{
	return _mm_extract_epi16(a, Index);
}

/// Computes the absolute difference of the upper 8 unsigned bytes
/// and stores the result in the upper quadword; computes the same
/// for the lower 8 unsigned bytes and stores the result in the 
/// lower quadword.
/// @ingroup SIMD
inline simd_t<uint16_t,8> 
sad(const simd_t<uint8_t,16> &a, const simd_t<uint8_t,16> &b)
{
	return _mm_sad_epu8(a, b);
}

/// Element-wise minimum.
/// @ingroup SIMD
inline simd_t<uint8_t,16>
min(const simd_t<uint8_t,16> &a, const simd_t<uint8_t,16> &b)
{
	return _mm_min_epu8(a, b);
}

/// Returns the sum of elements whose indices are in the range [Begin,End).
/// @ingroup SIMD
template <int Begin, int End> 
inline int sum(const simd_t<uint8_t,16> &a)
{
	static_assert(Begin >= 0 && Begin <= End && End <= 16, "Invalid [Begin,End) range");
	if (Begin == End)
		return 0;

	if (Begin % 8 == 0 && End % 8 == 0)
	{
		const simd_t<uint16_t,8> &t = sad(a, simd_t<uint8_t,16>::zero());
		if (Begin == 0 && End == 8)
			return extract<0>(t);
		else if (Begin == 8 && End == 16)
			return extract<4>(t);
		else // if (Begin == 0 && End == 16)
			return extract<0>(t) + extract<4>(t);
	}
	else
	{
		assert(0); // Not implemented
		return 0;
	}
}

#if 0
/// Returns the sum of elements.
/// @ingroup SIMD
inline unsigned int sum(const simd_t<uint8_t,16> &a)
{
	return sum<0,16>(a);
}
#endif

} } // namespace util::simd


#if 0
#include <tmmintrin.h>

namespace util { namespace simd {
	
#define SIMD_REQUIRE(version)

SIMD_REQUIRE("SSSE3")
inline simd_t<uint8_t,16> 
shuffle(const simd_t<uint8_t,16> &a, const simd_t<int8_t,16> &perm)
{
	return _mm_shuffle_epi8(a, perm);
}

} } // namespace util::simd
#endif

#endif // UTILITIES_SIMD_HPP
