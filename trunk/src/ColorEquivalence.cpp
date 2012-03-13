#include <numeric>

#include "Algorithm.hpp"
#include "Equivalence.hpp"
#include "util/intrinsic.hpp"

namespace Mastermind {

/// Represents a color equivalence filter.
class ColorEquivalenceFilter : public EquivalenceFilter
{
	Engine &e;
	ColorMask _unguessed;
	ColorMask _excluded;

	CodewordList filter_norep(CodewordConstRange candidates) const;
	CodewordList filter_excluded_norep(CodewordConstRange candidates) const;
	CodewordList filter_rep(CodewordConstRange candidates) const;

public:

	ColorEquivalenceFilter(Engine &engine)
		: e(engine), _unguessed(ColorMask::fill(e.rules().colors()))
	{
	}

	virtual EquivalenceFilter* clone() const
	{
		return new ColorEquivalenceFilter(*this);
	}

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates) const
	{
		if (e.rules().repeatable())
			return filter_rep(candidates);
		else
			//return filter_norep(candidates);
			return filter_excluded_norep(candidates);
	}

	virtual void add_constraint(
		const Codeword & guess,
		Feedback /* response */,
		CodewordConstRange remaining)
	{
		_excluded = ColorMask::fill(e.rules().colors());
		_excluded.reset(e.colorMask(remaining));
		_unguessed.reset(e.colorMask(guess));
		_unguessed.reset(_excluded);
	}
};

CodewordList ColorEquivalenceFilter::filter_rep(
	CodewordConstRange candidates) const
{
	// For codewords with repeated colors, we only apply color equivalence
	// on excluded colors.
	if (_excluded.empty())
		return CodewordList(candidates.begin(), candidates.end());

	int first = _excluded.smallest();

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	CodewordList canonical;
	canonical.reserve(candidates.size());
	for (CodewordConstIterator it = candidates.begin(); it != candidates.end(); ++it)
	{
		Codeword guess = *it;
		bool ok = true;
		for (int j = 0; j < e.rules().pegs(); j++)
		{
			int c = guess[j];
			if (_excluded[c] && c > first)
			{
				ok = false;
				break;
			}
		}
		if (ok)
		{
			canonical.push_back(guess);
		}
	}
	return canonical;
}

// @todo
// 1) clean up the code
// 2) we might use SSE2 to speed up part of the code
CodewordList ColorEquivalenceFilter::filter_excluded_norep(
	CodewordConstRange candidates) const
{
	// For each codeword without repetition, we check the color on each peg
	// in turn. If the color is excluded, it must be the smallest excluded
	// color, otherwise it is not canonical.
	if (_excluded.empty())
		return CodewordList(candidates.begin(), candidates.end());

	// Find out the minimum equivalent codeword of each codeword. If it is
	// equal to the codeword itself, keep it.
	CodewordList canonical;
	canonical.reserve(candidates.size());
	for (CodewordConstIterator it = candidates.begin(); it != candidates.end(); ++it)
	{
		Codeword guess = *it;

#if 0
		ColorMask unguessed = _unguessed;
#endif
		ColorMask excluded = _excluded;
		bool ok = true;

		for (int j = 0; j < e.rules().pegs(); j++)
		{
			int c = guess[j];
			if (excluded[c])
			{
				if ((excluded.value() & ((1 << c) - 1)) != 0)
				{
					ok = false;
					break;
				}
				excluded.reset(c);
			}
#if 0
			if (unguessed[c])
			{
				if ((unguessed.value() & ((1 << c) - 1)) != 0)
				{
					ok = false;
					break;
				}
				unguessed.reset(c);
			}
#endif
		}
		if (ok)
		{
			canonical.push_back(guess);
		}
	}
	return canonical;
}

static EquivalenceFilter* CreateColorEquivalenceFilter(Engine &e)
{
	return new ColorEquivalenceFilter(e);
}

REGISTER_ROUTINE(CreateEquivalenceFilterRoutine,
				 "Color",
				 CreateColorEquivalenceFilter)

} // namespace Mastermind
