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

} // namespace Intrinsic
} // namespace Utilities

#endif // UTILITIES_INTRINSIC_H