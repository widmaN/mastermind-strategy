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

	// Note: the peg permutations are in fact inverse of the 
	// traditional permutations. But since we generate all such
	// permutations, we don't need to explicitly compute the
	// inverse.

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

	// Optimization: if there is only one peg permutation left (in
	// which case it must be the identity permutation) and the color
	// permutation is "almost completely" specified, then the color
	// permutation must be identity too, and there is no codeword
	// to filter out.
	if (pp.size() == 1 && pp[0].almost_complete())
		return CodewordList(candidates.begin(), candidates.end());

	// Create an array to indicate whether a codeword has been crossed out.
	CodewordIndexer index(e.rules());
	std::vector<bool> crossed_out(index.size());

	// Check each non-crossed codeword in the list.
	size_t n = candidates.size();
	CodewordList canonical;
	for (size_t i = 0; i < n; ++i)
	{
		const Codeword guess = candidates.begin()[i];
		if (crossed_out[index(guess)])
			continue;

		// An uncrossed codeword is a canonical guess.
		canonical.push_back(guess);
		if (verbose)
			std::cout << "Processing canonical guess " << guess << std::endl;

#ifndef NDEBUG
		bool debug_stop = false;
		if (Codeword::pack(guess) == 0xffff0234)
			debug_stop = true;
#endif

		// Check each peg permutation.
		for (size_t j = 0; j < pp.size(); ++j)
		{
#ifndef NDEBUG
			if (debug_stop && j == 3)
				debug_stop = true;
#endif

#if 0
			// Note: we should not cross out too early. This is because
			// when we cross out a codeword, we are only filtering on
			// the equivalence _given_ a certain peg permutation. It is
			// not exhaustive.
			//if (crossed_out[get_codeword_index(cc, rules)])
			//	continue;

			// Maybe we could traverse the list from larger to smaller,
			// and only cross out lexicographically larger equivalent
			// codewords.

			// Or, we should not skip it if it is crossed out in 
			// this round.
#endif
			CodewordPermutation p = pp[j];

			// Find the unmapped colors in the codeword mapping. 
			// These are essentially "unguessed" colors, i.e. they
			// have never appeared in any constraints so far.
			int nunmapped = 0;
			int iunmapped[MM_MAX_COLORS];

			// If an unmapped color is present in the guess, then
			// we call it "free" because we can map it to any of
			// of the unmapped colors to generate a group of 
			// equivalent codewords.
			int nfree = 0;
			int ifree[MM_MAX_PEGS];

			for (int i = 0; i < rules.colors(); ++i)
			{
				if (p.color[i] < 0)
				{
					iunmapped[nunmapped++] = i;
					if (guess.count(i))
						ifree[nfree++] = i;
				}
			}

			// Generate all possible mappings for the free colors
			// in the guess, and cross out every resulting codeword
			// because they are equivalent to the guess.
			int tmp[MM_MAX_PEGS];
			generate_permutations<MM_MAX_COLORS>(
				iunmapped+0, iunmapped+nunmapped, tmp+0, tmp+nfree, [&]()
			{
				// Extend the partial color mapping to a complete mapping.
				for (int i = 0; i < nfree; i++)
					p.color[ifree[i]] = tmp[i];

				Codeword permuted_guess = p.permute(guess);
				crossed_out[index(permuted_guess)] = true;
				if (/* Codeword::pack(mapped) == 0xffff3456 || */ verbose)
				{
					std::cout << "    Crossed out " << permuted_guess 
						<< " from " << guess << std::endl;
				}
			});
		}
	}

	UPDATE_CALL_COUNTER(ConstraintEquivalence, canonical.size());
	UPDATE_CALL_COUNTER(ConstraintEquivalencePermutation, pp.size());
	UPDATE_CALL_COUNTER(ConstraintEquivalenceCrossout, (n-canonical.size()+1));

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
