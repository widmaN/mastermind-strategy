///////////////////////////////////////////////////////////////////////////
// HeuristicCodeBreaker implementation
//

#include "HeuristicCodeBreaker.hpp"
#include "Algorithm.hpp"

namespace Mastermind {

template <class Heuristic>
StrategyTreeNode* HeuristicCodeBreaker<Heuristic>::FillStrategy(
	CodewordList::iterator first_possibility,
	CodewordList::iterator last_possibility,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	const Codeword &first_guess,
	int *progress)
{
	StrategyTreeState state;
	if (first_guess.empty()) 
	{
		MakeGuess(first_possibility, last_possibility, 
			unguessed_mask, impossible_mask, &state);
	}
	else 
	{
		state.NPossibilities = last_possibility - first_possibility;
		state.NCandidates = -1;
		state.Guess = first_guess;
	}

	StrategyTreeMemoryManager *mm = default_strat_mm;

	Codeword guess = state.Guess;
	FeedbackFrequencyTable freq;
	partition(first_possibility, last_possibility, m_rules, guess, freq);
	//possibilities.Partition(guess, freq);
	//FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));

	Feedback perfect = Feedback::Perfect(m_rules.pegs());
	StrategyTreeNode *node = StrategyTreeNode::Create(mm);
	node->State = state;
	int k = 0;
	for (int fbv = 0; fbv <= freq.maxFeedback(); fbv++) 
	{
		Feedback fb(fbv);
		int n = freq[fb];
		if (n <= 0)
			continue;

		if (fb == perfect) 
		{
			(*progress)++;
			double pcnt = (double)(*progress) / m_all.size();
			printf("\rProgress: %3.0f%%", pcnt*100);
			fflush(stdout);
			node->AddChild(fb, StrategyTreeNode::Done());
		} 
		else 
		{
			Codeword t = Codeword::Empty();
			// CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
			CodewordList::iterator first = first_possibility + k;
			CodewordList::iterator last = first + n;
			//CodewordList filtered(possibilities, k, n);
			StrategyTreeNode *child = FillStrategy(
				first,
				last,
				//filtered,
				unguessed_mask & ~getDigitMask(guess),
				((1<<m_rules.colors())-1) & ~getDigitMask(first, last),
				t,
				progress);
			node->AddChild(fb, child);
		}
		k += n;
	}
	return node;
}

template <class Heuristic>
StrategyTree* HeuristicCodeBreaker<Heuristic>::BuildStrategyTree(const Codeword& first_guess)
{
	CodewordList all = m_all;
	unsigned short impossible_mask = 0;
	unsigned short unguessed_mask = (1 << m_rules.colors()) - 1;
	int progress = 0;
	StrategyTreeNode *node = FillStrategy(
		all.begin(), all.end(), unguessed_mask, impossible_mask, first_guess, &progress);
	return (StrategyTree*)node;

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

#if ENABLE_CALL_COUNTER
static Utilities::CallCounter _call_counter("HeuristicCodeBreaker::MakeGuess", true);
#endif

void PrintMakeGuessStatistics()
{
#if ENABLE_CALL_COUNTER
	_call_counter.DebugPrint();
#endif
}

template <class Heuristic>
void HeuristicCodeBreaker<Heuristic>::MakeGuess(
	CodewordList::const_iterator first_possibility,
	CodewordList::const_iterator last_possibility,
	//CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	StrategyTreeState *state)
{
	size_t count = last_possibility - first_possibility;
	assert(count > 0);
	state->NPossibilities = count;

	// Optimize if there are only two possibilities left
	if (count <= 2) 
	{
		state->NCandidates = 1;
		state->Guess = possibilities[0];
		return;
	}

	// Optimize if there are less than p(p+3)/2 possibilities left
	int npos = count; // number of remaining possibilities
	int npegs = m_rules.pegs(); // number of pegs
	int pretest = 0;
	if (count <= npegs*(npegs+3)/2) 
	{
		pretest = npos;
		for (int i = 0; i < possibilities.GetCount(); i++) 
		{
			Codeword guess = possibilities[i];
			FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
			if (freq.GetMaximum() == 1) 
			{
				state->NCandidates = i + 1;
				state->Guess = guess;
#if ENABLE_CALL_COUNTER
				_call_counter.AddCall(state->NCandidates*state->NPossibilities);
#endif
				return;
			}
		}
		pretest = npos;
	}

	// Calculate a score for each guess
	CodewordList candidates = m_posonly? possibilities : m_all;
	candidates = candidates.FilterByEquivalence(unguessed_mask, impossible_mask);

	Heuristic::score_t choose_score = std::numeric_limits<Heuristic::score_t>::max();
	int choose_i = -1;
	int choose_ispos = false;
	Feedback target = Feedback(m_rules.pegs(), 0);
	for (int i = 0; i < candidates.GetCount(); i++) 
	{
		Codeword guess = candidates[i];
		FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));

		// Evaluate each potential guess, and find the minimum
		Heuristic::score_t score = Heuristic::compute(freq);
		if ((score < choose_score) || 
			(score == choose_score && !choose_ispos && freq[target] > 0)) 
		{
			choose_score = score;
			choose_i = i;
			choose_ispos = (freq[target] > 0);
		}
	}

	state->NCandidates = candidates.GetCount();
	state->Guess = candidates[choose_i];
#if ENABLE_CALL_COUNTER
	_call_counter.AddCall(state->NCandidates*state->NPossibilities);
#endif
	return;
}

template <class Heuristic>
Codeword HeuristicCodeBreaker<Heuristic>::MakeGuess(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask)
{
	StrategyTreeState state;
	MakeGuess(possibilities, unguessed_mask, impossible_mask, &state);
	return state.Guess;
}

template <class Heuristic>
Codeword HeuristicCodeBreaker<Heuristic>::MakeGuess()
{
	StrategyTreeState state;
	MakeGuess(m_possibilities, m_unguessed, m_impossible, &state);
	return state.Guess;
}

///////////////////////////////////////////////////////////////////////////
// Heuristic Definitions
//

// MinMax heuristic
unsigned int Heuristics::MinimizeWorstCase::compute(const FeedbackFrequencyTable &freq)
{
	return freq.GetMaximum();
}
template HeuristicCodeBreaker<Heuristics::MinimizeWorstCase>;

// MinAvg heuristic
unsigned int Heuristics::MinimizeAverage::compute(const FeedbackFrequencyTable &freq)
{
	return freq.GetSumOfSquares();
}
template HeuristicCodeBreaker<Heuristics::MinimizeAverage>;

// Entropy heuristic
double Heuristics::MaximizeEntropy::compute(const FeedbackFrequencyTable &freq)
{
	return freq.GetModifiedEntropy();
}
template HeuristicCodeBreaker<Heuristics::MaximizeEntropy>;

// Max parts heuristic
int Heuristics::MaximizePartitions::compute(const FeedbackFrequencyTable &freq)
{
	return -freq.GetPartitionCount();
}
template HeuristicCodeBreaker<Heuristics::MaximizePartitions>;

// Minimize steps heuristic
int Heuristics::MinimizeSteps::compute(const FeedbackFrequencyTable &freq)
{
	int minsteps = 0;
	for (int fbv = 0; fbv <= freq.maxFeedback(); fbv++) {
		Feedback fb(fbv);
		int n = freq[fb];
		if (n > 0) {
			if (fb == Feedback::Perfect(4)) {
				//
			} else {
				minsteps += n;
				minsteps += partition_score[n];
			}
		}
	}
	return minsteps;
}

int Heuristics::MinimizeSteps::partition_score[10000];
template HeuristicCodeBreaker<Heuristics::MinimizeSteps>;

} // namespace Mastermind
