#include <assert.h>
#include <malloc.h>
#include <emmintrin.h>
#include <memory.h>

#include "Algorithm.hpp"
#include "util/intrinsic.hpp"

namespace Mastermind {

#if 0
// Returns n!/r!
int NPermute(int n, int r)
{
	assert(n >= r);
	if (n < r)
		return 0;

	int p = 1;
	for (int k = n - r + 1; k <= n; k++) {
		p *= k;
	}
	return p;
}

// Returns n^r
int NPower(int n, int r)
{
	assert(r >= 0);

	int p = 1;
    for (int k = 1; k <= r; k++) {
        p *= n;
    }
    return p;
}

// Returns n!/r!/(n-r)!
int NComb(int n, int r)
{
	return NPermute(n,r)/NPermute(r,r);
}
#endif

////////////////////////////////////////////////////////////////////////////
// Filter routines

// Filters the codeword list _src_ by removing duplicate elements according
// to equivalence class _eqclass_. Returns the number of elements remaining.
//
// _eqclass_ is a 16-byte array where each element specifies the next digit
// in the same equivalent class. Hence, each equivalence class is chained
// through a loop. For example: (assume only 10 digits)
//                  0  1  2  3  4  5  6  7  8  9
//  all-different:  0  1  2  3  4  5  6  7  8  9
//  all-same:       1  2  3  4  5  6  7  8  9  0
//  1,3,5 same:     0  3  2  5  4  1  6  7  8  9
//
// This function keeps the lexicographical-minimum codeword of all codewords
// equivalent to it. Therefore, this minimum codeword must exist in _src_ for
// the function to work correctly.

#if 0

static size_t FilterByEquivalenceClass_norep_v1(
	const Codeword *first,
	const Codeword *last,
	const unsigned char eqclass[16],
	Codeword *filtered)
	//const __m128i *src,
	//int nsrc,
	//const unsigned char eqclass[16],
	//__m128i *dest)
{
	// Find out the largest digit each digit is equivalent to.
	unsigned char head[16];
	memset(head, -1, 16);
	for (int i = 0; i < 16; i++) 
	{
		if (i < head[i])
			head[i] = i;
		for (int p = eqclass[i], c = 0; (p != i) && (c <= 16); p=eqclass[p], c++)
		{
			if (i < head[p])
				head[p] = i;
		}
	}

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	size_t ndest = 0;
	for (int i = 0; i < nsrc; i++) 
	{
		unsigned long cw = codeword_to_dword(src[i]);
		unsigned long cw_remapped = 0;
		unsigned char chain[16];
		memcpy(chain, head, 16);
		for (int j = 0; j < 8; j++) 
		{
			cw = util::intrinsic::rotate_left(cw, 4);
			unsigned char k = (cw & 0x0f);
			unsigned char k_remapped = chain[head[k]];
			chain[head[k]] = eqclass[k_remapped];
			cw_remapped <<= 4;
			cw_remapped |= k_remapped;
		}
		if (cw == cw_remapped) 
		{
			dest[ndest++] = src[i];
		}
	}
	return ndest;
}

int FilterByEquivalenceClass_norep_v2(
	const Codeword *src,
	int nsrc,
	const unsigned char eqclass[16],
	Codeword *dest)
{
	assert(src != NULL);
	assert(nsrc >= 0);
	assert(dest != NULL);

	// Find out the largest digit each digit is equivalent to.
	// This information is stored in the variable _head_.
	// For example, if head[5]=3, this means the smallest digit
	// equivalent to 5 is 3.
	unsigned char head[16];
	memset(head, -1, 16);
	for (int i = 0; i < 16; i++) {
		if (i < head[i])
			head[i] = i;
		int c = 0; // counter to avoid dead loop in case _eqclass_ is malformed.
		for (int p = eqclass[i]; (p != i) && (c <= 16); p=eqclass[p], c++) {
			if (i < head[p])
				head[p] = i;
		}
		assert(c <= 16); // check bad loop
	}

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	int ndest = 0;
	for (int i = 0; i < nsrc; i++) {
		unsigned char cw_remapped[MM_MAX_PEGS];
		unsigned char chain[16];
		memcpy(chain, head, 16);
		for (int j = 0; j < MM_MAX_PEGS; j++) {
			unsigned char d = src[i].digit[j];
			if (d == 0xFF) {
				cw_remapped[j] = 0xFF;
			} else {
				unsigned char d_remapped = chain[head[d]];
				assert(d_remapped < 16);
				chain[head[d]] = eqclass[d_remapped];
				cw_remapped[j] = d_remapped;
			}
		}
		if (memcmp(src[i].digit, cw_remapped, MM_MAX_PEGS) == 0) {
			dest[ndest++] = src[i];
		}
	}
	return ndest;
}

int FilterByEquivalenceClass_norep_v3(
	const Codeword *src,
	int nsrc,
	const unsigned char eqclass[16],
	Codeword *dest)
{
	assert(src != NULL);
	assert(nsrc >= 0);
	assert(dest != NULL);

	// Find out the largest digit each digit is equivalent to.
	// This information is stored in the variable _head_.
	// For example, if head[5]=3, this means the smallest digit
	// equivalent to 5 is 3.
	unsigned char head[16];
	memset(head, -1, 16);
	for (int i = 0; i < 16; i++) {
		if (i < head[i])
			head[i] = i;
		int c = 0; // counter to avoid dead loop in case _eqclass_ is malformed.
		for (int p = eqclass[i]; (p != i) && (c <= 16); p=eqclass[p], c++) {
			if (i < head[p])
				head[p] = i;
		}
		assert(c <= 16); // check bad loop
	}

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	int ndest = 0;
	for (int i = 0; i < nsrc; i++) {
		unsigned char chain[16];
		memcpy(chain, head, 16);
		bool ok = true;
		for (int j = 0; j < MM_MAX_PEGS; j++) {
			unsigned char d = src[i].digit[j];
			if (d == 0xFF) {
				break;
			}
			unsigned char d0 = head[d];
			if (chain[d0] != d) {
				ok = false;
				break;
			}
			chain[d0] = eqclass[d];
		}
		if (ok) {
			dest[ndest++] = src[i];
		}
	}
	return ndest;
}

int FilterByEquivalenceClass_rep_v1(
	const Codeword *src,
	int nsrc,
	const unsigned char eqclass[16],
	Codeword *dest)
{
	assert(src != NULL);
	assert(nsrc >= 0);
	assert(dest != NULL);

	// Find out the largest digit each digit is equivalent to.
	// This information is stored in the variable _head_.
	// For example, if head[5]=3, this means the smallest digit
	// equivalent to 5 is 3.
	unsigned char head[16];
	memset(head, -1, 16);
	for (int i = 0; i < 16; i++) {
		if (i < head[i])
			head[i] = i;
		int c = 0; // counter to avoid dead loop in case _eqclass_ is malformed.
		for (int p = eqclass[i]; (p != i) && (c <= 16); p=eqclass[p], c++) {
			if (i < head[p])
				head[p] = i;
		}
		assert(c <= 16); // check bad loop
	}

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	int ndest = 0;
	for (int i = 0; i < nsrc; i++) {
		unsigned char chain[16];
		unsigned char maxrep[16];
		unsigned char remrep[16]; // remaining repetitions
		memcpy(chain, head, 16);
		memset(maxrep, -1, 16);
		memset(remrep, 0, 16);
		bool ok = true;
		for (int j = 0; j < MM_MAX_PEGS; j++) {
			unsigned char d = src[i].digit[j];
			if (d == 0xFF) {
				break;
			}
			unsigned char d0 = head[d]; // the smallest digit equivalent to d; used
			                            // to represent its equivalence class
			if (chain[d0] != d) {
				ok = false;
				break;
			}
			if (remrep[d0] > 0) {
				if (--remrep[d0] == 0) {
					chain[d0] = eqclass[d]; // move to next element in the digit's
					                        // equivalence class
					remrep[d0] = 0;
				}
			} else {
				unsigned char rep = src[i].counter[d]; // number of repetitions of d
				if (maxrep[d0] < rep) {
					ok = false;
					break;
				}
				maxrep[d0] = rep;
				if (rep > 1) {
					remrep[d0] = rep - 1;
				} else {
					chain[d0] = eqclass[d];
				}
			}
		}
		if (ok) {
			dest[ndest++] = src[i];
		}
	}
	return ndest;
}

// TODO: impossible digit is not repetition-sensitive (repsens)
int FilterByEquivalenceClass_rep_v2(
	const Codeword *src,
	int nsrc,
	const unsigned char eqclass[16],
	Codeword *dest)
{
	assert(src != NULL);
	assert(nsrc >= 0);
	assert(dest != NULL);

	// Find out the largest digit each digit is equivalent to.
	// This information is stored in the variable _head_.
	// For example, if head[5]=3, this means the smallest digit
	// equivalent to 5 is 3.
	unsigned char head[16];
	memset(head, -1, 16);
	for (int i = 0; i < 16; i++) {
		if (i < head[i])
			head[i] = i;
		int c = 0; // counter to avoid dead loop in case _eqclass_ is malformed.
		for (int p = eqclass[i]; (p != i) && (c <= 16); p=eqclass[p], c++) {
			if (i < head[p])
				head[p] = i;
		}
		assert(c <= 16); // check bad loop
	}

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	int ndest = 0;
	for (int i = 0; i < nsrc; i++) {
		unsigned char chain[16];
		memcpy(chain, head, 16);
		bool ok = true;
		for (int j = 0; j < MM_MAX_PEGS; j++) {
			unsigned char d = src[i].digit[j];
			if (d == 0xFF) {
				break;
			}
			unsigned char d0 = head[d]; // the smallest digit equivalent to d; used
			                            // to represent its equivalence class
			if (d > chain[d0]) {
				ok = false;
				break;
			} else if (d < chain[d0]) {
				continue;
			} else {
				chain[d0] = eqclass[d];
			}
		}
		if (ok) {
			dest[ndest++] = src[i];
		}
	}
	return ndest;
}

///////////////////////////////////////////////////////////////////////////
// Interface routines
//

int FilterByEquivalence_NoRep(
	const Codeword *src,
	unsigned int nsrc,
	const unsigned char eqclass[16],
	Codeword *dest)
{
	//return FilterByEquivalenceClass_norep_v1((__m128i*)src, nsrc, eqclass, (__m128i*)dest);
	return FilterByEquivalenceClass_norep_v3(src, nsrc, eqclass, dest);
}

int FilterByEquivalence_Rep(
	const Codeword *src,
	unsigned int nsrc,
	const unsigned char eqclass[16],
	Codeword *dest)
{
	if (1) {
		return FilterByEquivalenceClass_rep_v2(src, nsrc, eqclass, dest);
		//return FilterByEquivalenceClass_rep_v1(src, nsrc, eqclass, dest);
	} else {
		for (unsigned int i = 0; i < nsrc; i++) {
			dest[i] = src[i];
		}
		return nsrc;
	}
}

#endif

} // namespace Mastermind
