#include <cassert>

//#include <malloc.h>

//#include "MMConfig.h"
//#include "Feedback.h"
//#include "Compare.h"
#include "Enumerate.h"
#include "Scan.h"
//#include "Frequency.h"

#include "CodewordRules.hpp"
#include "CodewordList.hpp"

namespace Mastermind {

// Generates all codewords that conforms to the given set of rules.
CodewordList generateCodewords(const CodewordRules &rules)
{
	assert(rules.valid());

	// Compute number of possibilities.
	size_t count = rules.repeatable()? 
		NPower(rules.colors(), rules.pegs()) : NPermute(rules.colors(), rules.pegs());

	// Creates the empty list.
	// @todo: do not initialize Codewords.
	CodewordList list(count);

	// Invoke the handler function to generate all codewords.
	if (rules.repeatable()) 
	{
		Enumerate_Rep(rules.pegs(), rules.colors(), (codeword_t*)list.data());
	} 
	else 
	{
		Enumerate_NoRep(rules.pegs(), rules.colors(), (codeword_t*)list.data());
	}
	return list;
}

#if 0

// IMPORTANT:
//
// NEVER use the 'new' operator. It doesn't respect SIMD type alignment,
// and will cause segfault. Use the _aligned_malloc() instead.

__m128i* CodewordList::Allocate(int count)
{
	m_alloc = (__m128i *)_aligned_malloc(sizeof(codeword_t)*count, sizeof(codeword_t));
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

#if 0
void CodewordList::Partition(const Codeword &guess, FeedbackFrequencyTable &freq)
{
	FeedbackList fbl(guess, *this);
	freq.CountFrequencies(fbl);

	// Build a table of start index of each feedback
	int start_index[256];
	int k = 0;
	start_index[0] = 0;
	for (int i = 0; i <= freq.maxFeedback(); i++) {
		start_index[i] = k;
		k += freq[Feedback(i)];
	}

	// Create a spare list to store temporary result
	// TODO: Modify code to enable MT
	static __m128i* tmp_list = NULL;
	static int tmp_size = 0;
	if (tmp_size < m_count) {
		_aligned_free(tmp_list);
		tmp_list = (__m128i *)_aligned_malloc(sizeof(codeword_t)*m_count, sizeof(codeword_t));
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
#endif

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
	codeword_t *filtered = (codeword_t *)list.Allocate(m_count); // allocate more elements then necessary
	int count = 0;
	
	if (m_rules.repeatable()) {
		count = FilterByEquivalence_Rep((const codeword_t *)m_data, m_count, eqclass, filtered);
	} else {
		count = FilterByEquivalence_NoRep((const codeword_t *)m_data, m_count, eqclass, filtered);
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

#endif

} // namespace Mastermind
