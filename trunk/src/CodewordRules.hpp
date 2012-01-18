#ifndef MASTERMIND_CODEWORD_RULES_HPP
#define MASTERMIND_CODEWORD_RULES_HPP

namespace Mastermind {

/// Defines the rules that a codeword conforms to.
class CodewordRules
{
	int _pegs;        // number of pegs.
	int _colors;      // number of colors.
	bool _repeatable; // whether the same color can appear more than once.

public:
	
	/// Constructs a set of rules for a codeword.
	CodewordRules(int pegs, int colors, bool repeatable)
		: _pegs(pegs), _colors(colors), _repeatable(repeatable) { }

	/// Constructs a set of empty rules.
	CodewordRules() 	: _pegs(0), _colors(0), _repeatable(false) { }

	/// Returns the number of pegs.
	int pegs() const { return _pegs; }

	/// Returns the number of colors.
	int colors() const { return _colors; }

	/// Returns whether the same color can appear more than once.
	bool repeatable() const { return _repeatable; }

	/// Checks whether this set of rules is valid.
	bool valid() const 
	{
#if 0
		return (length > 0 && length <= MM_MAX_PEGS) 
			&& (ndigits > 0 && ndigits <= MM_MAX_COLORS) 
			&& (allow_repetition || ndigits >= length);
#else
		return true;
#endif
	}

#if 0
	unsigned short GetFullDigitMask() const
	{
		return (1 << ndigits) - 1;
	}
#endif
};

} // namespace mastermind

#endif // MASTERMIND_CODEWORD_RULES_HPP
