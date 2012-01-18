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
	FeedbackFrequencyTable freq(FeedbackList(m_rules, guess, possibilities));
	StrategyTreeMemoryManager *mm = default_strat_mm;

	StrategyTreeNode *node = StrategyTreeNode::Create(mm);
	node->State.Guess = guess;
	node->State.NPossibilities = possibilities.size();
	node->State.NCandidates = 1;
	for (int fbv = 0; fbv <= freq.maxFeedback(); fbv++) 
	{
		Feedback fb(fbv);
		if (freq[fb] > 0) 
		{
			if (fb == Feedback(m_rules.pegs(), 0)) 
			{
				node->AddChild(fb, StrategyTreeNode::Done());
			} 
			else 
			{
				Codeword t = Codeword::Empty();
				StrategyTreeNode *child = FillStrategy(filterByFeedback(possibilities, m_rules, guess, fb), t);
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
