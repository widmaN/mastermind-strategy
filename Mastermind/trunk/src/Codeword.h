#pragma once

#include <string>
#include <assert.h>
//#include <memory.h>

#include "MMConfig.h"
#include "Feedback.h"

/// Contains everything related to the %Mastermind game.
namespace Mastermind 
{
	class Codeword;
	class CodewordList;

	/// Defines the rules that a codeword conforms to.
	class CodewordRules
	{
	public:
		/// Length of a codeword (i.e. the number of pegs).
		unsigned char length;

		/// Number of different digits that may appear in a codeword. 
		/// In a typical number guessing game, this is 10. In a Mastermind game,
		/// this is the number of colors.
		unsigned char ndigits;

		/// Whether the same digit (color) can appear more than once in a codeword.
		bool allow_repetition;

		// Reserved; must be zero.
		// bool allow_empty;
	public:

		bool IsValid() const 
		{
			return (length > 0 && length <= MM_MAX_PEGS) 
				&& (ndigits > 0 && ndigits <= MM_MAX_COLORS) 
				&& (allow_repetition || ndigits >= length);
		}

		unsigned short GetFullDigitMask() const
		{
			return (1 << ndigits) - 1;
		}
	};

	/// Represents a codeword.
	class Codeword
	{
	private:

		/// The internal representation of the codeword value.
		codeword_t m_value;

	public:

		/// Creates an empty codeword. 
		///
		/// @remark
		/// Use this constructor to create an empty codeword. An empy 
		/// codeword contains no pegs. It can be used as a special value 
		/// for certain purposes (for example, to represent an error). 
		/// 
		/// An empty codeword can also be returned by 
		/// <code>Codeword::Empty()</code>.
		/// You can test whether a codeword is empty by 
		/// <code>Codeword::IsEmpty()</code>.
		///
		/// @see <code>Empty()</code>, <code>IsEmpty()</code>
		Codeword()
		{
			memset(m_value.counter, 0, sizeof(m_value.counter));
			memset(m_value.digit, -1, sizeof(m_value.digit));
		}

		/// Creates the codeword from its internal representation.
		Codeword(codeword_t &value)
		{
			m_value = value;
		}

		Codeword(__m128i value128)
		{
			m_value.value = value128;
		}

		/// Gets the internal representation of the codeword.
		codeword_t GetValue() const 
		{
			return m_value;
		}

		/// Tests whether the codeword is empty.
		/// An empty codeword can be returned by <code>Codeword::Empty()</code>.
		/// @see <code>Codeword::Empty()</code>
		bool IsEmpty() const 
		{
			return (m_value.digit[0] == 0xFF);
		}

		/// Tests whether the codeword contains any color more than once.
		bool HasRepetition() const
		{
			for (int i = 0; i < MM_MAX_COLORS; i++) {
				if (m_value.counter[i] > 1)
					return true;
			}
			return false;
		}

		/// Converts the codeword to a string.
		std::string ToString() const;

		/// Compares this codeword to another codeword. 
		/// Returns the feedback.
		Feedback CompareTo(const Codeword& guess) const;

		/// Compares this codeword to a codeword list. 
		/// @return A list of feedbacks.
		void CompareTo(const CodewordList& list, FeedbackList& fbl) const;

		/// Returns a 16-bit mask of digits present in the codeword.
		unsigned short GetDigitMask() const;

		/// Gets the number of pegs in the codeword.
		int GetPegCount() const;

	public:

		/// Returns an empty codeword.
		/// This function has the same effect as calling the constructor with
		/// no parameter.
		static Codeword Empty()
		{
			return Codeword();
		}

		/// Parses codeword from a string, conforming to given rules.
		/// If the input text is invalid or is not conformant to the given rules,
		/// <code>Codeword::Empty()</code> is returned.
		static Codeword Parse(
			/// The string to parse
			const char *text,
			/// The rules to apply
			CodewordRules rules);

		/// Tests whether two codewords are equal.
		static friend bool operator == (Codeword& a, Codeword& b)
		{
			return memcmp(&a.m_value, &b.m_value, 16) == 0;
		}
	};

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
		codeword_t *m_data; 

		/// Pointer to the memory actual allocated. This pointer can be different 
		/// from <code>m_data</code> if this list is part of a larger
		/// list.
		codeword_t *m_alloc;

		/// Pointer to the (volatile) memory that stores the reference counter.
		int volatile* m_refcount;

	private:

		codeword_t* Allocate(int count);
		void ReleaseCurrent();

	public:

		/// Creates an empty list with the given rules.
		CodewordList(CodewordRules rules)
		{
			m_rules = rules;
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
		CodewordList(const CodewordList& list)
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
		CodewordRules GetRules() const { return m_rules; }

		/// Gets the number of codewords in this list.
		int GetCount() const { return m_count; }

		/// Gets a pointer to the internal array.
		const codeword_t *GetData() const 
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

		// Partition this codeword list according to feedback list _fbl_.
		// The elements in the codeword list are reordered in-memory.
		// After the partitioning, codewords with the same feedback are
		// stored consecutively in memory.
		// The feedback list _fbl_ is also reordered along with the codewords.
		//void Partition(FeedbackList *fbl);

		//void WriteToFile(FILE *fp) const;
		void DebugPrint() const
		{
			for (int i = 0; i < m_count; i++) {
				printf("%s ", Codeword(m_data[i]).ToString().c_str());
			}
		}

	public:

		/// Enumerates all codewords conforming to the given rules.
		static CodewordList Enumerate(
			CodewordRules rules);

	};


	/*
	class DigitMask
	{
	private:
		unsigned short m_mask;

	public:
		DigitMask() : m_mask(0) { }
		DigitMask(unsigned short mask) : m_mask(mask) { }
		DigitMask(const DigitMask& dm) : m_mask(dm.m_mask) { }

		DigitMask(

		operator unsigned short () const { return m_mask; }
		DigitMask& operator = (unsigned short mask) { m_mask = mask; return (*this); }
	};
	*/

};
