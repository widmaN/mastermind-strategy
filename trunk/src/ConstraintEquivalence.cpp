#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>

#include "Engine.hpp"
#include "Permutation.hpp"
#include "Equivalence.hpp"

#include "util/intrinsic.hpp"
#include "util/call_counter.hpp"

REGISTER_CALL_COUNTER(ConstraintEquivalence)
// REGISTER_CALL_COUNTER(ConstraintEquivalenceIdentity)
REGISTER_CALL_COUNTER(ConstraintEquivalencePermutation)
REGISTER_CALL_COUNTER(ConstraintEquivalenceCrossout)

namespace Mastermind {

struct MyColorMask
{
	unsigned short value;

	MyColorMask() : value(0) { }

	MyColorMask(int count) : value((1 << count) - 1) 
	{ 
		assert(count >= 0 && count <= MM_MAX_COLORS);
	}
	
	bool operator [] (int color) const
	{
		assert(color >= 0 && color <= MM_MAX_COLORS);
		return (value & (1 << color)) != 0;
	}

	int smallest() const 
	{
		return value == 0 ? -1 : util::intrinsic::bit_scan_forward(value);
	}

	void reset(int color)
	{
		assert(color >= 0 && color <= MM_MAX_COLORS);
		value &= ~(1 << color);
	}

	void reset() 
	{
		value = 0;
	}

	bool unique() const
	{
		return value && (value & (value - 1)) == 0;
	}

	bool empty() const { return value == 0; }

#if 0
	int count() const
	{
		int n = 0;
		for (int i = 0; i < MM_MAX_COLORS; ++i)
		{
			if (value & (1 << i))
				++n;
		}
		return n;
	}

	void set_count(int count)
	{
		assert(count >= 0 && count <= MM_MAX_COLORS);
		value = (1 << count) - 1;
	}
#endif
};

//typedef MyColorMask ColorMask;

/// Represents an incremental constraint equivalence filter.
class ConstraintEquivalenceFilter : public EquivalenceFilter
{
	Engine &e;
	MyColorMask free_colors;

	std::vector<	CodewordPermutation> pp;

public:

	ConstraintEquivalenceFilter(Engine &engine);

	virtual EquivalenceFilter* clone() const
	{
		return new ConstraintEquivalenceFilter(*this);
	}

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates) const;

	virtual void add_constraint(
		const Codeword &guess,
		Feedback response, 
		CodewordConstRange remaining
		);
};

/// Initializes a constraint equivalence filter.
ConstraintEquivalenceFilter::ConstraintEquivalenceFilter(Engine &engine)
	: e(engine), free_colors(e.rules().colors())
{
	// Generate all peg permutations, and associate with each peg 
	// permutation a fully unrestricted partial color permutation.
	CodewordPermutation p;
	do
	{
		pp.push_back(p);
	} 
	while (std::next_permutation(p.peg + 0, p.peg + e.rules().pegs()));
}

// Returns a list of canonical guesses given the current constraints.
CodewordList ConstraintEquivalenceFilter::get_canonical_guesses(
	CodewordConstRange candidates) const
{
	bool verbose = false;

#if 1
	// Optimization: if there is only one peg permutation left (in
	// which case it must be the identity permutation) and there are
	// no free colors left, then the color permutation must be 
	// identity too, and there is no codeword to filter out.
	if (pp.size() == 1 && free_colors.empty())
		return CodewordList(candidates.begin(), candidates.end());
#endif

	// Check each candidate in turn.
	size_t n = candidates.size();
	CodewordList canonical;
	canonical.reserve(n);
	for (size_t i = 0; i < n; ++i)
	{
		const Codeword candidate = candidates.begin()[i];
		bool is_canonical = true;

		// Check each peg permutation to see if there exists a peg/color
		// permutation that maps the candidate to a lexicographically
		// smaller equivalent codeword.
		for (size_t j = 0; j < pp.size(); ++j)
		{
			// Permute the pegs and colors of the candidate.
			// Free colors are kept unchanged.
			// Optimization: if this is the identity permutation,
			// then needn't permute.
			CodewordPermutation p = pp[j];
			Codeword permuted_candidate = 
				(j == 0)? candidate : p.permute_pegs(candidate);

			// Check the color on each peg in turn.
			// Take, for example, 1223. It must be able to map to 1123 and
			// show that it's not canonical.
			MyColorMask free_from = free_colors, free_to = free_colors;
			for (int k = 0; k < e.rules().pegs(); ++k)
			{
				// Let c be the color on peg k of the peg-permuted candidate.
				int c = permuted_candidate[k];

				// If c is free, map it to the smallest available free color,
				// and update the permutation. Otherwise, map it according 
				// to the current permutation.
				if (free_from[c])
				{
					// Find the smallest available free color.
					int cc = free_to.smallest();

					// Map c to cc.
					p.color[c] = cc;

					// Clear the respective free-color indicator.
					free_from.reset(c);
					free_to.reset(cc);
				}
				c = p.color[c];

				// The mapped color, c, must be lexicographically greater
				// than or equal to the corresponding color in the candidate
				// for the candidate to be canonical.
				if (c < candidate[k])
				{
					is_canonical = false;
					break;
				}
				else if (c > candidate[k])
				{
					break;
				}
			}
			if (!is_canonical)
				break;
		}

		// Append the candidate to the result if it's canonical.
		if (is_canonical)
			canonical.push_back(candidate);
	}

#if 0
	UPDATE_CALL_COUNTER(ConstraintEquivalence, canonical.size());
	UPDATE_CALL_COUNTER(ConstraintEquivalencePermutation, pp.size());
	UPDATE_CALL_COUNTER(ConstraintEquivalenceCrossout, (n-canonical.size()+1));
#endif

	return canonical;
}

void ConstraintEquivalenceFilter::add_constraint(
	const Codeword &guess,
	Feedback /* response */, 
	CodewordConstRange /* remaining */)
{
	bool verbose = false;

	if (verbose)
		std::cout << "Adding constraint: " << guess << std::endl;

	// For each peg permutation, restrict its associated partial 
	// color permutation so that the supplied guess maps to itself
	// under the peg+color permutation. If this is not possible,
	// remove the peg permutation from the list.
	for (size_t i = pp.size(); i > 0; )
	{
		--i;
		CodewordPermutation &p = pp[i];

		// Permute the pegs in the guess.
		Codeword permuted = p.permute_pegs(guess);

		// Try to map the color on each peg onto itself.
		MyColorMask free_from = free_colors, free_to = free_colors;
		bool ok = true;
		for (int j = 0; j < e.rules().pegs() && ok; ++j)
		{
			if (free_from[permuted[j]])
			{
				if (!free_to[guess[j]])
				{
					ok = false;
				}
				else
				{
					p.color[permuted[j]] = guess[j];
					free_from.reset(permuted[j]);
					free_to.reset(guess[j]);
				}
			}
			else if (p.color[permuted[j]] != guess[j])
				ok = false;
		}

		// Remove the peg permutation if no color permutation exists
		// that maps the guess onto itself.
		if (!ok)
		{
			if (verbose)
				std::cout << "Removed peg permutation: " << pp[i] << std::endl;

			std::swap(pp[i], pp[pp.size()-1]);
			pp.erase(pp.begin() + pp.size() - 1);
		}
		else
		{
			if (verbose)
				std::cout << "Restricted peg permutation: "
					<< pp[i] << std::endl;
		}
	}

	// Restrict the color mask.
	for (int i = 0; i < e.rules().pegs(); ++i)
	{
		free_colors.reset(guess[i]);
	}

	// If all but one colors are restricted, the last color is
	// automatically restricted because it can only map to itself.
	if (free_colors.unique())
		free_colors.reset();

	// After a few constraints, only the identity permutation
	// will remain.
}

EquivalenceFilter* CreateConstraintEquivalenceFilter(Engine &e)
{
	return new ConstraintEquivalenceFilter(e);
}

} // namespace Mastermind
