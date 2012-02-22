#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>
#include <vector>
#include <array>

#include "Engine.hpp"
#include "Strategy.hpp"
#include "Equivalence.hpp"
#include "ObviousStrategy.hpp"
#include "HeuristicStrategy.hpp"
#include "StrategyTree.hpp"
#include "util/zip_iterator.hpp"
#include "util/call_counter.hpp"

using namespace Mastermind;

REGISTER_CALL_COUNTER(OptimalRecursion)

// Returns an estimate of the minimum total number of steps required
// to reveal all the supplied secrets.
static int estimate_lower_bound(Engine &e, CodewordConstRange secrets, int look_ahead)
{
	int n = (int)secrets.size();

	// If look_ahead < 0, provide a dummy lower bound.
	if (look_ahead < 0)
		return n;

	// If look_ahead == 0, assume the branching factor is p(p+3)/2-1 always.
	// Then:
	//    1 secret  can be revealed with 1 guess,
	//   13 secrets can be revealed with 2 guesses,
	//  169 secrets can be revealed with 3 guesses, etc.
	if (look_ahead == 0)
	{
		int p = e.rules().pegs();
		int m = p*(p+3)/2-1;
		int cost = 0;
		for (int step=1, factor=1, count=0; n > count; )
		{
			cost += (n - count) * step;
			count += factor;
			factor *= m;
		}
		return cost;
	}

	return 0;
}

/// Represents the partition of a set.
#if 0
template <class TKey, size_t Capacity, class Iter>
class partition
{
public:

	struct cell
	{
		TKey key;
		Iter begin;
		Iter end;

		size_t size() const { return end - begin; }
	};

	std::array<cell,Capacity> _cells;
	int _ncells;

	template <class TVal>
	partition(
		const util::frequency_table<TKey,TVal,Capacity> &freq,
		Iter first, Iter last) : _ncells(0)
	{
		util::range<Iter> r(first, first);
		for (size_t i = 0; i < freq.size(); ++i)
		{
			if (freq[i] > 0)
			{
				r = util::range<Iter>(r.end(), r.end() + freq[j]);
				cell c;
				c.key = TKey(i);
				c.begin = r.begin();
				c.end = r.end();
				_cells[_ncells++] = c;
			}
		}
	}

	size_t size() const { return _ncells; }

	typename std::array<cell,Capacity>::iterator begin()
	{ 
		return _cells.begin(); 
	}

	typename std::array<cell,Capacity>::iterator end()
	{
		return _cells.begin() + _ncells;
	}
};
#endif

//typedef partition<Feedback,256,CodewordRange> CodewordPartition;
struct CodewordCell : public CodewordRange
{
	Feedback feedback;
	CodewordCell(Feedback fb, CodewordIterator first, CodewordIterator last)
		: feedback(fb), CodewordRange(first, last) { }
};

typedef std::vector<CodewordCell> CodewordPartition;

static CodewordPartition
partition(Engine &e, CodewordRange secrets, const Codeword &guess)
{
	FeedbackFrequencyTable freq = e.partition(secrets, guess);
	CodewordRange range(secrets.begin(), secrets.begin());
	CodewordPartition cells;
	for (size_t j = 0; j < freq.size(); ++j)
	{
		if (freq[j] > 0)
		{
			range = CodewordRange(range.end(), range.end() + freq[j]);
			cells.push_back(CodewordCell(j, range.begin(), range.end()));
		}
	}
	return cells;
}

#define VERBOSE_COUT if (verbose) std::cout << std::setw(depth*2) << "" \
	<< "[" << (depth+1) << "] "

// Finds the optimal strategy for a given set of remaining secrets.
// Returns the optimal (least) total number of steps needed to reveal
// all the secrets, or -1 if such optimal will not be less than _best_.
static int fill_strategy_tree(
	Engine &e,
	CodewordRange secrets,
	EquivalenceFilter *filter,
	int depth,     // Number of guesses already made
	int max_depth, // Maximum number of guesses allowed to make
	int best,      // Upper bound of additional cost; used for pruning
	StrategyTree &tree // Strategy tree that stores the best strategy
	)
{
	UPDATE_CALL_COUNTER(OptimalRecursion, secrets.size());

	// Note: Branch pruning is done based on the supplied cut-off 
	// threshold _best_. It is set to an upper bound of the total 
	// number of steps needed to reveal all the secrets, ignoring
	// the current depth of the tree.

	//const bool verbose = true;
	const bool verbose = (depth < 1);
	
	VERBOSE_COUT << "Checking " << secrets.size() << " remaining secrets" 
		<< std::endl;

	if (secrets.empty())
		return 0;

	const Feedback perfect = Feedback::perfectValue(e.rules());

	// Optimize when there are only a few secrets.
	Codeword obvious = ObviousStrategy(e).make_guess(secrets, secrets);
	if (!obvious.empty())
	{
		VERBOSE_COUT << "Found obvious guess: " << obvious << std::endl;

		int n = secrets.size();
		int extra = 2*n - 1;
		if (extra >= best)
		{
			VERBOSE_COUT << "Pruned because the exact cost (" << extra
				<< ") >= best (" << best << ")" << std::endl;
			return -1;
		}

		FeedbackList fbs = e.compare(obvious, secrets);
		for (size_t i = 0; i < fbs.size(); ++i)
		{
			StrategyTree::Node node(depth+1, Codeword::pack(obvious), fbs[i].value());
			//node.npossibilities = 1;
			tree.append(node);
			if (fbs[i] != perfect)
			{
				StrategyTree::Node leaf(
					depth+2, Codeword::pack(secrets.begin()[i]), perfect.value());
				tree.append(leaf);
			}
		}
		return extra;
	}
	// @todo: try guesses from non-secrets.

	// For a Mastermind game, all secrets can be revealed within five
	// guesses (though an optimal strategy requires 6 guesses in one
	// case). This means when we are about to make the 5th guess, we
	// must have already determined the secret, i.e. there is only one
	// possibility left. This in turn means when we are about to make 
	// the 4th guess, this guess must be able to partition the remaining
	// secrets into a discrete partition. If this condition is not
	// satisfied, we do not need to proceed no more.
	if (depth >= max_depth - 2)
	{
		VERBOSE_COUT << "Pruned because this branch is not possible to "
			<< "finish within in " << max_depth << " steps." << std::endl;
		return -1;
	}

	// Filter canonical guesses as candidates.
	CodewordList candidates = filter->get_canonical_guesses(e.universe());
	VERBOSE_COUT << "Found " << candidates.size() << " canonical guesses."
		<< std::endl;

	// Sort the candidates by a heuristic value, so that more "promising"
	// candidates are processed first. This helps to find a good upper
	// bound quickly.
	typedef HeuristicStrategy<Heuristics::MinimizeAverage> heuristic_type;
	std::vector<heuristic_type::score_type> scores(candidates.size());
#if 0
	heuristic_type h(e);
	h.make_guess(secrets, candidates, &scores[0]);
#else
	heuristic_type(e).make_guess(secrets, candidates, &scores[0]);
#endif
	std::sort(
		util::make_zip(scores.begin(), candidates.begin()),
		util::make_zip(scores.end(), candidates.end()));

	// @todo Instead of sorting by a heuristic, we could instead
	// sort directly by the estimated lower bound of the cost.
	// This has two benefits:
	// 1, if the estimate is good enough, the sorting would be
	//    more "promising".
	// 2, once we find the lower bound of one guess to be greater
	//    than the current best, we can ignore all guesses that
	//    follows.

	// Initialize some state variables to store the best guess
	// so far and related cut-off thresholds.
	// int best = std::numeric_limits<int>::max();

	// Try each candidate guess.
	for (auto it = candidates.begin(); it != candidates.end(); ++it)
	{
		Codeword guess = *it;

		VERBOSE_COUT << "Checking guess " << (it-candidates.begin()+1) 
			<< " of " << candidates.size() << " (" << guess << ") -> ";
		//	<< std::endl;

		// Partition the remaining secrets using this guess.
		CodewordPartition cells = partition(e, secrets, guess);

		// Skip this guess if it generates only one partition.
		if (cells.size() == 1)
		{
			if (verbose)
				std::cout << "Skipped: guess produces unit partition"
				<< std::endl;
			continue;
		}

		// Sort the partitions by their size, so that smaller 
		// partitions are processed first. This makes sure we can
		// update more lower bounds earlier.
		std::sort(cells.begin(), cells.end(), 
			[](const CodewordCell &c1, const CodewordCell &c2) -> bool
		{
			return c1.size() < c2.size();
		});

		// Estimate a lower bound of the number of steps required 
		// to reveal the secrets in each partition. If the total
		// lower bound is greater than the cut-off threshold, we 
		// can skip this guess.
		int lb_part[256];
		int lb = (int)secrets.size(); // each secret takes 1 initial guess
		for (size_t j = 0; j < cells.size(); ++j)
		{
			if (cells[j].feedback == perfect)
				lb_part[j] = 0;
			else
				lb_part[j] = estimate_lower_bound(e, cells[j], 0);
			lb += lb_part[j];
		}
		if (lb >= best)
		{
			if (verbose)
				std::cout << "Pruned: lower bound (" << lb 
				<< ") >= best (" << best << ")" 	<< std::endl;
			continue;
		}

		if (verbose)
			std::cout << cells.size() << " cells" << std::endl;

		// Find the best guess for each partition.
		bool pruned = false;
		for (size_t j = 0; j < cells.size() && !pruned; ++j)
		{
			CodewordCell cell = cells[j];

			// Add this node to the strategy tree.
			StrategyTree::Node node(depth+1, Codeword::pack(guess), cell.feedback.value());
			tree.append(node);

			// Do not recurse for a perfect match.
			if (cell.feedback == Feedback::perfectValue(e.rules()))
				continue;

			VERBOSE_COUT << "Checking cell " << cell.feedback << std::endl;

			std::unique_ptr<EquivalenceFilter> new_filter(filter->clone());
			new_filter->add_constraint(guess, cell.feedback, cell);

			int cell_size = (int)cell.size();
			int best_part = fill_strategy_tree(e, cell, new_filter.get(), 
				depth + 1, max_depth, best - (lb - lb_part[j]), tree);
			
			if (best_part < 0) // The branch was pruned.
			{
				VERBOSE_COUT << "Pruned this guess because the recursion returns -1."
					<< std::endl;
				pruned = true;
				break;
			}

			// Refine the lower bound estimate.
			lb += (best_part - lb_part[j]);
			lb_part[j] = best_part;
			if (lb >= best)
			{
				VERBOSE_COUT << "Skipping " << (cells.size()-j-1) << " remaining "
					<< "partitions because lower bound (" << lb << ") >= best ("
					<< best << ")" << std::endl;
				pruned = true;
				break;
			}
		}

		// Now the guess is either pruned, or is the best guess so far.
		if (!pruned)
		{
			best = lb;
			VERBOSE_COUT << "Improved upper bound to " << best << std::endl;
		}
	}
	return best;
}

static void build_optimal_strategy_tree(Engine &e)
{
	CodewordList all = e.generateCodewords();

	// Create a "suitable" equivalence filter.
	std::unique_ptr<EquivalenceFilter> filter(
#if 0
		//RoutineRegistry<CreateEquivalenceFilterRoutine>::get("Dummy")(e)
		//RoutineRegistry<CreateEquivalenceFilterRoutine>::get("Constraint")(e)
		RoutineRegistry<CreateEquivalenceFilterRoutine>::get("Color")(e)
#else
		new CompositeEquivalenceFilter(
			RoutineRegistry<CreateEquivalenceFilterRoutine>::get("Color")(e),
			RoutineRegistry<CreateEquivalenceFilterRoutine>::get("Constraint")(e))
#endif
		);

	// Create a strategy tree and add a level-0 root node.
	StrategyTree tree(e.rules());
	StrategyTree::Node root(0, 0, 0);
	tree.append(root);

	// Recursively find an optimal strategy.
	int best = fill_strategy_tree(e, all, filter.get(), 0, 10, 1000000, tree);
	std::cout << "OPTIMAL: " << best << std::endl;
}

void test_optimal_strategy(Engine &e)
{
	build_optimal_strategy_tree(e);
}

#if 0

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

#endif
