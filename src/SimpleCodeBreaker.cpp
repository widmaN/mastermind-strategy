#include <cassert>

#include "SimpleCodeBreaker.hpp"
#include "Algorithm.hpp"

namespace Mastermind {

Codeword SimpleCodeBreaker::MakeGuess()
{
	assert(m_possibilities.size() > 0);
	return m_possibilities[0];
}

Codeword SimpleCodeBreaker::MakeGuess(CodewordList &possibilities)
{
	assert(possibilities.size() > 0);
	return possibilities[0];
}

StrategyTreeNode* SimpleCodeBreaker::FillStrategy(CodewordList possibilities, const Codeword &first_guess)
{
	Codeword guess = first_guess.empty()? MakeGuess(possibilities) : first_guess;

	// @todo: use rvalue reference to reduce copying of feedback list.
#if 0
	FeedbackFrequencyTable freq;
	{
		FeedbackList feedbacks = e.compare(guess, 
			possibilities.cbegin(), possibilities.cend());
		e.countFrequencies(feedbacks.begin(), feedbacks.end(), freq);
	}
#else
	FeedbackFrequencyTable freq = e.frequency(e.compare(guess, 
		possibilities.begin(), possibilities.end()));
#endif
	StrategyTreeMemoryManager *mm = default_strat_mm;

	StrategyTreeNode *node = StrategyTreeNode::Create(mm);
	node->State.Guess = guess;
	node->State.NPossibilities = possibilities.size();
	node->State.NCandidates = 1;
	for (size_t i = 0; i < freq.size(); i++)
	{
		Feedback fb(i);
		if (freq[i] > 0) 
		{
			if (fb == Feedback::perfectValue(e.rules())) 
			{
				node->AddChild(fb, StrategyTreeNode::Done());
			} 
			else 
			{
				Codeword t = Codeword::emptyValue();
				StrategyTreeNode *child = FillStrategy(
					e.filterByFeedback(possibilities, guess, fb), t);
				node->AddChild(fb, child);
			}
		}
	}
	return node;
}

StrategyTree* SimpleCodeBreaker::BuildStrategyTree(const Codeword& first_guess)
{
	CodewordList all = m_all;
	return (StrategyTree*)FillStrategy(all, first_guess);
	/*
	partition the possibility list, i.e. reorder the elements so that
		elements with same feedback are put together;
	foreach (feedback in all feedbacks) {
		filter possibilities with the feedback (locate the partition);
		find the first remaining element in possibilities;
		use this item as the guess, and push it into strategy tree;
		the remaining possibilities are the partition;
		call BuildStrategyTree() recursively;
	}

	return (the strategy tree built this way);
	*/
}

} // namespace Mastermind
