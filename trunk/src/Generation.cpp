#include "Algorithm.hpp"

// Returns n^r
static size_t NPower(int n, int r)
{
	size_t p = 1;
    for (int k = 1; k <= r; k++) {
        p *= n;
    }
    return p;
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

////////////////////////////////////////////////////////////////////////////
// Codeword generation routines.

using namespace Mastermind;

static void generate_recursion(
	int npegs, int ncolors, int max_repeat,
	int peg, const Codeword &_partial, Codeword* &output)
{
	Codeword partial(_partial);
	for (int k = 0; k < ncolors; ++k)
	{
		if (partial.count(k) < max_repeat)
		{
			partial.set(peg, k);
			if (peg == npegs - 1)
				*(output++) = partial;
			else
				generate_recursion(npegs, ncolors, max_repeat, peg+1, partial, output);
		}
	}
}

static size_t generate_codewords(
	const Rules &rules,
	Codeword *results)
{
	assert(rules.valid());

	const int colors = rules.colors(), pegs = rules.pegs();
	const size_t count = rules.repeatable()?
		NPower(colors, pegs) : NPermute(colors, pegs);

	if (results)
	{
		generate_recursion(pegs, colors, rules.repeatable()? pegs : 1,
			0, Codeword(), results);
	}
	return count;
}

REGISTER_ROUTINE(GenerationRoutine, "generic", generate_codewords)
