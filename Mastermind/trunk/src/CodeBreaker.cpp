#include <assert.h>
#include <stdio.h>
#include <limits>

#include "Codeword.h"
#include "CodeBreaker.h"
#include "CallCounter.h"

using namespace Mastermind;

///////////////////////////////////////////////////////////////////////////
// CodeBreaker implementations
//

CodeBreaker::CodeBreaker(CodewordRules rules)
{
	assert(rules.IsValid());

	m_rules = rules;
	m_all = CodewordList::Enumerate(rules);
	// m_fp = fopen("E:/mastermind_stat.txt", "w");
	m_fp = 0;

	Reset();
}

CodeBreaker::~CodeBreaker()
{
	if (m_fp) {
		fclose((FILE*)m_fp);
		m_fp = NULL;
	}
}

void CodeBreaker::Reset()
{
	m_possibilities = m_all;
	m_guessed = 0;
	m_unguessed = ((unsigned short)1 << m_rules.ndigits) - 1;
	m_impossible = 0;
	if (m_fp) {
		fprintf((FILE*)m_fp, "R\n");
	}
}

static int count_bits(unsigned short a)
{
	int n = 0;
	for (; a; a >>= 1) {
		if (a & 1)
			n++;
	}
	return n;
}

void CodeBreaker::AddFeedback(Codeword &guess, Feedback fb)
{
	m_possibilities = m_possibilities.FilterByFeedback(guess, fb);
	assert(m_possibilities.GetCount() > 0);
	unsigned short allmask = ((unsigned short)1 << m_rules.ndigits) - 1;
	m_unguessed &= ~guess.GetDigitMask();
	m_impossible = allmask & ~m_possibilities.GetDigitMask();
	m_guessed = allmask & ~m_unguessed & ~m_impossible;

	if (m_fp) {
		fprintf((FILE*)m_fp, "%x%x\n", count_bits(m_unguessed), count_bits(m_impossible));
	}
}

///////////////////////////////////////////////////////////////////////////
// SimpleCodeBreaker implementation
//

const char * SimpleCodeBreaker::GetName() const
{
	return "simple";
}

std::string SimpleCodeBreaker::GetDescription() const
{
	return "guesses the first possibility";
}

Codeword SimpleCodeBreaker::MakeGuess()
{
	assert(m_possibilities.GetCount() > 0);
	return m_possibilities[0];
}

Codeword SimpleCodeBreaker::MakeGuess(CodewordList &possibilities)
{
	assert(possibilities.GetCount() > 0);
	return possibilities[0];
}

static StrategyTreeMemoryManager *default_strat_mm = new StrategyTreeMemoryManager();

StrategyTreeNode* SimpleCodeBreaker::FillStrategy(CodewordList possibilities, const Codeword &first_guess)
{
	Codeword guess = first_guess.IsEmpty()? MakeGuess(possibilities) : first_guess;
	FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
	StrategyTreeMemoryManager *mm = default_strat_mm;

	StrategyTreeNode *node = StrategyTreeNode::Create(mm);
	node->State.Guess = guess;
	node->State.NPossibilities = possibilities.GetCount();
	node->State.NCandidates = 1;
	for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
		Feedback fb(fbv);
		if (freq[fb] > 0) {
			if (fb == Feedback(m_rules.length, 0)) {
				node->AddChild(fb, StrategyTreeNode::Done());
			} else {
				Codeword t = Codeword::Empty();
				StrategyTreeNode *child = FillStrategy(possibilities.FilterByFeedback(guess, fb), t);
				node->AddChild(fb, child);
			}
		}
	}
	return node;
}

StrategyTree* SimpleCodeBreaker::BuildStrategyTree(const Codeword& first_guess)
{
	CodewordList all = m_all.Copy();
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

///////////////////////////////////////////////////////////////////////////
// HeuristicCodeBreaker implementation
//

template <class Heuristic>
const char * HeuristicCodeBreaker<Heuristic>::GetName() const
{
	return Heuristic::name;
}

template <class Heuristic>
std::string HeuristicCodeBreaker<Heuristic>::GetDescription() const
{
	return Heuristic::name;
}

template <class Heuristic>
StrategyTreeNode* HeuristicCodeBreaker<Heuristic>::FillStrategy(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	const Codeword& first_guess,
	int *progress)
{
	StrategyTreeState state;
	if (first_guess.IsEmpty()) {
		MakeGuess(possibilities, unguessed_mask, impossible_mask, &state);
	} else {
		state.NPossibilities = possibilities.GetCount();
		state.NCandidates = -1;
		state.Guess = first_guess;
	}

	StrategyTreeMemoryManager *mm = default_strat_mm;

	Codeword guess = state.Guess;
	FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));

	Feedback perfect = Feedback(m_rules.length, 0);
	StrategyTreeNode *node = StrategyTreeNode::Create(mm);
	node->State = state;
	for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
		Feedback fb(fbv);
		if (freq[fb] > 0) {
			if (fb == perfect) {
				(*progress)++;
				double pcnt = (double)(*progress) / m_all.GetCount();
				printf("\rProgress: %3.0f%%", pcnt*100);
				fflush(stdout);
				node->AddChild(fb, StrategyTreeNode::Done());
			} else {
				Codeword t = Codeword::Empty();
				CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
				StrategyTreeNode *child = FillStrategy(filtered,
					unguessed_mask & ~guess.GetDigitMask(),
					((1<<m_rules.ndigits)-1) & ~filtered.GetDigitMask(),
					t,
					progress);
				node->AddChild(fb, child);
			}
		}
	}
	return node;
}

template <class Heuristic>
StrategyTree* HeuristicCodeBreaker<Heuristic>::BuildStrategyTree(const Codeword& first_guess)
{
	CodewordList all = m_all.Copy();
	unsigned short impossible_mask = 0;
	unsigned short unguessed_mask = (1 << m_rules.ndigits) - 1;
	int progress = 0;
	StrategyTreeNode *node = FillStrategy(
		all, unguessed_mask, impossible_mask, first_guess, &progress);
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
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	StrategyTreeState *state)
{
	assert(possibilities.GetCount() > 0);
	state->NPossibilities = possibilities.GetCount();

	// Optimize if there are only two possibilities left
	if (possibilities.GetCount() <= 2) {
		state->NCandidates = 1;
		state->Guess = possibilities[0];
		return;
	}

	// Optimize if there are less than p(p+3)/2 possibilities left
	int npos = possibilities.GetCount(); // number of remaining possibilities
	int npegs = m_rules.length; // number of pegs
	int pretest = 0;
	if (possibilities.GetCount() <= npegs*(npegs+3)/2) {
		pretest = npos;
		for (int i = 0; i < possibilities.GetCount(); i++) {
			Codeword guess = possibilities[i];
			FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
			if (freq.GetMaximum() == 1) {
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
	Feedback target = Feedback(m_rules.length, 0);
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];
		FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));

		// Evaluate each potential guess, and find the minimum
		Heuristic::score_t score = Heuristic::compute(freq);
		if ((score < choose_score) || 
			(score == choose_score && !choose_ispos && freq[target] > 0)) {
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
const char * Heuristics::MinimizeWorstCase::name = "minmax";
template HeuristicCodeBreaker<Heuristics::MinimizeWorstCase>;

// MinAvg heuristic
unsigned int Heuristics::MinimizeAverage::compute(const FeedbackFrequencyTable &freq)
{
	return freq.GetSumOfSquares();
}
const char * Heuristics::MinimizeAverage::name = "minavg";
template HeuristicCodeBreaker<Heuristics::MinimizeAverage>;

// Entropy heuristic
double Heuristics::MaximizeEntropy::compute(const FeedbackFrequencyTable &freq)
{
	return freq.GetModifiedEntropy();
}
const char * Heuristics::MaximizeEntropy::name = "entropy";
template HeuristicCodeBreaker<Heuristics::MaximizeEntropy>;

// Max parts heuristic
int Heuristics::MaximizePartitions::compute(const FeedbackFrequencyTable &freq)
{
	return -freq.GetPartitionCount();
}
const char * Heuristics::MaximizePartitions::name = "maxparts";
template HeuristicCodeBreaker<Heuristics::MaximizePartitions>;

///////////////////////////////////////////////////////////////////////////
// OptimalCodeBreaker implementation
//

const char * OptimalCodeBreaker::GetName() const
{
	return "optimal";
}

std::string OptimalCodeBreaker::GetDescription() const
{
	return "makes the optimal guess by depth-first search";
}

Codeword OptimalCodeBreaker::MakeGuess()
{
	// Not implemented
	return m_possibilities[0];
}

static StrategyTreeMemoryManager *optimal_codebreaker_mm = new StrategyTreeMemoryManager();

StrategyTreeNode* OptimalCodeBreaker::FillStrategy(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	const Codeword& first_guess,
	int *progress)
{
	int npegs = m_rules.length; // number of pegs
	int npos = possibilities.GetCount(); // number of remaining possibilities
	assert(npos > 0);
	StrategyTreeMemoryManager *mm = optimal_codebreaker_mm;

	// Optimize if there is only one possibility
	if (npos == 1) {
		StrategyTreeNode *tree = StrategyTreeNode::Create(mm);
		tree->State.NPossibilities = npos;
		tree->State.NCandidates = 1;
		tree->State.Guess = possibilities[0];
		tree->AddChild(Feedback::Perfect(npegs), StrategyTreeNode::Done());
		return tree;
	}

	// Optimize if there are only two possibilities left
	if (npos == 2) {
		StrategyTreeNode *tree = StrategyTreeNode::Create(mm);
		tree->State.NPossibilities = npos;
		tree->State.NCandidates = 1;
		tree->State.Guess = possibilities[0];
		tree->AddChild(
			Feedback::Perfect(npegs), 
			StrategyTreeNode::Done());
		tree->AddChild(
			possibilities[0].CompareTo(possibilities[1]), 
			StrategyTreeNode::Single(mm, possibilities[1]));
		return tree;
	}

	// Optimize if there are less than p(p+3)/2 possibilities left
	int npretest = 0;
#if 1
	if (npos <= npegs*(npegs+3)/2) {
		for (int i = 0; i < possibilities.GetCount(); i++) {
			Codeword guess = possibilities[i];
			FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
			if (freq.GetMaximum() == 1) {
				StrategyTreeNode *tree = StrategyTreeNode::Create(mm);
				tree->State.NPossibilities = npos;
				tree->State.NCandidates = i + 1;
				tree->State.Guess = guess;
				for (int j = 0; j < npos; j++) {
					Codeword secret = possibilities[j];
					Feedback fb = guess.CompareTo(secret);
					if (fb == Feedback::Perfect(npegs)) {
						tree->AddChild(fb, StrategyTreeNode::Done());
					} else {
						tree->AddChild(fb, StrategyTreeNode::Single(mm, secret));
					}
				}
				return tree;
			}
		}
		npretest = npos;
	}
#endif

	// Now we have to iterate through each candidate guess.
	// Find out "distinct" codewords as candidate for next guess
	CodewordList candidates = m_all.FilterByEquivalence(unguessed_mask, impossible_mask);

	// Try each possible guess
	StrategyTreeNode *best_tree = NULL;
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];
		FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
		
		//assert(freq.GetPartitionCount() > 1);
		if (freq.GetPartitionCount() <= 1) { // an impossible guess
			continue;
		}
		StrategyTreeNode *this_tree = StrategyTreeNode::Create(mm);
		this_tree->State.Guess = guess;

		// Find the "best" guess route for each possible feedback
		for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
			Feedback fb = Feedback(fbv);
			if (freq[fb] > 0) {
				if (fb == Feedback::Perfect(npegs)) {
					this_tree->AddChild(fb, StrategyTreeNode::Done());
				} else {
					CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
					int new_unguessed_mask = unguessed_mask & ~guess.GetDigitMask();
					int new_impossible_mask = m_rules.GetFullDigitMask() & ~filtered.GetDigitMask();
					Codeword dummy = Codeword::Empty();
					this_tree->AddChild(fb, FillStrategy(filtered, new_unguessed_mask, new_impossible_mask,
						dummy, progress));
				}
			}
		}

		// Is this guess good?
		if (best_tree == NULL) {
			best_tree = this_tree;
		} else if ((this_tree->GetTotalDepth() < best_tree->GetTotalDepth()) ||
			(this_tree->GetTotalDepth() == best_tree->GetTotalDepth() && 
			 this_tree->GetDepth() < best_tree->GetDepth())) {
			StrategyTreeNode::Destroy(mm, best_tree);
			best_tree = this_tree;
		} else {
			StrategyTreeNode::Destroy(mm, this_tree);
		}
	}
		
	best_tree->State.NPossibilities = npos;
	best_tree->State.NCandidates = npretest + candidates.GetCount();
	return best_tree;

}

/*
int OptimalCodeBreaker::SearchLowestSteps(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	int *pdepth,
	Codeword *choice)
{
	assert(possibilities.GetCount() > 0);

	if (possibilities.GetCount() == 1) {
		if (choice) {
			*choice = possibilities[0];
		}
		(*pdepth) += 1;
		return 1;
	}
	if (possibilities.GetCount() == 2) {
		if (choice) {
			*choice = possibilities[0];
		}
		(*pdepth) += 2;
		return 3;
	}

	// Find out "distinct" codewords as candidate for next guess
	CodewordList candidates = m_all.FilterByEquivalence(unguessed_mask, impossible_mask);

	// Try each possible guess
	Feedback target = Feedback(m_rules.length, 0);
	int best_steps = 0x7fffffff;
	int best_depth = 100;
	int best_i = 0;
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];
		FeedbackList fbl(guess, possibilities);
		FeedbackFrequencyTable freq(fbl);
		
		//assert(freq.GetPartitionCount() > 1);
		if (freq.GetPartitionCount() <= 1) {
			continue;
		}

		// Find the "best" guess route for each possible feedback
		int total_steps = 0;
		int depth = (*pdepth) + 1;
		for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
			Feedback fb = Feedback(fbv);
			if (freq[fb] > 0) {
				if (fb == target) {
					total_steps += 1; // CHECK HERE
				} else {
					CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
					int temp_depth = (*pdepth) + 1;
					int new_unguessed_mask = unguessed_mask & ~guess.GetDigitMask();
					int new_impossible_mask = m_rules.GetFullDigitMask() & ~filtered.GetDigitMask();
					int steps = SearchLowestSteps(filtered, new_unguessed_mask, new_impossible_mask, &temp_depth, NULL);
					total_steps += steps + filtered.GetCount();
					if (temp_depth > depth)
						depth = temp_depth;
				}
			}
		}
		if ((total_steps < best_steps) ||
			(total_steps == best_steps) && (depth < best_depth)) {
			best_steps = total_steps;
			best_depth = depth;
			best_i = i;
		}
	}

	if (choice) {
		*choice = candidates[best_i];
	}
	(*pdepth) = best_depth;
	return best_steps;
}
*/

/*
StrategyTreeNode* OptimalCodeBreaker::FillStrategy(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	const Codeword& first_guess,
	int *progress)
{
	Codeword guess = first_guess.IsEmpty()?
		MakeGuess(possibilities, unguessed_mask, impossible_mask) : first_guess;

	FeedbackList fbl(guess, possibilities);
	FeedbackFrequencyTable freq(fbl);

	Feedback perfect = Feedback(m_rules.length, 0);
	StrategyTreeNode *node = new StrategyTreeNode();
	node->State.Guess = guess;
	node->State.NPossibilities = possibilities.GetCount();
	node->State.NCandidates = -1;
	for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
		Feedback fb(fbv);
		if (freq[fb] > 0) {
			if (fb == perfect) {
				node->AddChild(fb, StrategyTreeNode::Done());
				(*progress)++;
				//double pcnt = (double)(*progress) / m_all.GetCount();
				//printf("\rProgress: %3.0f%%", pcnt*100);
				//fflush(stdout);
			} else {
				Codeword t = Codeword::Empty();
				CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
				StrategyTreeNode *child = FillStrategy(filtered,
					unguessed_mask & ~guess.GetDigitMask(),
					((1<<m_rules.ndigits)-1) & ~filtered.GetDigitMask(),
					t,
					progress);
				node->AddChild(fb, child);
			}
		}
	}
	return node;
}
*/

/*
Codeword OptimalCodeBreaker::MakeGuess(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask)
{
	Codeword guess;
	int depth = 0;
	SearchLowestSteps(possibilities, unguessed_mask, impossible_mask, &depth, &guess);
	return guess;
}
*/

StrategyTree* OptimalCodeBreaker::BuildStrategyTree(const Codeword& first_guess)
{
	CodewordList all = m_all.Copy();
	unsigned short impossible_mask = 0;
	unsigned short unguessed_mask = m_rules.GetFullDigitMask();
	int progress = 0;
	StrategyTreeNode *node = FillStrategy(
		all, unguessed_mask, impossible_mask, first_guess, &progress);
	return (StrategyTree*)node;
}

