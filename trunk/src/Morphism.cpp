#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <bitset>

#include "Engine.hpp"
#include "Permutation.hpp"
#include "util/aligned_allocator.hpp"
#include "util/intrinsic.hpp"

using namespace Mastermind;

class ConstraintEquivalenceFilter
{
	Engine &e;
	Rules rules;
	std::vector<
		CodewordPermutation,
		util::aligned_allocator<CodewordPermutation,16>
	> pp;

public:

	/// Initializes a constraint equivalence filter.
	ConstraintEquivalenceFilter(Engine &engine)
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

	/// Returns a list of canonical guesses given the current constraints.
	CodewordList get_canonical_guesses() const
	{
		bool verbose = false;

		// Create an array to indicate whether a codeword has been crossed out.
		size_t n = rules.size();
		std::vector<bool> crossed_out(n);
		CodewordIndexer index(e.rules());
		CodewordList canonical;
		CodewordConstIterator all = e.universe().begin();

		// Check each non-crossed codeword in the list.
		for (size_t i = 0; i < n; ++i)
		{
			if (crossed_out[i])
				continue;

			// An uncrossed codeword is a canonical guess.
			const Codeword guess = all[i];
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

				// Find the colors in the guess that are not mapped in
				// the partial color mapping. We can freely choose the
				// colors to map these colors to.
				int nfree = 0;
				int ifree[MM_MAX_PEGS];
				for (int i = 0; i < rules.colors(); ++i)
				{
					if (guess.count(i) && p.color[i] < 0)
						ifree[nfree++] = i;
				}

				// Find the colors that are unmapped to in the partial 
				// mapping. These are the colors that we can map to.
				CodewordPermutation pinv = p.inverse();
				int nunmapped = 0;
				int iunmapped[MM_MAX_COLORS];
				for (int i = 0; i < rules.colors(); ++i)
				{
					if (pinv.color[i] < 0)
						iunmapped[nunmapped++] = i;
				}

				assert(nunmapped >= nfree);

				// Iterate all possible color mappings in the free slots,
				// and cross out every resulting codeword.
				int tmp[MM_MAX_PEGS];
				generate_permutations<MM_MAX_COLORS>(
					iunmapped+0, iunmapped+nunmapped, tmp+0, tmp+nfree, [&]()
				{
					// Extend the partial color mapping to a complete mapping.
					for (int i = 0; i < nfree; i++)
						p.color[ifree[i]] = tmp[i];

					//Codeword mapped = permute_colors(rules, cc, partial);
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
		return canonical;
	}

	/// Adds a constraint. Due to the nature of this filter, only 
	/// the guess matters in the contraint. The response is ignored.
	void add_constraint(const Codeword &guess)
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
};

void display_canonical_guesses(
	const ConstraintEquivalenceFilter &filter, 
	int level, int max_level)
{
	CodewordList canonical = filter.get_canonical_guesses();

	// Display each canonical guess, and expand one more level is needed.
	if (level == max_level)
	{
		std::cout << "[" << level << ":" << canonical.size() << "]";
#if 1
		if (canonical.size() > 20)
		{
			std::cout << " ... " << std::endl;
		}
		else
#endif
		{
			for (size_t i = 0; i < canonical.size(); ++i)
			{
				Codeword guess = canonical[i];
				std::cout << " " << guess;
			}
			std::cout << std::endl;
		}
	}
	else
	{
		for (size_t i = 0; i < canonical.size(); ++i)
		{
			Codeword guess = canonical[i];
			std::cout << "[" << level << ":" << i << "] " << guess << std::endl;

			ConstraintEquivalenceFilter child(filter);
			child.add_constraint(guess);
			display_canonical_guesses(child, level+1, max_level);
			//std::cout << "[" << level << "] Total: " << canonical.size() << std::endl;
		}
	}
}

void test_morphism(Engine &e)
{
	ConstraintEquivalenceFilter filter(e);
	display_canonical_guesses(filter, 0, 1);
}
