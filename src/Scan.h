#pragma once

#include <emmintrin.h>

#include "MMConfig.h"

/// Scans an array of codewords and returns a 16-bit mask of appeared 
/// digits.
///
/// In the bit-mask returned, a bit is set if the corresponding digit
/// appears in at least one of the codewords. A bit is cleared if the
/// corresponding digit never appears in any of the codewords. The bits
/// are numbered from LSB to MSB. 
///
/// For example, if the codeword is <code>4169</code>, then bits 1, 4, 6,
/// and 9 are set, and the rest unset. The mask returned will be 
/// <code>0000-0010-0101-0010</code>, or <code>0x0252</code>.
///
/// Note that the highest bit (corresponding to digit 0xF) is never set
/// in the returned mask, even if it exists in the codeword, because
/// <code>0xF</code> is reserved for padding use.
unsigned short ScanDigitMask(
	/// Pointer to the codeword array
	const __m128i *codewords,
	/// Number of codewords in the array
	unsigned int count);
