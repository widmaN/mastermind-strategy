#include "Feedback.h"
#include "Codeword.h"

using namespace Mastermind;

FeedbackList::FeedbackList(const Codeword &guess, const CodewordList &secrets)
{
	m_count = secrets.GetCount();
	m_values = (unsigned char *)_malloca(m_count);
	guess.CompareTo(secrets, *this);
}
