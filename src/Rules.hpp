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

#if MM_MAX_PEGS > 9
#error MM_MAX_PEGS must be smaller than or equal to 9.
#endif

#ifndef MM_MAX_COLORS
/// The maximum number of colors supported by the program.
/// For performance reasons, certain data structures or algorithms
/// pose a limit on the maximum number of colors allowed in a codeword.
/// If this constant is defined higher than the limit, the program
/// will produce a compile-time error.
#define MM_MAX_COLORS 10
#endif

#if MM_MAX_COLORS > 10
#error MM_MAX_COLORS must be smaller than or equal to 10.
#endif

#if (MM_MAX_PEGS + MM_MAX_COLORS) != 16
#error MM_MAX_PEGS and MM_MAX_COLORS must add to 16.
#endif

namespace Mastermind {

/// Defines the rules that a codeword conforms to.
class Rules
{
	int _pegs;        // number of pegs.
	int _colors;      // number of colors.
	bool _repeatable; // whether the same color can appear more than once.

	/// Checks whether a set of rules is valid.
	static bool valid(int pegs, int colors, bool repeatable)
	{
		return (pegs > 0 && pegs <= MM_MAX_PEGS)
			&& (colors > 0 && colors <= MM_MAX_COLORS)
			&& (repeatable || colors >= pegs);
	}

public:

	/// Constructs an empty (invalid) set of rules.
	Rules() 	: _pegs(0), _colors(0), _repeatable(false) { }

	/// Constructs a set of rules. If the input is invalid, an empty 
	/// set of rules is constructed.
	Rules(int pegs, int colors, bool repeatable)
		: _pegs(0), _colors(0), _repeatable(false)
	{
		if (valid(pegs, colors, repeatable))
		{
			_pegs = pegs;
			_colors = colors;
			_repeatable = repeatable;
		}
	}

	/// Constructs a set of rules from a string of the form "p4c6r" 
	/// or "p4c10n". If the input string is invalid, an empty set
	/// of rules is constructed.
	explicit Rules(const char *s) : _pegs(0), _colors(0), _repeatable(false)
	{
		if ((s[0] == 'p' || s[0] == 'P') &&
			(s[1] >= '1' && s[1] <= '0' + MM_MAX_PEGS) &&
			(s[2] == 'c' || s[2] == 'C'))
		{
			int pegs = s[1] - '0';
			if (	(s[3] >= '1' && s[3] <= '0' + MM_MAX_COLORS) &&
				(s[4] == 'r' || s[4] == 'n' || s[4] == 'R' || s[4] == 'N') &&
				(s[5] == '\0'))
			{
				int colors = s[3] - '0';
				bool repeatable = (s[4] == 'r' || s[4] == 'R');
				*this = Rules(pegs, colors, repeatable);
			}
			else if ((MM_MAX_COLORS >= 10) && (s[3] == '1') &&
				(s[4] >= '0' && s[4] <= '0' + MM_MAX_COLORS - 10) &&
				(s[5] == 'r' || s[5] == 'n' || s[5] == 'R' || s[5] == 'N') &&
				(s[6] == '\0'))
			{
				int colors = s[4] - '0' + 10;
				bool repeatable = (s[5] == 'r' || s[5] == 'R');
				*this = Rules(pegs, colors, repeatable);
			}
		}
	}

	/// Returns the number of pegs.
	int pegs() const { return _pegs; }

	/// Returns the number of colors.
	int colors() const { return _colors; }

	/// Returns whether the same color can appear more than once.
	bool repeatable() const { return _repeatable; }

	/// Checkes whether this set of rules is valid (i.e. non-empty).
	bool valid() const
	{
		return _pegs > 0;
	}

#if 0
	/// Checks whether this set of rules is valid.
	bool valid() const
	{
		return (_pegs > 0 && _pegs <= MM_MAX_PEGS)
			&& (_colors > 0 && _colors <= MM_MAX_COLORS)
			&& (_repeatable || _colors >= _pegs);
	}
#endif

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
