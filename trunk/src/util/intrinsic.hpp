/**
 * @defgroup Intrinsic Intrinsic Functions
 * @ingroup util
 */

#ifndef UTILITIES_INTRINSIC_HPP
#define UTILITIES_INTRINSIC_HPP

#ifdef _WIN32
#include <intrin.h>
#else
//#include <x86intrin.h>
#endif

/// @ingroup Intrinsic
// @{

namespace util { namespace intrinsic {

/// Returns the number of bits set in an integer.
template <class T>
inline int pop_count(T value)
{
	int n = 0;
	for (; value; value &= (value - T(1)))
		++n;
	return n;
}

/// @cond DETAILS
#if defined(_WIN32)
#if HAVE_POPCOUNT
inline int pop_count(unsigned char x) { return _popcnt(x); }
inline int pop_count(unsigned short x) { return __popcnt16(x); }
inline int pop_count(unsigned int x) { return __popcnt(x); }
inline int pop_count(unsigned long x) { return __popcnt(x); }
#if defined(_WIN64)
inline int pop_count(unsigned long long x) { return __popcnt64(x); }
#else // _WIN64
inline int pop_count(unsigned long long x) 
{
	return __popcnt((unsigned long)(x >> 32)) + _popcnt((unsigned long)x);
}
#endif // defined(_WIN64)
#endif // HAVE_POPCOUNT
#else  // defined(_WIN32)
inline int pop_count(unsigned char x) { return __builtin_popcount(x); }
inline int pop_count(unsigned short x) { return __builtin_popcount(x); }
inline int pop_count(unsigned int x) { return __builtin_popcount(x); }
inline int pop_count(unsigned long x) { return __builtin_popcountl(x); }
inline int pop_count(unsigned long long x) { return __builtin_popcountll(x); }
#endif // defined(_WIN32)
/// @endcond

/// Returns the (zero-based) position of the least significant bit set
/// in an integer. If the integer is zero, the return value is undefined.
template <class T>
inline int bit_scan_forward(T x)
{
	if (x == T(0))
		return -1;

	int i = 0;
	while (!(x & T(1)))
	{
		x >>= 1;
		++i;
	}
	return i;
}

/// @cond DETAILS
#if defined(_WIN32)
inline int bit_scan_forward(unsigned long x)
{
	unsigned long pos = 0;
	_BitScanForward(&pos, x);
	return pos;
}
inline int bit_scan_forward(unsigned int x) { return bit_scan_forward((unsigned long)x); }
inline int bit_scan_forward(unsigned short x) { return bit_scan_forward((unsigned long)x); }
inline int bit_scan_forward(unsigned char x) { return bit_scan_forward((unsigned long)x); }
#if defined(_WIN64)
inline int bit_scan_forward(unsigned long long x)
{
	unsigned long pos = 0;
	_BitScanForward64(&pos, x);
	return pos;
}
#else  // defined(_WIN64)
inline int bit_scan_forward(unsigned long long x)
{
	if ((unsigned long)x == 0)
		return bit_scan_forward((unsigned long)(x >> 32)) + 32;
	else
		return bit_scan_forward((unsigned long)x);
}
#endif // defined(_WIN64)
#else  // defined(_WIN32)
inline int bit_scan_forward(unsigned char x) { return __builtin_ctz(x); }
inline int bit_scan_forward(unsigned short x) { return __builtin_ctz(x); }
inline int bit_scan_forward(unsigned int x) { return __builtin_ctz(x); }
inline int bit_scan_forward(unsigned long x) { return __builtin_ctzl(x); }
inline int bit_scan_forward(unsigned long long x) { return __builtin_ctzll(x); }
#endif // defined(_WIN32)
/// @endcond

/// Returns the (zero-based) position of the most significant bit set
/// in an integer. If the integer is zero, the return value is undefined.
template <class T>
inline int bit_scan_reverse(T x)
{
	if (x == T(0))
		return -1;

	const T bit = T(1) << (sizeof(T)*8-1);

	int i = sizeof(T)*8-1;
	while (!(x & bit))
	{
		x <<= 1;
		--i;
	}
	return i;
}

/// @cond DETAILS
#if defined(_WIN32)
inline int bit_scan_reverse(unsigned long x)
{
	unsigned long pos = 0;
	_BitScanReverse(&pos, x);
	return pos;
}
inline int bit_scan_reverse(unsigned int x) { return bit_scan_reverse((unsigned long)x); }
inline int bit_scan_reverse(unsigned short x) { return bit_scan_reverse((unsigned long)x); }
inline int bit_scan_reverse(unsigned char x) { return bit_scan_reverse((unsigned long)x); }
#if defined(_WIN64)
inline int bit_scan_reverse(unsigned long long x)
{
	unsigned long pos = 0;
	_BitScanReverse64(&pos, x);
	return pos;
}
#else  // defined(_WIN64)
inline int bit_scan_reverse(unsigned long long x)
{
	if ((unsigned long)(x >> 32) == 0)
		return bit_scan_reverse((unsigned long)x);
	else
		return bit_scan_reverse((unsigned long)(x >> 32)) + 32;
}
#endif // defined(_WIN64)
#else  // defined(_WIN32)
inline int bit_scan_reverse(unsigned long long x) { return sizeof(long long) - __builtin_clzll(x); }
inline int bit_scan_reverse(unsigned long x) { return sizeof(long) - __builtin_clzl(x); }
inline int bit_scan_reverse(unsigned int x) { return sizeof(int) - __builtin_clz(x); }
inline int bit_scan_reverse(unsigned short x) { return bit_scan_reverse((unsigned int)x); }
inline int bit_scan_reverse(unsigned char x) { return bit_scan_reverse((unsigned int)x); }
#endif // defined(_WIN32)
/// @endcond

} } // namespace util::intrinsic

#endif // UTILITIES_INTRINSIC_HPP
