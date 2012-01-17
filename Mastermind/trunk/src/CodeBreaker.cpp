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
	FeedbackFrequencyTable freq;
	possibilities.Partition(guess, freq);
	//FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));

	Feedback perfect = Feedback::Perfect(m_rules.length);
	StrategyTreeNode *node = StrategyTreeNode::Create(mm);
	node->State = state;
	int k = 0;
	for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
		Feedback fb(fbv);
		int n = freq[fb];
		if (n <= 0)
			continue;

		if (fb == perfect) {
			(*progress)++;
			double pcnt = (double)(*progress) / m_all.GetCount();
			printf("\rProgress: %3.0f%%", pcnt*100);
			fflush(stdout);
			node->AddChild(fb, StrategyTreeNode::Done());
		} else {
			Codeword t = Codeword::Empty();
			// CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
			CodewordList filtered(possibilities, k, n);
			StrategyTreeNode *child = FillStrategy(
				filtered,
				unguessed_mask & ~guess.GetDigitMask(),
				((1<<m_rules.ndigits)-1) & ~filtered.GetDigitMask(),
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

// Minimize steps heuristic
int Heuristics::MinimizeSteps::compute(const FeedbackFrequencyTable &freq)
{
	int minsteps = 0;
	for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
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
const char * Heuristics::MinimizeSteps::name = "minsteps";
int Heuristics::MinimizeSteps::partition_score[10000];
template HeuristicCodeBreaker<Heuristics::MinimizeSteps>;


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

static int ocb_depthstat[50] = {0};
static int ocb_stepstat[100000] = {0};
static int ocb_boundstat[100000] = {0};

void OCB_PrintStatistics()
{
	printf("==== Round Statistics ====\n");
	for (int i = 1; i < 50; i++) {
		if (ocb_depthstat[i] > 0) {
			printf("Round %2d: %d\n", i, ocb_depthstat[i]);
		}
	}
	if (0) {
		printf("==== Step Statistics ====\n");
		for (int i = 1; i < 10000; i++) {
			if (ocb_stepstat[i] > 0) {
				printf("Steps %5d: %d\n", i, ocb_stepstat[i]);
			}
		}	
	}
	if (1) {
		printf("==== Lower Bound Statistics ====\n");
		for (int i = 1; i < 10000; i++) {
			if (ocb_boundstat[i] > 0) {
				printf("Lower bound %5d: %d\n", i, ocb_boundstat[i]);
			}
		}
	}
}

static StrategyTreeMemoryManager *optimal_codebreaker_mm = new StrategyTreeMemoryManager();

StrategyTreeNode* OptimalCodeBreaker::FillStrategy(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	const Codeword& first_guess,
	const progress_t *progress,
	const search_t *arg)
{
	ocb_depthstat[arg->round]++;
	ocb_stepstat[arg->total_steps]++;

	int npegs = m_rules.length; // number of pegs
	int npos = possibilities.GetCount(); // number of remaining possibilities
	assert(npos > 0);
	StrategyTreeMemoryManager *mm = optimal_codebreaker_mm;

	if (1) {
		int lb = arg->lower_bound + (arg->round - 1) * npos
			+ m_partsize2minsteps[npos];
		ocb_boundstat[lb]++;
		if (lb > arg->max_cost) // branch pruning
			return NULL;
	}

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

	// Check whether we need children to display progress
	double prog_step = (progress->end - progress->begin) / (double)candidates.GetCount();
	bool display_progress = progress->display;
	double last_pcnt = -1.0;

	// Try each possible guess
	StrategyTreeNode *best_tree = NULL;
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];
		FeedbackFrequencyTable freq;
		possibilities.Partition(guess, freq);
		// FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
		
		// Display progress
		if (display_progress) {
			double pcnt = progress->begin + prog_step*i;
			if (pcnt - last_pcnt >= 0.001) {
				printf("\rProgress: %4.1f%%", pcnt*100);
				fflush(stdout);
			}
			if (pcnt > 0.9) {
				int k = 0;
			}
			last_pcnt = pcnt;
		}

		//assert(freq.GetPartitionCount() > 1);
		int npart = freq.GetPartitionCount();
		if (npart <= 1) { // an impossible guess
			continue;
		}
		StrategyTreeNode *this_tree = StrategyTreeNode::Create(mm);
		this_tree->State.Guess = guess;

		// Estimate the lower bound of total steps needed if we make
		// this particular guess.
		int lbs[256];
		int lb = 0;
		for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
			int part_size = freq[Feedback(fbv)];
			if (part_size == 0) {
				lbs[fbv] = 0;
			} else {
				if (Feedback(fbv) == Feedback::Perfect(npegs)) {
					lbs[fbv] = arg->round;
				} else {
					lbs[fbv] = arg->round*part_size + m_partsize2minsteps[part_size];
				}
			}
			lb += lbs[fbv];
		}
		search_t arg2;
		arg2.round = arg->round + 1;
		arg2.total_steps = arg->total_steps;
		arg2.lower_bound = arg->lower_bound + lb;
		arg2.max_cost = arg->max_cost;

		// Find the best guess tree for each possible feedback
		//int stepstat = arg->total_steps + possibilities.GetCount();
		int k = 0;
		for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
			Feedback fb = Feedback(fbv);
			int n = freq[fb];
			if (n <= 0)
				continue;

			if (fb == Feedback::Perfect(npegs)) {
				arg2.lower_bound -= lbs[fbv];
				this_tree->AddChild(fb, StrategyTreeNode::Done());
				arg2.total_steps += arg->round;
				arg2.lower_bound += arg->round;
			} else {
				//CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
				CodewordList filtered(possibilities, k, n);
				int new_unguessed_mask = unguessed_mask & ~guess.GetDigitMask();
				int new_impossible_mask = m_rules.GetFullDigitMask() & ~filtered.GetDigitMask();
				Codeword dummy = Codeword::Empty();
				progress_t prog;
				if (display_progress) {
					prog.display = (prog_step > 0.001);
					prog.begin = progress->begin + prog_step*i + prog_step*k/npos;
					prog.end = progress->begin + prog_step*i + prog_step*(k+n)/npos;
				} else {
					prog.display = false;
					prog.begin = 0;
					prog.end = 0;
				}
				arg2.lower_bound -= lbs[fbv];
				StrategyTreeNode *child = FillStrategy(
					filtered, new_unguessed_mask, new_impossible_mask,
					dummy, &prog, &arg2);
				if (child == NULL) { // pruned; no need to continue on this way
					StrategyTreeNode::Destroy(mm, this_tree);
					this_tree = NULL;
					break;
				}
				this_tree->AddChild(fb, child);
				arg2.total_steps += arg->round*n + child->GetTotalDepth();
				arg2.lower_bound += arg->round*n + child->GetTotalDepth();
				//stepstat += child->GetTotalDepth();
			}
			k += n;
		}

		// If guess pruned, skip it
		if (this_tree == NULL)
			continue;

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
	
	if (best_tree != NULL) { // not pruned
		best_tree->State.NPossibilities = npos;
		best_tree->State.NCandidates = npretest + candidates.GetCount();
	}
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

	progress_t progress;
	progress.begin = 0.0;
	progress.end = 1.0;
	progress.display = true;

	// Use heuristic code breaker to estimate an upper bound of total guesses
	CodeBreaker* breakers[] = {
		new HeuristicCodeBreaker<Heuristics::MinimizeWorstCase>(m_rules, false),
		new HeuristicCodeBreaker<Heuristics::MinimizeAverage>(m_rules, false),
		new HeuristicCodeBreaker<Heuristics::MaximizePartitions>(m_rules, false),
		new HeuristicCodeBreaker<Heuristics::MaximizeEntropy>(m_rules, false),
	};
	bool verbose = true;
	CodeBreaker *best_cb = NULL;
	int best_steps = 0x7fffffff;
	if (verbose) {
		printf("Testing heuristic algorithms...\n");
	}
	for (int ib = 0; ib < 4; ib++) {
		CodeBreaker *breaker = breakers[ib];
		StrategyTree *tree = breaker->BuildStrategyTree(first_guess);
		// TODO: free memory
		int steps = tree->GetTotalDepth();
		if (steps < best_steps) {
			best_cb = breaker;
			best_steps = steps;
		}
		if (verbose) {
			printf("\r    %8s: %d    \n", breaker->GetName(), steps);
		}
	}
	printf("Best heuristic: %s\n", best_cb->GetName());

	m_partsize2minsteps = Heuristics::MinimizeSteps::partition_score;

	search_t info;
	info.round = 1; // which round is this? (initial guess has round=1)
	info.total_steps = 0; // total steps taken so far for the guessed-out secrets
	info.lower_bound = 0; // lower bound of the total number of steps needed to
	                      // solve the entire strategy tree, EXCLUDING the sub-tree
	                      // that is currently being solved

	// Maximum cost allowed for the search tree. If the cost exceeds this
	// threshold, no need to continue searching because there already
	// exists a strategy that can achieve this cost.
	info.max_cost = best_steps;

	StrategyTreeNode *node = FillStrategy(
		all, unguessed_mask, impossible_mask, first_guess, &progress, &info);
	// , 0, best_steps);

	return (StrategyTree*)node;
}

