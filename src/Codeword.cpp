#include <iostream>
#include <algorithm>

#include "Codeword.hpp"

namespace Mastermind {

static const char *codeword_alphabet = "1234567890abcdef";

std::istream& operator >> (std::istream &is, Codeword &codeword)
{
	Codeword ret;
	const char *first = codeword_alphabet;
	const char *last = first + 16;

	// Read first character, skipping leading whitespaces.
	char c = 0;
	if (!(is >> c))
		return is;

	// Process characters.
	int i = 0;
	bool ok = true;
	do
	{
		const char *p = std::find(first, last, c);
		if (p == last) // invalid input
		{
			// Unget character if this is not the first character.
			if (i > 0)
				is.unget();
			break;
		}
		if (i < MM_MAX_PEGS && (p - first) < MM_MAX_COLORS)
		{
			ret.set(i, (int)(p - first));
		}
		else
		{
			ok = false;
		}
		++i;
	} while ((c = (char)is.get()) != EOF);

	// Set stream status and return result.
	if (ok && !ret.empty())
	{
		if (c == EOF)
			is.clear(std::ios_base::eofbit);
		codeword = ret;
	}
	else
	{
		is.setstate(std::ios_base::failbit);
	}
	return is;
}

std::ostream& operator << (std::ostream &os, const Codeword &c)
{
	// Build a string.
	char s[MM_MAX_PEGS + 1] = {0};
	for (int k = 0; k < MM_MAX_PEGS; k++) 
	{
		unsigned char d = c[k];
		if (d >= 0x0F)
			break;
		s[k] = codeword_alphabet[d];
	}
	return os << s;
}

int Codeword::pegs() const
{
	int k;
	for (k = 0; k < MM_MAX_PEGS && _digit[k] != 0xFF; k++);
	return k;
}

bool Codeword::valid(const Rules &rules) const
{
	if (!rules.valid())
		return false;

	// Check pegs.
	for (int p = 0; p < rules.pegs(); ++p)
	{
		if (_digit[p] == 0xFF)
			return false;
	}
	for (int p = rules.pegs(); p < MM_MAX_PEGS; ++p)
	{
		if (_digit[p] != 0xFF)
			return false;
	}

	// Check colors.
	if (!rules.repeatable())
	{
		for (int c = 0; c < rules.colors(); ++c)
		{
			if (_counter[c] > 1)
				return false;
		}
	}
	for (int c = rules.colors(); c < MM_MAX_COLORS; ++c)
	{
		if (_counter[c] > 0)
			return false;
	}
	return true;
}

} // namespace Mastermind
