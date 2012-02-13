#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <bitset>

#include "Engine.hpp"

using namespace Mastermind;

template <class T, T MaxSize>
struct permutation
{
	T _perm[MaxSize];
	T _size;

	permutation(T size) : _size(size)
	{
		for (T i = 0; i < _size; i++)
			_perm[i] = i;
	}

	permutation(T size, T init_value) : _size(size)
	{
		for (T i = 0; i < _size; i++)
			_perm[i] = init_value;
	}

	T* begin() { return _perm; }
	T* end() { return _perm + _size; }

	T operator[](size_t index) const { return _perm[index]; }
	T& operator[](size_t index) { return _perm[index]; }
};

typedef permutation<char,MM_MAX_PEGS> peg_perm_t;
typedef permutation<char,MM_MAX_COLORS> color_perm_t;

struct codeword_permutation
{
	peg_perm_t peg_perm;
	color_perm_t color_perm;
	codeword_permutation(const peg_perm_t &pp, const color_perm_t &pc)
		: peg_perm(pp), color_perm(pc) { }
};

// Generates all permutations of size n.
template <class T, T MaxSize>
std::vector<permutation<T,MaxSize>> 
generate_permutations(unsigned char size)
{
	std::vector<permutation<T,MaxSize>> ret;
	permutation<T,MaxSize> p(size);
	do 
	{
		ret.push_back(p);
	} while (std::next_permutation(p._perm, p._perm + size));
	return ret;
}

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

// Returns n!/r!
static size_t NPermute(int n, int r)
{
	assert(n >= r);
	if (n < r)
		return 0;

	size_t p = 1;
	for (int k = n - r + 1; k <= n; k++) {
		p *= k;
	}
	return p;
}

static int get_codeword_index(const Codeword &c, const Rules &rules)
{
	if (rules.repeatable())
	{
		int index = 0;
		int t = 1;
		for (int i = rules.pegs() - 1; i >= 0; --i, t *= rules.colors())
			index += c[i] * t;
		return index;
	}
	else
	{
		std::bitset<MM_MAX_COLORS> bitmask;
		int index = 0;
		for (int i = 0; i < rules.pegs(); ++i)
		{
			int t = c[i] - (bitmask & std::bitset<MM_MAX_COLORS>((1 << c[i]) - 1)).count();
			index += t*NPermute(rules.colors()-1-i, rules.pegs()-1-i);
			bitmask.set(c[i]);
		}
		return index;
	}
}

static CodewordList get_canonical_guesses(
	Engine &e,
	std::vector<codeword_permutation> &perms)
{
	// Generate all permutations of p pegs and c colors.
	Rules rules = e.rules();

	// Create an array to indicate whether a codeword has been crossed out.
	std::vector<bool> crossed_out(e.rules().size());
	CodewordList canonical;

	// Check each codeword in turn.
	CodewordConstIterator all = e.universe().begin();
	for (size_t i = 0; i < crossed_out.size(); ++i)
	{
		//std::cout << "Testing codeword " << all[i] << std::endl;

		// Skip if this codeword is crossed out.
		if (crossed_out[i])
			continue;

		Codeword c = all[i];
		canonical.push_back(c);

		// Cross out all codewords that are a permutation of this codeword.
		for (size_t j = 0; j < perms.size(); ++j)
		{
			Codeword d = permute_codeword(e.rules(), c, perms[j]);
			crossed_out[get_codeword_index(d, e.rules())] = true;
		}
	}

	// Display canonical codewords.
	for (size_t i = 0; i < canonical.size(); ++i)
		std::cout << canonical[i] << std::endl;

	return canonical;
}

void test_morphism_1(Engine &e)
{
	// Generate all permutations of p pegs and c colors.
	int p = e.rules().pegs();
	int c = e.rules().colors();

	// Generate all permutations of the pegs and colors.
	std::vector<peg_perm_t> peg_perms = 
		generate_permutations<char,MM_MAX_PEGS>(e.rules().pegs());
	std::vector<color_perm_t> color_perms = 
		generate_permutations<char,MM_MAX_COLORS>(e.rules().colors());

	// Create a cross-product of the peg/color permutation.
	std::vector<codeword_permutation> perms;
	for (size_t j = 0; j < peg_perms.size(); ++j)
	{
		for (size_t k = 0; k < color_perms.size(); ++k)
		{
			perms.push_back(codeword_permutation(peg_perms[j], color_perms[k]));
		}
	}

	CodewordList canonical = get_canonical_guesses(e, perms);
}

// Incremental constraint equivalence detection.
/*
Algorithm: since the number of pegs is small but the number of digits
is big, we do a semi-brute-force in the pegs.

That is, for each permutation of pegs, \delta, we associate with it 
a \emph{partial permutation} of the colors, \pi'. A partial permutation 
is an incomplete permutation where the mapping of some colors are 
specified, but the mapping the other colors are not specified (i.e.
they're free to be specified later).

For 4 pegs, there are 24 permutations of pegs. \delta_1 to \delta_24.
Here \delta_1 is the identity permutation.

At the beginning, there are no constraints, and all associated partial 
color permutations \pi'_i are fully unspecified.

We then get the first codeword in lexicographical order. This codeword
must be canonical for some equivalence class. Then, we iterate through
each peg permutation \delta_i. Suppose we're at the first permutation,
\delta_1 = (1 2 3 4). The associated partial color permutation is
(1 2 3 4 5 6 7 8 9 0)
(* * * * * * * * * *)
For the concerned range, 1234, we are free to map it to any color.
We can therefore do a simple iteration to specialize the partial 
permutation in P(10,4)=5040 ways. As a result we get 5040 codewords.
We cross them out.

We then proceed with the second peg permutation, \delta_2 = (1 2 4 3).
The resulting codeword is now 1243, but this has already been crossed
out in the above step. Therefore we skip it. Similarly, we skip all
codewords in this round.

Now suppose we have already have some constraint. Due to the nature of
constraint equivalence, the response doesn't matter; only the guess
matters.

Suppose without loss of generality that the first guess is 3456. 
To canonicalize the second guess,
we first need to restrict the partial color permutations associated
with each peg permutation. Take for example \delta_3 = (1324).
The permuted constraint is 3546. In order for this to be equal to 
the original, we must restrict the associated color permutation as
(1 2 3 4 5 6 7 8 9 0) <- original
(* * 3 5 4 6 * * * *) <- permuted

Now suppose we come again to 1234. If we apply peg permutation \delta_3,
it becomes 1324. Now to find out what color permutations are eligible,
note that 3 and 4 are already fixed; only 1 and 2 are free to be mapped.
So we can map it freely to {1,2,7,8,9,0}. 

As we proceed with this, we note that the partial color permutation
associated with each peg permutation gets more restrictive with each
added constraint. Hence we call it "incremental equivalence detection".

*/

void incremental_equivalence_detection()
{

}

template <size_t MaxSize, class Iter1, class Iter2, class Func>
void iterate_permutations_recursion(
	Iter1 from, Iter2 to, size_t n, size_t r, size_t level,
	std::bitset<MaxSize> &mask, Func f)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (!mask[i])
		{
			if (level == r - 1)
			{
				to[level] = from[i];
				f();
			}
			else
			{
				mask[i] = true;
				to[level] = from[i];
				iterate_permutations_recursion(from, to, n, r, level+1, mask, f);
				mask[i] = false;
			}
		}
	}
}

template <size_t MaxSize, class Iter1, class Iter2, class Func>
void iterate_permutations(
	Iter1 from_first, Iter1 from_last, 
	Iter2 to_first, Iter2 to_last,
	Func f)
{
	size_t n = from_last - from_first;
	size_t r = to_last - to_first;
	assert(n >= r && r >= 0 && MaxSize >= n);

	if (r > 0)
	{
		std::bitset<MaxSize> mask;
		iterate_permutations_recursion(from_first, to_first, n, r, 0, mask, f);
	}
	else
	{
		assert(0);
	}
}

void test_morphism_2(Engine &e)
{
	Rules rules = e.rules();
	bool verbose = false;

	// Generate all peg permutations, and associate with each peg 
	// permutation a fully-free partial color permutation.
	std::vector<std::pair<peg_perm_t, color_perm_t>> pp;
	peg_perm_t peg_perm(rules.pegs());
	do
	{
		pp.push_back(std::make_pair(peg_perm, color_perm_t(rules.colors(), -1)));
	} 
	while (std::next_permutation(peg_perm.begin(), peg_perm.end()));

	if (verbose)
		std::cout << "There are " << pp.size() << " peg permutations." << std::endl;

	// Create an array to indicate whether a codeword has been crossed out.
	size_t n = rules.size();
	std::vector<bool> crossed_out(n);
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

		// Check each peg permutation.
		for (size_t j = 0; j < pp.size(); ++j)
		{
			// Proceed only if this peg permutation maintains all the
			// existing guesses unchanged.
			// ...

			// Permute the pegs in the codeword.
			Codeword cc = permute_pegs(c, pp[j].first);
			if (crossed_out[get_codeword_index(cc, rules)])
				continue;

#if 0
			// Find free colors to be mapped from and mapped to in the
			// partial color permutation.
			const color_perm_t &partial = pp[j].second;
			std::bitset<MM_MAX_COLORS> free_from(true), free_to(true);
			for (int i = 0; i < rules.colors(); ++i)
			{
				if (partial[i] >= 0)
				{
					free_from.reset(i);
					free_to.reset(partial[i]);
				}
			}
#endif

			// Find the colors in the codeword which can be permuted.
			// Such colors are not fixed in the partial color mapping.
			color_perm_t partial = pp[j].second;
			int nfree = 0;
			int ifree[MM_MAX_PEGS];
			for (int i = 0; i < rules.pegs(); ++i)
			{
				if (partial[cc[i]] == -1 && 
					std::find(ifree,ifree+nfree,cc[i]) == ifree+nfree)
				{
					ifree[nfree++] = cc[i];
				}
			}
			if (nfree == 0)
			{
				if (verbose)
					std::cout << "    Crossed out " << cc << std::endl;
				crossed_out[get_codeword_index(cc, rules)] = true;
				continue;
			}

			// Find the colors that are unmapped to in the partial mapping.
			int nunmapped = 0;
			int iunmapped[MM_MAX_COLORS];
			for (int i = 0; i < rules.colors(); ++i)
			{
				if (partial[i] == -1)
					iunmapped[nunmapped++] = i;
			}

			// Iterate all possible color mappings in the free slots,
			// and cross out every resulting codeword.
			int tmp[MM_MAX_PEGS];
			int tt = 1;
			iterate_permutations<MM_MAX_COLORS>(
				iunmapped+0, iunmapped+nunmapped, tmp+0, tmp+nfree, [&]()
			{
				for (int i = 0; i < nfree; i++)
					partial[ifree[i]] = tmp[i];
				Codeword mapped = permute_colors(rules, cc, partial);
				if (!crossed_out[get_codeword_index(mapped, rules)])
				{
					crossed_out[get_codeword_index(mapped, rules)] = true;
					if (verbose)
						std::cout << "    Crossed out " << mapped << std::endl;
				}
			});
		}
	}

	// Display the remaining canonical codewords.
	std::cout << "Canonical codewords:";
	for (size_t i = 0; i < canonical.size(); ++i)
		std::cout << " " << canonical[i];
	std::cout << std::endl << "Total: " << canonical.size() << std::endl;
}

void test_morphism(Engine &e)
{
	test_morphism_2(e);
}
