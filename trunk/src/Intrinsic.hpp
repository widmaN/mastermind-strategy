#ifndef UTILITIES_INTRINSIC_H
#define UTILITIES_INTRINSIC_H

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace Utilities { 
namespace Intrinsic {

inline unsigned int rotateLeft(unsigned int value, int shift)
{
	return _rotl(value, 4);
}

inline int bitScanReverse(unsigned int value)
{
	unsigned long pos = 0;
#ifdef _WIN32
	_BitScanReverse(&pos, value);
#else
	pos = 31 - __builtin_clz(value);
#endif
	return pos;
}

static int popCount(unsigned short a)
{
#ifdef _WIN32
#if ENABLE_SSE2
	return __popcnt16(a);
#else /* ENABLE_SSE2 */
	int n = 0;
	for (; a; a >>= 1) 
	{
		n += (a & 1);
	}
	return n;
#endif
#else /* _WIN32 */
	return  __builtin_popcount(a);
#endif
	
}

} // namespace Intrinsic
} // namespace Utilities

#endif // UTILITIES_INTRINSIC_H