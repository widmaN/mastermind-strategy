#ifndef MASTERMIND_RULES_HPP
#define MASTERMIND_RULES_HPP

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

// Whether to compile call counter.
// Note that this option should not be here. Instead, it should
// go into the -D compiler option. It is here only temporarily.
#define ENABLE_CALL_COUNTER 0

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

} // namespace Mastermind

#endif // MASTERMIND_RULES_HPP
