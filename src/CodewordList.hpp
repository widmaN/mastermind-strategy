#ifndef MASTERMIND_CODEWORD_LIST_HPP
#define MASTERMIND_CODEWORD_LIST_HPP

//#include <string>
//#include <cassert>
#include <vector>

#include "Codeword.hpp"
#include "CodewordRules.hpp"
#include "AlignedAllocator.hpp"

//#include "MMConfig.h"
//#include "Feedback.h"

namespace Mastermind 
{
	
typedef 	std::vector<Codeword,aligned_allocator<Codeword,16>> CodewordList;

typedef CodewordList::iterator CodewordIterator;

/// Generates all codewords that conforms to the given set of rules.
CodewordList generateCodewords(const CodewordRules &rules);

// class FeedbackFrequencyTable;

#if 0
/// An array of codewords. 
class CodewordList
{
private:
	/// Rules that all codewords in this list are supposed to 
	/// conform to. Knowing the rules of the codewords can help optimize
	/// the performance of certain operations such as comparison.
	CodewordRules m_rules; 

	/// Number of codewords contained in this list.
	int m_count;

	/// Pointer to the storage of the codeword array.
	//codeword_t *m_data; 
	__m128i *m_data;

	/// Pointer to the memory actual allocated. This pointer can be different 
	/// from <code>m_data</code> if this list is part of a larger
	/// list.
	// codeword_t *m_alloc;
	__m128i *m_alloc;

	/// Pointer to the (volatile) memory that stores the reference counter.
	int volatile* m_refcount;

private:

	__m128i* Allocate(int count);
	void ReleaseCurrent();

public:

	/// Creates an empty list with the given rules.
	CodewordList(const CodewordRules &rules)
		: m_rules(rules)
	{
		m_refcount = NULL;
		m_alloc = NULL;
		m_data = NULL;
		m_count = 0;
	}

	/// Creates an empty list. 
	CodewordList()
	{
		m_refcount = NULL;
		m_alloc = NULL;
		m_data = NULL;
		m_count = 0;
	}

	/// Creates the list as a reference copy of another list. The two
	/// lists share the same memory, and changing one of them affects
	/// the other.
	CodewordList(const CodewordList& list);

	/// Creates the list as a reference copy of the range of another 
	/// list. The two lists share the same memory, and changing one 
	/// of them affects the other.
	CodewordList(const CodewordList& list, int start, int count);

	/// Destructor.
	~CodewordList()
	{
		ReleaseCurrent();
	}

	/// Sets the list to a reference copy of another list. The two
	/// lists share the same memory, and changing one of them affects
	/// the other.
	CodewordList& operator = (const CodewordList& list)
	{
		if (&list == this)
			return (*this);

		ReleaseCurrent();
		this->m_rules = list.m_rules;
		this->m_data = list.m_data;
		this->m_count = list.m_count;
		this->m_alloc = list.m_alloc;
		this->m_refcount = list.m_refcount;
		++(*m_refcount);
		return (*this);
	}

	CodewordList Copy() const;

public:

	/// Returns the rules that all codewords in this list conform to.
	const CodewordRules& GetRules() const { return m_rules; }

	/// Gets the number of codewords in this list.
	int GetCount() const { return m_count; }

	/// Gets a pointer to the internal array.
	const __m128i* GetData() const 
	{
		assert(m_data != NULL);
		return m_data; 
	}

	/// Gets the codeword at the given index.
	Codeword operator [] (int i) const
	{
		assert(m_data != NULL);
		assert(i >= 0 && i < m_count);
		return m_data[i];
	}

	unsigned short GetDigitMask() const;

	CodewordList FilterByFeedback(
		Codeword& guess, 
		Feedback fb) const;

	CodewordList FilterByEquivalence(
		const unsigned char eqclass[16]) const;

	CodewordList FilterByEquivalence(
		unsigned short unguessed_mask,
		unsigned short impossible_mask) const;

#if 0
	//void WriteToFile(FILE *fp) const;
	void DebugPrint() const
	{
		for (int i = 0; i < m_count; i++) {
			// std::cout << 
			printf("%s ", Codeword(m_data[i]).ToString().c_str());
		}
	}
#endif

};
#endif

} // namespace Mastermind

#endif // MASTERMIND_CODEWORD_LIST_HPP
