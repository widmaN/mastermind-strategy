#include "CodeBreaker.hpp"
#include "ObviousStrategy.hpp"

namespace Mastermind {

// Create a free-standing function to make a guess.
Codeword MakeGuess(
	Engine &e,
	State &state,
	Strategy *strat,
	EquivalenceFilter *filter,
	const CodeBreakerOptions &options)
{
	CodewordConstRange possibilities = state.possibilities;
	size_t count = possibilities.size();
	if (count == 0)
		return Codeword::emptyValue();

	// state->NPossibilities = count;

	// Check for obvious guess.
	if (options.optimize_obvious)
	{
		Codeword guess = ObviousStrategy(e).make_guess(possibilities, possibilities);
		if (!guess.empty())
		{
			//state->NCandidates = (it - possibilities.begin()) + 1;
			//state->Guess = *it;
			return guess;
		}
	}

	// Initialize the set of candidate guesses.
	CodewordConstRange candidates = options.possibility_only?
		possibilities : e.universe();

	// Filter the candidate set to remove "equivalent" guesses.
	// For now the equivalence is determined by bit-mask of colors.
	// In the next step, we should determine it by graph isomorphasm.
	CodewordList canonical = filter ?
		filter->get_canonical_guesses(candidates) :
		CodewordList(candidates.begin(), candidates.end());
		//candidates.FilterByEquivalence(unguessed_mask, impossible_mask);
		//CodewordList canonical = e.canonicalize(candidates, filter);

	// Make a guess using the strategy provided.
	Codeword guess = strat->make_guess(possibilities, canonical);
	return guess;
}

// Build a partial strategy tree from the given state.
// That state has NOT been output to the tree yet.
// The following fields must be filled in partial_node:
//   - node.depth
//   - node.guess
//   - node.feedback
// Note: This actually can be turned into a tail recursion.
//       We can do that later if necessary.
#if 1
static void FillStrategy(
	StrategyTree &tree,
	const StrategyTree::Node &partial_node,
	Engine &e,
	State &state,
	Strategy *strat,
	EquivalenceFilter *filter,
	const CodeBreakerOptions &options,
	int *progress)
{
	// Create a node.
	StrategyTree::Node node(partial_node);
		
	// Make a guess.
	Codeword guess = MakeGuess(e, state, strat, filter, options);
	if (guess.empty())
	{
		tree.append(node);
		return;
	}

	// Fill strategy-related fields and output this node/state.
	node.npossibilities = state.possibilities.size();
	node.ncandidates = 0; // TBD
	node.suggestion = Codeword::pack(guess);
	tree.append(node);

	// Partition the possibility set using this guess.
	FeedbackFrequencyTable freq = e.partition(state.possibilities, guess);

	// Recursively fill the strategy for each non-empty cell in the partition.
	CodewordRange cell(state.possibilities.begin(), state.possibilities.begin());
	Feedback perfect = Feedback::perfectValue(e.rules());
	for (size_t k = 0; k < freq.size(); ++k)
	{
		Feedback feedback(k);
		if (freq[k] == 0)
			continue;

		// Select the possibility set with this feedback.
		cell = CodewordRange(cell.end(), cell.end() + freq[k]);

		// Prepare a node for the child state.
		StrategyTree::Node child(node.depth + 1, node.suggestion, feedback.value());

		if (feedback == perfect)
		{
			(*progress)++;
			//double pcnt = (double)(*progress) / m_all.size();
			//printf("\rProgress: %3.0f%%", pcnt*100);
			//fflush(stdout);
			tree.append(child);
		}
		else
		{
			// Create a new, child state.
			State new_state(state);
			new_state.udpate(e, guess, feedback, cell);

			// Create a child filter.
			EquivalenceFilter *new_filter = 
				filter ? filter->clone() : NULL;
			if (new_filter)
			{
				new_filter->add_constraint(guess, feedback, cell);
			}

			// Recursively build the strategy tree.
			FillStrategy(tree, child, e, new_state, strat, new_filter, options, progress);

			// Clean up resource.
			delete new_filter;
		}
	}
}
#endif

#if 1
StrategyTree BuildStrategyTree(
	Engine &e, Strategy *strat, EquivalenceFilter *filter,
	const CodeBreakerOptions &options)
{
	StrategyTree tree(e.rules());
	CodewordList all = e.generateCodewords();
	StrategyTree::Node root(0, (uint32_t)-1, 0);
	State state(e, all);
	int progress = 0;
	FillStrategy(tree, root, e, state, strat, filter, options, &progress);
	return tree;
}
#endif

// Copied from HeuristicCodeBreaker.cpp

//StrategyTreeMemoryManager *default_strat_mm = new StrategyTreeMemoryManager();

///////////////////////////////////////////////////////////////////////////
// HeuristicCodeBreaker implementation
//

#if ENABLE_CALL_COUNTER
static Utilities::CallCounter _call_counter("HeuristicCodeBreaker::MakeGuess", true);
void PrintMakeGuessStatistics()
{
	_call_counter.DebugPrint();
}
#endif


///////////////////////////////////////////////////////////////////////////
// MakeGuess() implementation.

#if 0
MakeGuess(...)
	state->NCandidates = candidates.size();
	state->Guess = candidates[choose_i];
#if ENABLE_CALL_COUNTER
	_call_counter.AddCall(state->NCandidates*state->NPossibilities);
#endif
#endif

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
#endif

} // namespace Mastermind
