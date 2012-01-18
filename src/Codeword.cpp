#include <iostream>
#include "MMConfig.h"
#include "Codeword.hpp"
#include "Scan.h"

namespace Mastermind {

#if 1
#else
Codeword Codeword::Parse(const char *s, const CodewordRules &rules)
{
#if MM_MAX_COLORS > 10
# error Codeword::Parse() only handles MM_MAX_COLORS <= 9
#endif

	int length = rules.pegs();
	int ndigits = rules.colors();
	bool allow_repetition = rules.repeatable();

	assert(s != NULL);
	assert(length > 0 && length <= MM_MAX_PEGS);
	assert(ndigits > 0 && ndigits <= MM_MAX_COLORS);

	Codeword ret;
	int k = 0;
	for (; k < length; k++) {
		if (s[k] >= '0' && s[k] <= '9') {
			unsigned char d = s[k] - '0';
			if (d > MM_MAX_COLORS) {
				return Codeword::Empty();
			}
			// BUG: WRONG ORDER
			if (!allow_repetition && ret.m_value.counter[d] > 0)
				return Codeword::Empty();
			ret.m_value.counter[d]++;
			ret.m_value.digit[k] = d;
		} else {
			return Codeword::Empty();
		}
	}
	if (s[k] != '\0') {
		return Codeword::Empty();
	}
	for (; k < MM_MAX_PEGS; k++) {
		ret.m_value.digit[k] = 0xFF;
	}
	return ret;
}
#endif

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

/*
void Codeword::CompareTo(const CodewordList& list, FeedbackList& fbl) const
{
	assert(list.GetCount() == fbl.GetCount());

	int count = list.GetCount();
	unsigned char *results = fbl.GetData();

	if (list.GetRules().allow_repetition) {
		CompareRepImpl->Run(m_value.value, (const __m128i*)list.GetData(), count, results);
	} else {
		CompareNoRepImpl->Run(m_value.value, (const __m128i*)list.GetData(), count, results);
	}
}
*/

#if 0
/// Compares this codeword to another codeword. 
/// \return The feedback of the comparison.
Feedback Codeword::CompareTo(const Codeword& guess) const
{
	unsigned char fb;
	codeword_t guess_value = guess.GetValue();
	CompareRepImpl->Run(m_value.value, (const __m128i*)&guess_value, 1, &fb);
	return Feedback(fb);
}
#endif

int Codeword::pegs() const
{
	int k;
	for (k = 0; k < MM_MAX_PEGS && _digit[k] != 0xFF; k++);
	return k;
}

} // namespace Mastermind
