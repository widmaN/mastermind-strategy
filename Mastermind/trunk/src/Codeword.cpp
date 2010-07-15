#include <assert.h>
#include <malloc.h>

#include "MMConfig.h"
#include "Feedback.h"
#include "Compare.h"
#include "Enumerate.h"
#include "Codeword.h"
#include "Scan.h"
#include "Frequency.h"

using namespace Mastermind;

// IMPORTANT:
//
// NEVER use the 'new' operator. It doesn't respect SIMD type alignment,
// and will cause segfault. Use the _aligned_malloc() instead.

///////////////////////////////////////////////////////////////////////////
// Codeword implementations
//

Codeword Codeword::Parse(const char *s, CodewordRules rules)
{
#if MM_MAX_COLORS > 10
# error Codeword::Parse() only handles MM_MAX_COLORS <= 9
#endif

	int length = rules.length;
	int ndigits = rules.ndigits;
	bool allow_repetition = rules.allow_repetition;

	assert(s != NULL);
	assert(length > 0 && length <= MM_MAX_PEGS);
	assert(ndigits > 0 && ndigits <= MM_MAX_COLORS);

	Codeword ret;
	int k = 0;
	for (; k < length; k++) {
		if (s[k] >= '0' && s[k] <= '9') {
			unsigned char d = s[k] - '0';
			if (d > MM_MAX_COLORS) {
				return Codeword::Empty();
			}
			// BUG: WRONG ORDER
			if (!allow_repetition && ret.m_value.counter[d] > 0)
				return Codeword::Empty();
			ret.m_value.counter[d]++;
			ret.m_value.digit[k] = d;
		} else {
			return Codeword::Empty();
		}
	}
	if (s[k] != '\0') {
		return Codeword::Empty();
	}
	for (; k < MM_MAX_PEGS; k++) {
		ret.m_value.digit[k] = 0xFF;
	}
	return ret;
}

std::string Codeword::ToString() const
{
	char buf[17];
	int k;
	for (k = 0; k < MM_MAX_PEGS; k++) {
		unsigned char d = m_value.digit[k];
		if (d == 0xFF)
			break;
		assert(d >= 0 && d < MM_MAX_COLORS);
		buf[k] = '0' + d;
	}
	buf[k] = '\0';
	return buf;
}

FeedbackList* Codeword::CompareTo(const CodewordList& list) const
{
	int count = list.GetCount();
	unsigned char *results = new unsigned char [count];

	if (list.GetRules().allow_repetition) {
		Compare_Rep(m_value, list.GetData(), count, results);
	} else {
		Compare_NoRep(m_value, list.GetData(), count, results);
	}

	return new FeedbackList(results, count);
}

/// Compares this codeword to another codeword. 
/// \return The feedback of the comparison.
Feedback Codeword::CompareTo(Codeword& guess) const
{
	unsigned char fb;
	codeword_t guess_value = guess.GetValue();
	Compare_Rep(m_value, &guess_value, 1, &fb);
	return Feedback(fb);
}

/// Returns a 16-bit mask of digits present in the codeword.
unsigned short Codeword::GetDigitMask() const
{
	return ScanDigitMask(&m_value, 1);
}



///////////////////////////////////////////////////////////////////////////
// CodewordList implementations
//

codeword_t* CodewordList::Allocate(int count)
{
	m_alloc = (codeword_t *)_aligned_malloc(sizeof(codeword_t)*count, sizeof(codeword_t));
	m_data = m_alloc;
	m_count = count;
	m_refcount = new int(1);
	return m_alloc;
}

CodewordList CodewordList::Copy() const
{
	assert(m_count > 0);
	CodewordList cwl(m_rules);
	cwl.Allocate(m_count);
	memcpy(cwl.m_data, m_data, m_count * sizeof(codeword_t));
	return cwl;
}

void CodewordList::ReleaseCurrent()
{
	if (m_refcount != NULL) {
		if (--(*m_refcount) == 0) {
			_aligned_free(m_alloc);
			delete m_refcount;
		}
		m_refcount = NULL;
		m_alloc = NULL;
		m_data = NULL;
		m_count = 0;
	}
}

CodewordList CodewordList::Enumerate(CodewordRules rules)
{
	int length = rules.length;
	int ndigits = rules.ndigits;
	assert(length > 0 && length <= MM_MAX_PEGS);
	assert(ndigits > 0 && ndigits <= MM_MAX_COLORS);

	CodewordList list(rules);

	if (rules.allow_repetition) {
		int count = NPower(ndigits, length);
		codeword_t *data = list.Allocate(count);
		Enumerate_Rep(length, ndigits, data);
	} else {
		assert(ndigits >= length);
		int count = NPermute(ndigits, rules.length);
		codeword_t *data = list.Allocate(count);
		Enumerate_NoRep(length, ndigits, data);
	}
	
	return list;
}

CodewordList CodewordList::FilterByFeedback(
	Codeword &guess, 
	Feedback feedback) const
{
	// TODO: try stack alloc - it's fast!
	unsigned char fb = feedback.GetValue();
	FeedbackList *fblist = guess.CompareTo(*this);

	// Count feedbacks equal to fb
	int count = 0;
	if (1) {
		const unsigned char *pfb = fblist->GetData();
		int total = fblist->GetCount();
		while (total-- > 0) {
			if (*(pfb++) == fb)
				count++;
		}
	}

	// Copy elements whose feedback are equal to fb
	CodewordList cwl(m_rules);
	codeword_t *data = cwl.Allocate(count);
	int j = 0;
	for (int i = 0; i < fblist->GetCount(); i++) {
		if ((*fblist)[i] == fb) 
			data[j++] = m_data[i];
	}

	delete fblist;
	return cwl;
}

CodewordList CodewordList::FilterByEquivalence(
	const unsigned char eqclass[16]) const
{
	CodewordList list(m_rules);
	codeword_t *filtered = list.Allocate(m_count); // allocate more elements then necessary
	int count = 0;
	
	if (m_rules.allow_repetition) {
		count = FilterByEquivalence_Rep(m_data, m_count, eqclass, filtered);
	} else {
		count = FilterByEquivalence_NoRep(m_data, m_count, eqclass, filtered);
	}
	list.m_count = count;

	return list;
}

static inline void fill_eqclass(unsigned char eqclass[16], unsigned short mask)
{
	char first = -1, prev = -1;
	for (int i = 0; i < 16; i++) {
		if (mask & (1 << i)) {
			if (first == -1) {
				first = i;
			}
			if (prev >= 0) {
				eqclass[prev] = i;
			}
			prev = i;
		}
	}
	if (first >= 0) {
		eqclass[prev] = first;
	}
}

CodewordList CodewordList::FilterByEquivalence(
	unsigned short unguessed_mask,
	unsigned short impossible_mask) const
{
	unsigned char eqclass[16] = {
		0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15 };

	fill_eqclass(eqclass, unguessed_mask);
	fill_eqclass(eqclass, impossible_mask);

	return this->FilterByEquivalence(eqclass);
}

unsigned short CodewordList::GetDigitMask() const
{
	return ScanDigitMask(m_data, m_count);
}
