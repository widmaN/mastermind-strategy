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

/*
void Codeword::CompareTo(const CodewordList& list, FeedbackList& fbl) const
{
	assert(list.GetCount() == fbl.GetCount());

	int count = list.GetCount();
	unsigned char *results = fbl.GetData();

	if (list.GetRules().allow_repetition) {
		CompareRepImpl->Run(m_value.value, (const __m128i*)list.GetData(), count, results);
	} else {
		CompareNoRepImpl->Run(m_value.value, (const __m128i*)list.GetData(), count, results);
	}
}
*/

/// Compares this codeword to another codeword. 
/// \return The feedback of the comparison.
Feedback Codeword::CompareTo(const Codeword& guess) const
{
	unsigned char fb;
	codeword_t guess_value = guess.GetValue();
	CompareRepImpl->Run(m_value.value, (const __m128i*)&guess_value, 1, &fb);
	return Feedback(fb);
}

/// Returns a 16-bit mask of digits present in the codeword.
unsigned short Codeword::GetDigitMask() const
{
	return ScanDigitMask(&m_value, 1);
}

int Codeword::GetPegCount() const
{
	int k;
	for (k = 0; k < MM_MAX_PEGS; k++) {
		unsigned char d = m_value.digit[k];
		if (d == 0xFF)
			break;
	}
	return k;
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

CodewordList::CodewordList(const CodewordList& list)
{
	if (&list == this)
		return;

	this->m_rules = list.m_rules;
	this->m_data = list.m_data;
	this->m_count = list.m_count;
	this->m_alloc = list.m_alloc;
	this->m_refcount = list.m_refcount;
	++(*m_refcount);
}

CodewordList::CodewordList(const CodewordList& list, int start, int count)
{
	assert(&list != this);
	assert(start >= 0);
	assert(count >= 0);
	assert(start + count <= list.m_count);
	
	this->m_rules = list.m_rules;
	this->m_data = list.m_data + start;
	this->m_count = count;
	this->m_alloc = list.m_alloc;
	this->m_refcount = list.m_refcount;
	++(*m_refcount);
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
	// TODO: Does this function take a lot of time?
	unsigned char fb = feedback.GetValue();
	FeedbackList fblist(guess, *this);

	// Count feedbacks equal to fb
	int count = 0;
	if (1) {
		const unsigned char *pfb = fblist.GetData();
		int total = fblist.GetCount();
		while (total-- > 0) {
			if (*(pfb++) == fb)
				count++;
		}
	}

	// Copy elements whose feedback are equal to fb
	CodewordList cwl(m_rules);
	codeword_t *data = cwl.Allocate(count);
	int j = 0;
	for (int i = 0; i < fblist.GetCount(); i++) {
		if (fblist[i] == fb) 
			data[j++] = m_data[i];
	}

	return cwl;
}

void CodewordList::Partition(const Codeword &guess, FeedbackFrequencyTable &freq)
{
	FeedbackList fbl(guess, *this);
	freq.CountFrequencies(fbl);

	// Build a table of start index of each feedback
	int start_index[256];
	int k = 0;
	start_index[0] = 0;
	for (int i = 0; i <= freq.GetMaxFeedbackValue(); i++) {
		start_index[i] = k;
		k += freq[Feedback(i)];
	}

	// Create a spare list to store temporary result
	// TODO: Modify code to enable MT
	static codeword_t* tmp_list = NULL;
	static int tmp_size = 0;
	if (tmp_size < m_count) {
		_aligned_free(tmp_list);
		tmp_list = (codeword_t*)_aligned_malloc(sizeof(codeword_t)*m_count, sizeof(codeword_t));
		tmp_size = m_count;
	}

	// Re-order codewords to the temporary list
	const unsigned char *fblist = fbl.GetData();
	for (int i = 0; i < m_count; i++) {
		tmp_list[start_index[fblist[i]]++] = m_data[i];
	}

	// Copy temporary list back
	for (int i = 0; i < m_count; i++) {
		m_data[i] = tmp_list[i];
	}
}

/*
void CodewordList::Partition(const Codeword &guess, FeedbackFrequencyTable &freq)
{
	FeedbackList fbl(guess, *this);
	freq.CountFrequencies(fbl);

	// Build a table of start index of each feedback
	int start_index[256];
	int k = 0;
	start_index[0] = 0;
	for (int i = 0; i <= freq.GetMaxFeedbackValue(); i++) {
		start_index[i] = k;
		k += freq[Feedback(i)];
	}

	// Create a spare list to store temporary result
	// TODO: Modify code to enable MT
	static codeword_t* tmp_list = NULL;
	static int tmp_size = 0;
	if (tmp_size < m_count) {
		_aligned_free(tmp_list);
		tmp_list = (codeword_t*)_aligned_malloc(sizeof(codeword_t)*m_count, sizeof(codeword_t));
		tmp_size = m_count;
	}

	// Re-order codewords to the temporary list
	const unsigned char *fblist = fbl.GetData();
	for (int i = 0; i < m_count; i++) {
		tmp_list[start_index[fblist[i]]++] = m_data[i];
	}

	// Copy temporary list back
	for (int i = 0; i < m_count; i++) {
		m_data[i] = tmp_list[i];
	}
}
*/

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

	if (unguessed_mask & impossible_mask) {
		if (unguessed_mask & ~impossible_mask) {
			int k = 0;
		}
	}
	fill_eqclass(eqclass, unguessed_mask);
	fill_eqclass(eqclass, impossible_mask);

	return this->FilterByEquivalence(eqclass);
}

unsigned short CodewordList::GetDigitMask() const
{
	return ScanDigitMask(m_data, m_count);
}
