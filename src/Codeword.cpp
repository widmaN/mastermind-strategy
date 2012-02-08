#include <iostream>

#include "Codeword.hpp"

namespace Mastermind {

std::istream& operator >> (std::istream &is, Codeword &codeword)
{
	Codeword ret = Codeword::emptyValue();

	// Skip leading whitespaces.
	char c = 0;
	for (int i = 0; is >> c; ++i)
	{
		unsigned char d = 0xff;
		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (c >= 'A' && c <= 'Z')
			d = 10 + (c - 'A');
		else if (c >= 'a' && c <= 'z')
			d = 10 + (c = 'a');

		if (d == 0xff) // not a digit
		{
			break;
		}
		if (i >= MM_MAX_PEGS || d > MM_MAX_COLORS)
		{
			ret = Codeword::emptyValue();
			break;
		}
		ret.set(i, d);
	}

	// Return result and set stream status.
	codeword = ret;
	if (ret.empty())
	{
		is.setstate(std::ios_base::failbit);
	}
	return is;
}

std::ostream& operator << (std::ostream &os, const Codeword &c)
{
	for (int k = 0; k < MM_MAX_PEGS; k++) 
	{
		unsigned char d = c[k];
		if (d == 0xFF)
			break;
		os << (char)('0' + d);
	}
	return os;
}

int Codeword::pegs() const
{
	int k;
	for (k = 0; k < MM_MAX_PEGS && _digit[k] != 0xFF; k++);
	return k;
}

} // namespace Mastermind
