///////////////////////////////////////////////////////////////////////////
// HeuristicCodeBreaker implementation
//

#include "HeuristicCodeBreaker.hpp"
#include "Algorithm.hpp"

#if ENABLE_CALL_COUNTER
static Utilities::CallCounter _call_counter("HeuristicCodeBreaker::MakeGuess", true);
#endif

void PrintMakeGuessStatistics()
{
#if ENABLE_CALL_COUNTER
	_call_counter.DebugPrint();
#endif
}

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// MakeGuess() implementation.

template <class Heuristic>
void HeuristicCodeBreaker<Heuristic>::MakeGuess(
	CodewordConstRange possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	StrategyTreeState *state)
{
	size_t count = possibilities.size();
	assert(count > 0);
	state->NPossibilities = count;

	// Check for obvious guess.
	auto it = makeObviousGuess(possibilities);
	if (it != possibilities.end())
	{
		state->NCandidates = (it - possibilities.begin()) + 1;
		state->Guess = *it;
#if ENABLE_CALL_COUNTER
		_call_counter.AddCall(state->NCandidates*state->NPossibilities);
#endif
		return;
	}

	// Now that there are no obvious guesses, we need to make a guess 
	// according to the supplied heuristic.

	// Initialize the set of candidate guesses.
	CodewordConstRange candidates1 = m_posonly? possibilities : m_all;

	// Filter the candidate set to remove "equivalent" guesses.
	// For now the equivalence is determined by bit-mask of colors.
	// In the next step, we should determine it by graph isomorphasm.
	CodewordList candidates = 
		candidates1.FilterByEquivalence(unguessed_mask, impossible_mask);

	// Initialize some book-keeping variables.
	typename Heuristic::score_t choose_score = 0;
	size_t choose_i = 0;
	bool choose_ispos = false;
	size_t target = Feedback::perfectValue(e.rules()).value();

	// Evaluate each potential guess and find the one with the lowest score.
	for (size_t i = 0; i < candidates.size(); i++) 
	{
		Codeword guess = candidates[i];
		FeedbackFrequencyTable freq = 
			e.frequency(e.compare(guess, possibilities));

		Heuristic::score_t score = Heuristic::compute(freq);
		if ((i == 0) || (score < choose_score) || 
			(score == choose_score && !choose_ispos && freq[target] > 0)) 
		{
			choose_score = score;
			choose_i = i;
			choose_ispos = (freq[target] > 0);
		}
	}

	// Store the guess we made.
	state->NCandidates = candidates.size();
	state->Guess = candidates[choose_i];
#if ENABLE_CALL_COUNTER
	_call_counter.AddCall(state->NCandidates*state->NPossibilities);
#endif
}

#if 0
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
#endif

template <class Heuristic>
Codeword HeuristicCodeBreaker<Heuristic>::MakeGuess()
{
	StrategyTreeState state;
	MakeGuess(m_possibilities, m_unguessed, m_impossible, &state);
	return state.Guess;
}

#if 0
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
	e.partition(first_possibility, last_possibility, guess, freq);
	//possibilities.Partition(guess, freq);
	//FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));

	Feedback perfect = Feedback::perfectValue(e.rules());
	StrategyTreeNode *node = StrategyTreeNode::Create(mm);
	node->State = state;
#if 0
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
#else
	// Try write something like this:
	auto p = partition(...);
	for (auto it = p.begin(); it != p.end(); ++it)
	{
		CodewordPartition::Cell cell = *it;
		if (cell.feedback() == perfect) 
		{
			(*progress)++;
			double pcnt = (double)(*progress) / m_all.size();
			printf("\rProgress: %3.0f%%", pcnt*100);
			fflush(stdout);
			node->AddChild(fb, StrategyTreeNode::Done());
		}
		else
		{
			StrategyTreeNode *child = FillStrategy(
				cell.begin(),
				cell.end(),
				unguessed_mask & ~getDigitMask(guess),
				((1<<m_rules.colors())-1) & ~getDigitMask(first, last),
				Codeword::emptyValue(),
				progress);
			node->AddChild(fb, child);
		}
	}
#endif

	return node;
}
#endif

#if 0
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
#endif


///////////////////////////////////////////////////////////////////////////
// Instantiate the heuristic solver with pre-built heuristics.
// Ideally we should put the logic in the header file, so that
// we don't need to explicitly instantiate them.

#if 0
template HeuristicCodeBreaker<Heuristics::MinimizeWorstCase>;
template HeuristicCodeBreaker<Heuristics::MinimizeAverage>;
template HeuristicCodeBreaker<Heuristics::MaximizeEntropy>;
template HeuristicCodeBreaker<Heuristics::MaximizePartitions>;
#endif

///////////////////////////////////////////////////////////////////////////
// Heuristic Definitions
//

#if 0
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
#endif

#if 0
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
#endif

