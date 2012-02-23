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
// #include "util/zip_iterator.hpp"
#include "util/call_counter.hpp"

using namespace Mastermind;

REGISTER_CALL_COUNTER(OptimalRecursion)

class LowerBoundEstimator
{
	Engine &e;
	std::vector<int> _cache;

public:
	
	// Returns a simple estimate of minimum total number of steps 
	// required to reveal @c n secrets given a branching factor of @c b.
	static int simple_estimate(
		int _n, // Number of remaining secrets
		int b  // Branching factor: number of distinct non-perfect feedbacks
		)
	{
		int cost = 0;
		for (int remaining = _n, count = 1; remaining > 0; )
		{
			cost += remaining;
			remaining -= count;
			count *= b;
		}
		return cost;
	}

	LowerBoundEstimator(Engine &engine) 
		: e(engine), _cache(e.rules().size()+1)
	{
		// Build a cache of simple estimates
		int p = e.rules().pegs();
		int b = p*(p+3)/2-1;
		for (size_t n = 0; n < _cache.size(); ++n)
		{
			_cache[n] = simple_estimate(n, b);
		}
	}

	// Returns a simple estimate of minimum total number of steps 
	// required to reveal @c n secrets, assuming the maximum branching
	// factor for the game.
	int simple_estimate(int n) const
	{
#if 0
		int p = e.rules().pegs();
		int b = p*(p+3)/2-1;
		return simple_estimate(n, b);
#else
		assert(n >= 0 && n < (int)_cache.size());
		return _cache[n];
#endif
	}

	// Computes a lower bound for each candidate guess.
	void estimate_candidates(
		CodewordConstRange guesses,
		CodewordConstRange secrets,
		int lower_bound[]) const
	{
		const int secret_count = secrets.size();
		const int guess_count = guesses.size();
		const int table_size = (int)Feedback::maxValue(e.rules()) + 1;
		std::vector<unsigned int> frequency_cache(table_size*guess_count);

		// Partition the secrets using each candidate guess.
		//int max_b = 0; // maximum branching factor of non-perfect feedbacks
		Feedback perfect = Feedback::perfectValue(e.rules());
		for (int i = 0; i < guess_count; ++i)
		{
			Codeword guess = guesses[i];
			FeedbackFrequencyTable freq = e.frequency(e.compare(guess, secrets));
			
			std::copy(freq.begin(), freq.end(), frequency_cache.begin() + i*table_size);

#if 0
			int b = freq.nonzero_count();
			if (freq[perfect.value()] > 0)
				--b;
			if (b > max_b)
				max_b = b;
#endif
		}
		// assert(frequency_cache.size() == table_size*guess_count);

		// For each guess, compute the lower bound of total number of steps
		// required to reveal all secrets starting from that guess.
		for (int i = 0; i < guess_count; ++i)
		{
			int lb = secret_count;
			int j0 = i * table_size;
			for (int j = 0; j < table_size; ++j)
			{
				if (j != perfect.value())
				{
					// Note: we must NOT use max_b because canonical guesses
					// can change after we make a guess. So max_b no longer
					// works.
					//lb += simple_estimate(frequency_cache[j0+j], max_b);
					lb += simple_estimate(frequency_cache[j0+j]);
				}
			}
			lower_bound[i] = lb;
		}
	}
};

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

// Searches for an obviously optimal strategy.
// If one exists, returns the total cost of the strategy.
// Otherwise, returns -1.
static int fill_obviously_optimal_strategy(
	Engine &e,
	CodewordRange secrets,
	StrategyTree &tree // Strategy tree that stores the best strategy
	)
{
	Codeword guess = ObviousStrategy(e).make_guess(secrets, secrets);
	if (guess.empty())
		return -1;

	//	VERBOSE_COUT << "Found obvious guess: " << obvious << std::endl;

	int n = secrets.size();
	int cost = 2*n - 1;

	Feedback perfect = Feedback::perfectValue(e.rules());
	FeedbackList fbs = e.compare(guess, secrets);
	int depth = tree.currentDepth();
	for (size_t i = 0; i < fbs.size(); ++i)
	{
		StrategyTree::Node node(depth+1, Codeword::pack(guess), fbs[i].value());
		//node.npossibilities = 1;
		tree.append(node);
		if (fbs[i] != perfect)
		{
			StrategyTree::Node leaf(
				depth+2, Codeword::pack(secrets[i]), perfect.value());
			tree.append(leaf);
		}
	}

	return cost;
	// @todo: try guesses from non-secrets.
}

#define VERBOSE_COUT(text) do { if (verbose) { std::cout << std::setw(depth*2) << "" \
	<< "[" << (depth+1) << "] " << text << std::endl; } } while (0)

// Finds the optimal strategy for a given set of remaining secrets.
// Returns the optimal (least) total number of steps needed to reveal
// all the secrets, or -1 if such optimal will not be less than _best_.
static int fill_strategy_tree(
	Engine &e,
	CodewordRange secrets,
	EquivalenceFilter *filter,
	LowerBoundEstimator &estimator,
	int depth,     // Number of guesses already made
	int max_depth, // Maximum number of guesses allowed to make
	int cut_off,       // Upper bound of additional cost; used for pruning
	StrategyTree &tree // Strategy tree that stores the best strategy
	)
{
	UPDATE_CALL_COUNTER(OptimalRecursion, secrets.size());

	// Note: Branch pruning is done based on the supplied cut-off 
	// threshold _best_. It is set to an upper bound of the total 
	// number of steps needed to reveal all the secrets, ignoring
	// the current depth of the tree.

	const bool verbose = (depth < 0);
	
	VERBOSE_COUT("Checking " << secrets.size() << " remaining secrets");

	if (secrets.empty())
		return 0;

	const Feedback perfect = Feedback::perfectValue(e.rules());

	// For a Mastermind game, all secrets can be revealed within five
	// guesses (though an optimal strategy requires 6 guesses in one
	// case). This means when we are about to make the 5th guess, we
	// must have already determined the secret, i.e. there is only one
	// possibility left. This in turn means when we are about to make 
	// the 4th guess, this guess must be able to partition the remaining
	// secrets into a discrete partition. If this condition is not
	// satisfied, we do not need to proceed no more.
#if 0
	if (depth >= max_depth - 2)
	{
		VERBOSE_COUT("Pruned because this branch is not possible to "
			<< "finish within in " << max_depth << " steps.");
		return -1;
	}
#endif

	// Filter canonical guesses as candidates.
	CodewordList candidates = filter->get_canonical_guesses(e.universe());
	VERBOSE_COUT("Found " << candidates.size() << " canonical guesses.");

	// Compute a lower bound of the cost for each candidate guess.
	// Then sort the candidates by this lower bound so that more
	// "promising" candidates are processed first. This helps to
	// improve the upper bound as early as possible.
	std::vector<int> scores(candidates.size());
	estimator.estimate_candidates(candidates, secrets, &scores[0]);

	std::vector<int> order(candidates.size());
	for (size_t i = 0; i < order.size(); ++i)
		order[i] = i;

	std::sort(order.begin(), order.end(), [&](int i, int j) -> bool {
		return scores[i] < scores[j];
	});
		//util::make_zip(scores.begin(), candidates.begin()),
		//util::make_zip(scores.end(), candidates.end()));

	// Initialize some state variables to store the best guess
	// so far and related cut-off thresholds.
	int best = -1;

	// Try each candidate guess.
	size_t candidate_count = candidates.size();
	for (size_t index = 0; index < candidate_count; ++index)
	{
		size_t i = order[index];
		Codeword guess = candidates[i];

		// Since we keep improving the upper bound dynamically,
		// and we sort the candidates by their lower bound,
		// we need to check here whether the remaining candidates
		// are still worth checking.
#if 1
		if (scores[i] >= cut_off)
		{
			VERBOSE_COUT("Pruned " << (candidate_count - index)
				<< " remaining guesses: lower bound (" << scores[i]
				<< ") >= cut-off (" << cut_off << ")");
#if 0
			if (i == 0)
			{
				std::cout << "Very first pruned: best lower bound = "
					<< scores[i] << ", cut-off = " << cut_off << std::endl;
			}
#endif
			break;
		}
#endif

		VERBOSE_COUT("Checking guess " << (i+1) << " of " 
			<< candidate_count << " (" << guess << ") -> ");

		// Partition the remaining secrets using this guess.
		// Note that after successive calls to @c partition,
		// the order of the secrets are shuffled. However, 
		// that should not impact the result.
		CodewordPartition cells = partition(e, secrets, guess);

		// Skip this guess if it generates only one partition.
		if (cells.size() == 1)
		{
			if (verbose)
				std::cout << "Skipped: guess produces unit partition"
				<< std::endl;
			continue;
		}

#if 1
		// Sort the partitions by their size, so that smaller partitions
		// (i.e. smaller search trees) are processed first. This helps 
		// to improve the lower bound at an earlier stage.
		std::sort(cells.begin(), cells.end(), 
			[](const CodewordCell &c1, const CodewordCell &c2) -> bool
		{
			return c1.size() < c2.size();
		});
#endif

		// Estimate a lower bound of the number of steps required 
		// to reveal the secrets in each partition. If the total
		// lower bound is greater than the cut-off threshold, we 
		// can skip this guess.
		int lb_part[256]; // note: lb_part doesn't count the initial guess
		int lb = (int)secrets.size(); // each secret takes 1 initial guess
		for (size_t j = 0; j < cells.size(); ++j)
		{
			if (cells[j].feedback == perfect)
				lb_part[j] = 0;
			else
				//lb_part[j] = estimate_lower_bound(e, cells[j], 0);
				lb_part[j] = estimator.simple_estimate(cells[j].size());
			lb += lb_part[j];
		}

		// Since this is a double-computation, we shouldn't be pruning
		// this guess here.
		assert(lb == scores[i]);

		if (verbose)
			std::cout << cells.size() << " cells, lower bound = "
			<< lb << ", best = " << best << std::endl;

		// Find the best guess for each partition.
		bool pruned = false;
		for (size_t j = 0; j < cells.size() && !pruned; ++j)
		{
			const CodewordCell &cell = cells[j];

			// Add this node to the strategy tree.
			StrategyTree::Node node(depth+1, Codeword::pack(guess), cell.feedback.value());
			tree.append(node);

			// Do not recurse for a perfect match.
			if (cell.feedback == perfect)
			{
				VERBOSE_COUT("- Checking cell " << cell.feedback 
					<< " -> perfect");
				continue;
			}

			VERBOSE_COUT("- Checking cell " << cell.feedback 
				<< " -> lower bound = " << lb_part[j]);

			// If there's an obviously optimal guess for this cell,
			// use it.
			int cell_cost = fill_obviously_optimal_strategy(e, cell, tree);
			if (cell_cost >= 0)
			{
				//VERBOSE_COUT("- Checking cell " << cell.feedback 
				//	<< " -> found obvious guess");
				VERBOSE_COUT("  Found obvious guess");
			}
			else
			{
				//VERBOSE_COUT("- Checking cell " << cell.feedback 
				//	<< " -> lower bound = " << lb);
				//VERBOSE_COUT("- Checking cell " << cell.feedback 
				//	<< " -> lower bound = " << lb_part[j]);

				std::unique_ptr<EquivalenceFilter> new_filter(filter->clone());
				new_filter->add_constraint(guess, cell.feedback, cell);

				int cell_size = (int)cell.size();
				cell_cost = fill_strategy_tree(e, cell, new_filter.get(), estimator,
					depth + 1, max_depth, cut_off - (lb - lb_part[j]), tree);			
			}

			if (cell_cost < 0) // The branch was pruned.
			{
				VERBOSE_COUT("Pruned this guess because the recursion returns -1.");
				pruned = true;
				break;
			}

#if 1
			if (cell_cost > lb_part[j])
				VERBOSE_COUT("  Cell optimal cost is " << cell_cost);
			else if (cell_cost < lb_part[j])
				VERBOSE_COUT("  ERROR: LOWER BOUND IS HIGHER THAN ACTUAL COST.");
			else
				VERBOSE_COUT("  Lower bound unchanged.");
#endif

			// Refine the lower bound estimate.
			lb += (cell_cost - lb_part[j]);
			lb_part[j] = cell_cost;
			if (lb >= cut_off)
			{
				VERBOSE_COUT("Skipping " << (cells.size()-j-1) << " remaining "
					<< "partitions because lower bound (" << lb << ") >= best ("
					<< best << ")");
				pruned = true;
				break;
			}
		}

		// Now the guess is either pruned, or is the best guess so far.
		if (!pruned)
		{
			assert(lb < cut_off);
			assert(best < 0 || lb < best);
			best = lb;
			cut_off = best;
			VERBOSE_COUT("Improved cut-off to " << best);
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
	LowerBoundEstimator estimator(e);
	int best = fill_strategy_tree(e, all, filter.get(), estimator,
		0, 100, 1000000, tree);
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
