#ifndef MASTERMIND_RULES_HPP
#define MASTERMIND_RULES_HPP

#include <iostream>
#include "util/choose.hpp"

#ifndef MM_MAX_PEGS
/// The maximum number of pegs supported by the program.
/// This value must be smaller than or equal to @c 9.
/// @ingroup Rules
#define MM_MAX_PEGS 6
#endif

#if MM_MAX_PEGS > 9
#error MM_MAX_PEGS must be smaller than or equal to 9.
#endif

#ifndef MM_MAX_COLORS
/// The maximum number of colors supported by the program.
/// This value must be smaller than or equal to @c 10.
/// @ingroup Rules
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
/// @ingroup Rules
class Rules
{
	union 
	{
		long _value;          // packed value
		struct 
		{
			char _pegs;       // number of pegs
			char _colors;     // number of colors
			bool _repeatable; // whether the same color can appear more than once
		};
	};

public:

	/// Constructs an empty set of rules.
	Rules() 	: _value(0) { }

	/// Constructs a set of rules with the given parameters.
	/// If the input is invalid, an empty set of rules is constructed.
	Rules(int pegs, int colors, bool repeatable) : _value(0)
	{
		if ((pegs > 0 && pegs <= MM_MAX_PEGS)
			&& (colors > 0 && colors <= MM_MAX_COLORS)
			&& (repeatable || colors >= pegs))
		{
			_pegs = (char)pegs;
			_colors = (char)colors;
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

	/// Tests whether this set of rules is empty.
	bool empty() const { return _value == 0; }

	/// Tests whether this set of rules is non-empty.
	operator void* () const { return empty()? 0 : (void*)this; }

	/// Tests whether this set of rules is empty.
	bool operator ! () const { return empty(); }

	/// Gets the number of codewords conforming to this set of rules.
	size_t size() const
	{
		if (empty())
			return 0;
		else
			return util::choice<size_t>(_colors, _pegs, true, _repeatable);
	}

	/// Returns a packed value representing this set of rules.
	long pack() const { return _value; }

	/// Unpacks a set of rules from a packed value.
	/// No validity check is performed, so the caller is responsible for
	/// ensuring the correctness of the operation.
	static Rules unpack(long value)
	{
		Rules r;
		r._value = value;
		return r;
	}
};

/// @cond DETAILS
namespace details {

struct RulesFormatter
{
	Rules rules;

	RulesFormatter(const Rules &r) : rules(r) { }

	/// Returns the index to a custom ios format field.
	static int index()
	{
		static int i = std::ios_base::xalloc();
		return i;
	}
};

inline std::istream& operator >> (std::istream &s, const RulesFormatter &f)
{
	s.iword(RulesFormatter::index()) = f.rules.pack();
	return s;
}

} // namespace details
/// @endcond

/// Sets the rules to be associated with a stream.
/// @ingroup Rules
inline details::RulesFormatter setrules(const Rules &rules)
{
	return details::RulesFormatter(rules);
}

/// Gets the rules associated with a stream.
/// @ingroup Rules
inline Rules getrules(std::ios_base &s)
{
	return Rules::unpack(s.iword(details::RulesFormatter::index()));
}

} // namespace Mastermind

#endif // MASTERMIND_RULES_HPP
