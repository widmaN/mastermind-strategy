#ifndef MASTERMIND_PERMUTATION_HPP
#define MASTERMIND_PERMUTATION_HPP

#include <iostream>
#include <cassert>
#include "Codeword.hpp"

namespace Mastermind {

/**
 * Permutation that permutes the pegs and colors in a codeword.
 * It is the composition of a peg permutation and a color permutation.
 */
struct CodewordPermutation
{
	char color[MM_MAX_COLORS];
	char peg[MM_MAX_PEGS];

public:

	/// Creates an identity permutation.
	CodewordPermutation()
	{
		for (int i = 0; i < MM_MAX_COLORS; ++i)
			color[i] = i;
		for (int i = 0; i < MM_MAX_PEGS; ++i)
			peg[i] = i;
	}

#if 0
	/// Returns the inverse of the permutation.
	CodewordPermutation inverse() const 
	{
		CodewordPermutation ret(_rules);
		for (int i = 0; i < _rules.pegs(); ++i)
		{
			if (peg[i] >= 0)
				ret.peg[peg[i]] = i;
		}
		for (int i = 0; i < _rules.colors(); ++i)
		{
			if (color[i] >= 0)
				ret.color[color[i]] = i;
		}
		return ret;
	}
#endif

	/// Returns the mapped value of a peg.
	//char peg(size_t i) const { return _peg[i]; }
	//char& peg(size_t i) { return _peg[i]; }

	/// Returns the mapped value of a color.
	//int color(int i) const { return _color[i]; }

	/// Permutes the pegs and colors in a codeword.
	Codeword permute(const Codeword &w) const
	{
		Codeword ret;
		for (int i = 0; i < MM_MAX_PEGS && w[i] != 0xFF; ++i)
		{
			ret.set(peg[i], color[w[i]]);
		}
		return ret;
	}

	/// Permutes the pegs in a codeword.
	Codeword permute_pegs(const Codeword &w) const
	{
		Codeword ret;
		for (int i = 0; i < MM_MAX_PEGS /* && w[i] != 0xFF */; ++i)
			ret.set(peg[i], w[i]);
		return ret;
	}

#if 0
	Codeword permute_colors(const Codeword &w) const
	{
		Codeword ret;
		for (int i = 0; i < _rules.pegs(); ++i)
			ret.set(i, color[w[i]]);
		return ret;
	}
#endif

	friend std::ostream& operator << (
		std::ostream& os, const CodewordPermutation &p) 
	{
		// Output peg permutation.
		os << "(";
		for (int i = 0; i < MM_MAX_PEGS; ++i)
		{
			if (i > 0)
				os << ' ';
			os << (size_t)p.peg[i];
		}
		os << ") o (";

		// Output color permutation.
		for (int i = 0; i < MM_MAX_COLORS; ++i)
		{
			if (i > 0)
				os << ' ';
			os << (size_t)p.color[i];
		}
		os << ")";
		return os;
	}
};

} // namespace Mastermind

#endif // MASTERMIND_PERMUTATION_HPP
