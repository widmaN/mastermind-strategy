#include "Feedback.h"
#include "Codeword.h"

using namespace Mastermind;

FeedbackList::FeedbackList(const Codeword &guess, const CodewordList &secrets)
{
	Allocate(secrets.GetCount(), 0);
	guess.CompareTo(secrets, *this);
}
