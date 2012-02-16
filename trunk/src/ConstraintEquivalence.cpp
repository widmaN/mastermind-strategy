#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <bitset>

#include "Engine.hpp"
#include "Permutation.hpp"
#include "util/intrinsic.hpp"
#include "Equivalence.hpp"
#include "util/call_counter.hpp"

REGISTER_CALL_COUNTER(ConstraintEquivalence)
// REGISTER_CALL_COUNTER(ConstraintEquivalenceIdentity)
REGISTER_CALL_COUNTER(ConstraintEquivalencePermutation)
REGISTER_CALL_COUNTER(ConstraintEquivalenceCrossout)

using namespace Mastermind;

/// Initializes a constraint equivalence filter.
ConstraintEquivalenceFilter::ConstraintEquivalenceFilter(Engine &engine)
	: e(engine), rules(e.rules())
{
	// Generate all peg permutations, and associate with each peg 
	// permutation a fully-free partial color permutation.

	// Note: the peg permutations stored are in fact the inverse of 
	// the traditional permutations. But since we generate all such
	// permutations, we don't need to explicitly compute the inverse.

	CodewordPermutation p(rules);
	for (int i = 0; i < rules.pegs(); ++i)
		p.peg[i] = (char)i;

	do
	{
		pp.push_back(p);
	} 
	while (std::next_permutation(p.peg + 0, p.peg + rules.pegs()));
}

// Returns a list of canonical guesses given the current constraints.
CodewordList ConstraintEquivalenceFilter::get_canonical_guesses(
	CodewordConstRange candidates) const
{
	bool verbose = false;

	// Let u be the number of colors fixed so far.
	int u = 0;
	for (u = 0; u < rules.colors(); ++u)
	{
		if (pp[0].color[u] < 0)
			break;
	}

#if 1
	// Optimization: if there is only one peg permutation left (in
	// which case it must be the identity permutation) and the color
	// permutation is "almost completely" specified, then the color
	// permutation must be identity too, and there is no codeword
	// to filter out.
	if (pp.size() == 1 && u >= rules.colors() - 1)
		return CodewordList(candidates.begin(), candidates.end());
#endif

	// Check each non-crossed codeword in the list.
	size_t n = candidates.size();
	CodewordList canonical;
	canonical.reserve(n);
	for (size_t i = 0; i < n; ++i)
	{
		const Codeword candidate = candidates.begin()[i];
		bool is_canonical = true;

#if 0
		bool debug_stop = false;
		if (Codeword::pack(guess) == 0xffff0234)
			debug_stop = true;
#endif

		// Check each peg permutation to see if there exists a peg/color
		// permutation that maps the candidate to a lexicographically
		// smaller equivalent codeword.
		for (size_t j = 0; j < pp.size(); ++j)
		{
#if 0
			if (debug_stop && j == 3)
				debug_stop = true;
#endif

			// Get a bit-mask of the unmapped colors in the permutation.
			CodewordPermutation p = pp[j];

			// Permute the pegs and colors of the candidate.
			// Unmapped colors are kept unchanged.
			// Optimization: if this is the identity permutation,
			// then needn't permute.
			Codeword permuted_candidate = 
				(j == 0)? candidate : p.permute(candidate);

			// Check the color on each peg in turn.
			// Take, for example, 1223. It must be able to map to 1123 and
			// show that it's not canonical.
			int free_color = u;
			for (int k = 0; k < rules.pegs(); ++k)
			{
				// Let c be the color on peg k of the permuted candidate.
				int c = permuted_candidate[k];

				// If c is not mapped, find the smallest, unmapped color
				// c', and swap c with c' in the permuted candidate.
				if (p.color[c] < 0)
				{
					// Find the smallest unmapped color.
#if 0
					int cc = -1;
					for (int l = 0; l < rules.colors(); ++l)
					{
						if (p.color[l] < 0) 
						{
							cc = l;
							break;
						}
					}
#else
					int cc = free_color++;
#endif

					for (int l = k; l < rules.pegs(); ++l)
					{
						if (permuted_candidate[l] == c)
							permuted_candidate.set(l, cc);
						else if (permuted_candidate[l] == cc)
							permuted_candidate.set(l, c);
					}
					p.color[cc] = cc;
					c = cc;
				}

				// The color, c, must be lexicographically greater than
				// or equal to the corresponding color in the candidate
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

#if 1
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
		Codeword c = p.permute_pegs(guess);

		// Try to map the color on each peg onto itself.
		// @bug The following code is not correct in some cases;
		// see the modified version for the correct one.
		bool ok = true;
		for (int j = 0; j < rules.pegs() && ok; ++j)
		{
			if (p.color[c[j]] == -1)
				p.color[c[j]] = guess[j];
			else if (p.color[c[j]] != guess[j])
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

	// After a few constraints, only the identity permutation
	// will remain.
}
