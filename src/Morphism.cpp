#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <bitset>

#include "Engine.hpp"
#include "Permutation.hpp"
#include "util/intrinsic.hpp"

using namespace Mastermind;

typedef permutation<char,MM_MAX_PEGS> peg_perm_t;
typedef permutation<char,MM_MAX_COLORS> color_perm_t;

#if 0
static Codeword permute_codeword(
	const Rules &rules,
	const Codeword &c,
	const codeword_permutation &p)
{
	Codeword d;
	for (int i = 0; i < rules.pegs(); ++i)
		d.set(i, p.color_perm._perm[c[p.peg_perm._perm[i]]]);
	return d;
}
#endif

static Codeword permute_pegs(const Codeword &c, const peg_perm_t &p)
{
	Codeword d;
	for (int i = 0; i < p._size; ++i)
		d.set(i, c[p._perm[i]]);
	return d;
}

static Codeword permute_colors(const Rules &rules, const Codeword &c, const color_perm_t &p)
{
	Codeword d;
	for (int i = 0; i < rules.pegs(); ++i)
		d.set(i, p[c[i]]);
	return d;
}

/// Utility class that computes the lexicographical index of a codeword.
class CodewordIndexer
{
	Rules _rules;
	int _weights[MM_MAX_COLORS];

public:

	CodewordIndexer(const Rules &rules) : _rules(rules)
	{
		if (_rules.repeatable())
		{
			int w = 1;
			for (int i = _rules.pegs() - 1; i >= 0; --i)
			{
				_weights[i] = w;
				w *= _rules.colors();
			}
		}
		else
		{
			int w = 1;
			for (int i = _rules.pegs() - 1; i >= 0; --i)
			{
				_weights[i] = w;
				w *= (_rules.colors() - i);
			}
		}
	}

	int operator()(const Codeword &c) const 
	{
		if (_rules.repeatable())
		{
			int index = 0;
			for (int i = 0; i < _rules.pegs(); ++i)
				index += c[i] * _weights[i];
			return index;
		}
		else
		{
			unsigned short bitmask = 0;
			int index = 0;
			for (int i = 0; i < _rules.pegs(); ++i)
			{
				int t = c[i] - util::intrinsic::pop_count(bitmask & ((1 << c[i]) - 1));
				index += t * _weights[i];
				bitmask |= (1 << c[i]);
			}
			return index;
		}
	}
};

class ConstraintEquivalenceFilter
{
	Engine &e;
	Rules rules;
	std::vector<std::pair<peg_perm_t, color_perm_t>> pp;

public:

	/// Initializes an incremental equivalence detector.
	ConstraintEquivalenceFilter(Engine &engine)
		: e(engine), rules(e.rules())
	{
		// Generate all peg permutations, and associate with each peg 
		// permutation a fully-free partial color permutation.
		peg_perm_t peg_perm(rules.pegs());
		do
		{
			//pp.push_back(std::make_pair(peg_perm, color_perm_t(rules.colors(), -1)));
			pp.push_back(std::pair<peg_perm_t, color_perm_t>(
				peg_perm, color_perm_t(rules.colors(), -1)));
		} 
		while (std::next_permutation(peg_perm.begin(), peg_perm.end()));
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

			Codeword c = all[i];
			canonical.push_back(c);
			if (verbose)
				std::cout << "Processing canonical codeword " << c << std::endl;

#ifndef NDEBUG
			bool debug_stop = false;
			if (Codeword::pack(c) == 0xffff0234)
				debug_stop = true;
#endif

			// Check each peg permutation.
			for (size_t j = 0; j < pp.size(); ++j)
			{
#ifndef NDEBUG
				if (debug_stop && j == 3)
					debug_stop = true;
#endif

				// Permute the pegs in the codeword.
				Codeword cc = permute_pegs(c, pp[j].first);

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

				// Find the colors in the codeword that are not fixed in
				// the partial color mapping. We can freely choose the
				// colors to map these colors to.
				color_perm_t partial = pp[j].second;
				int nfree = 0;
				int ifree[MM_MAX_PEGS];
				for (int i = 0; i < rules.colors(); ++i)
				{
					if (cc.count(i) && partial[i] == -1)
						ifree[nfree++] = i;
				}

				// Find the colors that are unmapped to in the partial 
				// mapping. These are the colors that we can map to.
				// BUG: this is wrong; instead of finding the colors
				// unmapped TO, it finds the colors unmapped FROM.
				int nunmapped = 0;
				int iunmapped[MM_MAX_COLORS];
				for (int i = 0; i < rules.colors(); ++i)
				{
					if (partial[i] == -1)
						iunmapped[nunmapped++] = i;
				}

				assert(nunmapped >= nfree);

				// Iterate all possible color mappings in the free slots,
				// and cross out every resulting codeword.
				int tmp[MM_MAX_PEGS];
				generate_permutations<MM_MAX_COLORS>(
					iunmapped+0, iunmapped+nunmapped, tmp+0, tmp+nfree, [&]()
				{
					for (int i = 0; i < nfree; i++)
						partial[ifree[i]] = tmp[i];
					Codeword mapped = permute_colors(rules, cc, partial);
					crossed_out[index(mapped)] = true;
					if (/* Codeword::pack(mapped) == 0xffff3456 || */ verbose)
					{
						std::cout << "    Crossed out " << mapped 
							<< " from " << cc << std::endl;
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

			// Permute the pegs in the guess.
			Codeword c = permute_pegs(guess, pp[i].first);

			// Try to map the color on each peg onto itself.
			color_perm_t &partial = pp[i].second;
			bool ok = true;
			for (int j = 0; j < rules.pegs() && ok; ++j)
			{
				if (partial[c[j]] == -1)
					partial[c[j]] = guess[j];
				else if (partial[c[j]] != guess[j])
					ok = false;
			}

			// Remove the peg permutation if no color permutation exists
			// that maps the guess onto itself.
			if (!ok)
			{
				if (verbose)
					std::cout << "Removed peg permutation: " << pp[i].first << std::endl;
				std::swap(pp[i], pp[pp.size()-1]);
				pp.resize(pp.size()-1);
			}
			else
			{
				if (verbose)
					std::cout << "Restricted color mapping for peg perm "
						<< pp[i].first << ": " << pp[i].second << std::endl;
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
