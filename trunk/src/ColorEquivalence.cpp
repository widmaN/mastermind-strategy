#include <assert.h>
#include <malloc.h>
#include <emmintrin.h>
#include <memory.h>
#include <array>
#include <numeric>

#include "Algorithm.hpp"
#include "util/intrinsic.hpp"
#include "Equivalence.hpp"

namespace Mastermind {

/// Represents a color equivalence filter.
class ColorEquivalenceFilter : public EquivalenceFilter
{
	Engine &e;
	ColorMask _unguessed;
	ColorMask _excluded;
	std::array<unsigned char,16> eqclass;

	CodewordList filter_norep(CodewordConstRange candidates) const;
	CodewordList filter_excluded_norep(CodewordConstRange candidates) const;
	CodewordList filter_rep(CodewordConstRange candidates) const;

	void update_eqclass()
	{
		std::iota(eqclass.begin(), eqclass.end(), (unsigned char)0);
#if 0
		for (int i = 0; i < 16; ++i)
		{
			eqclass[i] = i;
		}
#endif

		for (int last = -1, i = 0; i < e.rules().colors(); ++i)
		{
			if (_unguessed[i])
			{
				if (last >= 0)
				{
					eqclass[i] = eqclass[last];
					eqclass[last] = (unsigned char)i;
				}
				last = i;
			}
		}

		for (int last = -1, i = 0; i < e.rules().colors(); ++i)
		{
			if (_excluded[i])
			{
				if (last >= 0)
				{
					eqclass[i] = eqclass[last];
					eqclass[last] = (unsigned char)i;
				}
				last = i;
			}
		}
	}

public:

	ColorEquivalenceFilter(Engine &engine)
		: e(engine), _unguessed(ColorMask::fill(e.rules().colors()))
	{
		update_eqclass();
	}

	virtual EquivalenceFilter* clone() const
	{
		return new ColorEquivalenceFilter(*this);
	}

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates) const
	{
		if (e.rules().repeatable())
			return filter_rep(candidates);
		else
			//return filter_norep(candidates);
			return filter_excluded_norep(candidates);
	}

	virtual void add_constraint(
		const Codeword & guess,
		Feedback /* response */,
		CodewordConstRange remaining)
	{
		_excluded = ColorMask::fill(e.rules().colors());
		_excluded.reset(e.colorMask(remaining));
		_unguessed.reset(e.colorMask(guess));
		_unguessed.reset(_excluded);
		update_eqclass();
	}
};

CodewordList ColorEquivalenceFilter::filter_norep(
	CodewordConstRange candidates) const
{
	// Find out the largest digit each digit is equivalent to.
	// This information is stored in the variable _head_.
	// For example, if head[5]=3, this means the smallest digit
	// equivalent to 5 is 3.
	std::array<unsigned char,16> head;
	std::fill(head.begin(), head.end(), (unsigned char)0xFF);
	for (int i = 0; i < 16; i++)
	{
		if (i < head[i])
			head[i] = (unsigned char)i;
		int c = 0; // counter to avoid dead loop in case _eqclass_ is malformed.
		for (int p = eqclass[i]; (p != i) && (c <= 16); p=eqclass[p], c++)
		{
			if (i < head[p])
				head[p] = (unsigned char)i;
		}
		assert(c <= 16); // check bad loop
	}

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	CodewordList canonical;
	canonical.reserve(candidates.size());
	for (CodewordConstIterator it = candidates.begin(); it != candidates.end(); ++it)
	{
		Codeword guess = *it;
		std::array<unsigned char,16> chain(head);
		bool ok = true;
		for (int j = 0; j < e.rules().pegs(); j++)
		{
			unsigned char d = guess[j];
			unsigned char d0 = head[d];
			if (chain[d0] != d)
			{
				ok = false;
				break;
			}
			chain[d0] = eqclass[d];
		}
		if (ok)
		{
			canonical.push_back(guess);
		}
	}
	return canonical;
}

CodewordList ColorEquivalenceFilter::filter_rep(
	CodewordConstRange candidates) const
{
	// For codewords with repeated colors, we only apply color equivalence
	// on excluded colors.
	if (_excluded.empty())
		return CodewordList(candidates.begin(), candidates.end());

	int first = _excluded.smallest();

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	CodewordList canonical;
	canonical.reserve(candidates.size());
	for (CodewordConstIterator it = candidates.begin(); it != candidates.end(); ++it)
	{
		Codeword guess = *it;
		bool ok = true;
		for (int j = 0; j < e.rules().pegs(); j++)
		{
			unsigned char c = guess[j];
			if (_excluded[c] && c > first)
			{
				ok = false;
				break;
			}
		}
		if (ok)
		{
			canonical.push_back(guess);
		}
	}
	return canonical;
}

// @todo
// 1) clean up the code
// 2) we might use SSE2 to speed up part of the code
CodewordList ColorEquivalenceFilter::filter_excluded_norep(
	CodewordConstRange candidates) const
{
	// For each codeword without repetition, we check the color on each peg
	// in turn. If the color is excluded, it must be the smallest excluded
	// color, otherwise it is not canonical.
	if (_excluded.empty())
		return CodewordList(candidates.begin(), candidates.end());

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	CodewordList canonical;
	canonical.reserve(candidates.size());
	for (CodewordConstIterator it = candidates.begin(); it != candidates.end(); ++it)
	{
		Codeword guess = *it;

#if 0
		ColorMask unguessed = _unguessed;
#endif
		ColorMask excluded = _excluded;
		bool ok = true;

		for (int j = 0; j < e.rules().pegs(); j++)
		{
			unsigned char c = guess[j];
			if (excluded[c])
			{
				if ((excluded.value() & ((1 << c) - 1)) != 0)
				{
					ok = false;
					break;
				}
				excluded.reset(c);
			}
#if 0
			if (unguessed[c])
			{
				if ((unguessed.value() & ((1 << c) - 1)) != 0)
				{
					ok = false;
					break;
				}
				unguessed.reset(c);
			}
#endif
		}
		if (ok)
		{
			canonical.push_back(guess);
		}
	}
	return canonical;
}

static EquivalenceFilter* CreateColorEquivalenceFilter(Engine &e)
{
	return new ColorEquivalenceFilter(e);
}

REGISTER_ROUTINE(CreateEquivalenceFilterRoutine,
				 "Color",
				 CreateColorEquivalenceFilter)

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
