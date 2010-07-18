#include "Feedback.h"
#include "Codeword.h"

using namespace Mastermind;

FeedbackList::FeedbackList(const Codeword &guess, const CodewordList &secrets)
{
	Allocate(secrets.GetCount(), 1);
	guess.CompareTo(secrets, *this);
}
