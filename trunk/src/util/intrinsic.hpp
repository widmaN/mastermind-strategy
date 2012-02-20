/**
 * @defgroup Intrinsic Intrinsic Functions
 * @ingroup util
 */

#ifndef UTILITIES_INTRINSIC_HPP
#define UTILITIES_INTRINSIC_HPP

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace util { namespace intrinsic {

/// Rotates the bits in @c value left by @c shift bits.
/// @ingroup Intrinsic
inline unsigned int rotate_left(unsigned int value, int shift)
{
	return _rotl(value, 4);
}

/// Returns the position of the least significant one bit in @c value.
/// @param value An integer.
/// @returns The (zero-based) position of the least significant one bit
///      in @c value, or <code>-1</code> if @c value is zero.
/// @ingroup Intrinsic
inline int bit_scan_forward(unsigned long value)
{
	if (value == 0)
		return -1;

	unsigned long pos = 0;
#ifdef _WIN32
	_BitScanForward(&pos, value);
#else
	pos = __builtin_ctz(value);
#endif
	return pos;
}

/// Returns the position of the most significant one bit in @c value.
/// @param value An integer.
/// @returns The (zero-based) position of the most significant one bit
///      in @c value, or <code>-1</code> if @c value is zero.
/// @ingroup Intrinsic
inline int bit_scan_reverse(unsigned long value)
{
	if (value == 0)
		return -1;

	unsigned long pos = 0;
#ifdef _WIN32
	_BitScanReverse(&pos, value);
#else
	pos = 31 - __builtin_clz(value);
#endif
	return pos;
}

/// Counts the number of one bits in a 16-bit integer.
/// @param value An integer.
/// @returns The number of one bits in @c value.
/// @ingroup Intrinsic
inline int pop_count(unsigned short value)
{
#ifdef _WIN32
#ifndef WITHOUT_POPCOUNT
	return __popcnt16(value);
#else /* WITHOUT_POPCOUNT */
	int n = 0;
	for (; value; value >>= 1)
	{
		n += (value & 1);
	}
	return n;
#endif
#else /* _WIN32 */
	return  __builtin_popcount(value);
#endif
}

} } // namespace util::intrinsic

#endif // UTILITIES_INTRINSIC_HPP
