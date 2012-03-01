#ifndef MASTERMIND_RULES_HPP
#define MASTERMIND_RULES_HPP

#include <cstddef>

#ifndef MM_MAX_PEGS
/// The maximum number of pegs supported by the program.
/// For performance reasons, certain data structures or algorithms
/// pose a limit on the maximum number of pegs allowed in a codeword.
/// If this constant is defined higher than the limit, the program
/// will produce a compile-time error.
#define MM_MAX_PEGS 6
#endif

#ifndef MM_MAX_COLORS
/// The maximum number of colors supported by the program.
/// For performance reasons, certain data structures or algorithms
/// pose a limit on the maximum number of colors allowed in a codeword.
/// If this constant is defined higher than the limit, the program
/// will produce a compile-time error.
#define MM_MAX_COLORS 10
#endif

#if (MM_MAX_PEGS + MM_MAX_COLORS) != 16
# error MM_MAX_PEGS and MM_MAX_COLORS must add to 16.
#endif

namespace Mastermind {

/// Defines the rules that a codeword conforms to.
class Rules
{
	int _pegs;        // number of pegs.
	int _colors;      // number of colors.
	bool _repeatable; // whether the same color can appear more than once.

public:

	/// Constructs a set of rules for a codeword.
	Rules(int pegs, int colors, bool repeatable)
		: _pegs(pegs), _colors(colors), _repeatable(repeatable) { }

	/// Constructs a set of empty rules.
	Rules() 	: _pegs(0), _colors(0), _repeatable(false) { }

	/// Returns the number of pegs.
	int pegs() const { return _pegs; }

	/// Returns the number of colors.
	int colors() const { return _colors; }

	/// Returns whether the same color can appear more than once.
	bool repeatable() const { return _repeatable; }

	/// Checkes whether this set of rules is empty.
	bool empty() const
	{
		return (_pegs == 0) && (_colors == 0) && (_repeatable == false);
	}

	/// Checks whether this set of rules is valid.
	bool valid() const
	{
		return (_pegs > 0 && _pegs <= MM_MAX_PEGS)
			&& (_colors > 0 && _colors <= MM_MAX_COLORS)
			&& (_repeatable || _colors >= _pegs);
	}

	/// Gets the number of codewords conforming to this set of rules.
	size_t size() const
	{
		if (!valid())
			return 0;

		if (repeatable())
		{
			size_t n = 1;
			for (int i = 0; i < _pegs; ++i)
				n *= (size_t)_colors;
			return n;
		}
		else
		{
			size_t n = 1;
			for (int i = _colors; i > _colors - _pegs; --i)
				n *= (size_t)i;
			return n;
		}
	}
};

} // namespace Mastermind

#endif // MASTERMIND_RULES_HPP
