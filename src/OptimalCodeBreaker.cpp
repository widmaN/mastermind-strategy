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
#include "OptimalStrategy.hpp"
#include "StrategyTree.hpp"
// #include "util/zip_iterator.hpp"
#include "util/call_counter.hpp"
#include "util/hr_timer.hpp"
#include "util/io_format.hpp"

using namespace Mastermind;

REGISTER_CALL_COUNTER(OptimalRecursion)

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
		: CodewordRange(first, last), feedback(fb) { }
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
			Feedback fb((unsigned char)j);
			range = CodewordRange(range.end(), range.end() + freq[j]);
			cells.push_back(CodewordCell(fb, range.begin(), range.end()));
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

	int n = (int)secrets.size();
	int cost = 2*n - 1;

	Feedback perfect = Feedback::perfectValue(e.rules());
	FeedbackList fbs = e.compare(guess, secrets);
	int depth = tree.currentDepth();
	for (size_t i = 0; i < fbs.size(); ++i)
	{
		StrategyTree::Node node(depth + 1, guess, fbs[i]);
		//node.npossibilities = 1;
		tree.append(node);
		if (fbs[i] != perfect)
		{
			StrategyTree::Node leaf(depth + 2, secrets[i], perfect);
			tree.append(leaf);
		}
	}

	return cost;
	// @todo: try guesses from non-secrets.
}

#define VERBOSE_COUT(text) do { if (verbose) { std::cout << std::setw(depth*2) << "" \
	<< "[" << (depth+1) << "] " << text << std::endl; } } while (0)

typedef HeuristicStrategy<Heuristics::MinimizeLowerBound> LowerBoundEstimator;

// @todo: Use heuristic code breaker to estimate an upper bound of total guesses
// This could be helpful in pruning obvious bad candidates

// Finds the optimal strategy for a given set of remaining secrets.
// Returns the optimal (least) total number of steps needed to reveal
// all the secrets, or -1 if such optimal will not be less than _best_.
// @todo Add progress report (0% - 100%)
// @todo Output strategy tree
static int fill_strategy_tree(
	Engine &e,
	CodewordRange secrets,
	EquivalenceFilter *filter,
	LowerBoundEstimator &estimator,
	int depth,     // Number of guesses already made
	int cut_off,       // Upper bound of additional cost; used for pruning
	OptimalStrategyOptions options,
	StrategyTree &tree // Strategy tree that stores the best strategy
	)
{
	UPDATE_CALL_COUNTER(OptimalRecursion, (int)secrets.size());

	// Note: Branch pruning is done based on the supplied cut-off
	// threshold _best_. It is set to an upper bound of the total
	// number of steps needed to reveal all the secrets, ignoring
	// the current depth of the tree.

	const bool verbose = (depth < 0);

	VERBOSE_COUT("Checking " << secrets.size() << " remaining secrets");

	// Short-cut if the secret set is empty or the max-depth is 0.
	if (secrets.empty())
		return 0;
	if (options.max_depth == 0)
		return -1;

	// Short-cut if there is only one secret or the max-depth is 1.
	const Feedback perfect = Feedback::perfectValue(e.rules());
	if (secrets.size() == 1)
	{
		tree.append(StrategyTree::Node(depth + 1, secrets[0], perfect));
		return 1;
	}
	if (options.max_depth == 1)
		return -1;

	// Let n be the number of secrets.
	int nsecrets = (int)secrets.size();

	// From now on, we are allowed to make at least 2 more steps
	// to reveal all the secrets. We reduce the max-steps limit
	// by one so that it can be passed to recursive routines.
	--options.max_depth;

	// If find_last is true, then we only cut-off a guess when it's
	// strictly worse than the current best; otherwise, we cut-off
	// a guess as long as it's no better than the current best.
	int cut_off_delta = (options.find_last? 1 : 0);

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
	typedef Heuristics::MinimizeLowerBound::score_t lowerbound_t;
	std::vector<lowerbound_t> scores(candidates.size());
	estimator.make_guess(secrets, candidates, &scores[0]);

	std::vector<int> order(candidates.size());
	for (size_t i = 0; i < order.size(); ++i)
		order[i] = (int)i;

	// Note: the stable-sort takes longer time than the non-stable sort.
	std::sort(order.begin(), order.end(), [&](int i, int j) -> bool {
#if 0
		return scores[i] < scores[j];
#else
		return (scores[i] < scores[j]) || (!(scores[j] < scores[i]) && (i < j));
#endif
	});
		//util::make_zip(scores.begin(), candidates.begin()),
		//util::make_zip(scores.end(), candidates.end()));

	// Initialize some state variables to store the best guess
	// so far and related cut-off thresholds.
	int best = -1;
	size_t best_pos = tree.nodes().size();

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
		if (scores[i].steps + nsecrets >= cut_off + cut_off_delta)
		{
			VERBOSE_COUT("Pruned " << (candidate_count - index)
				<< " remaining guesses: lower bound (" << scores[i].steps
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

		VERBOSE_COUT("Checking guess " << (i+1) << " of "
			<< candidate_count << " (" << guess << ") -> ");

		// If there's a limit on the maximum number of depth, 
		// check if we can prune it.
		if (scores[i].depth > options.max_depth)
		{
			if (verbose)
				std::cout << "Skipped: guess will have too many steps"
				<< std::endl;
			continue;
		}

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
			{
				lowerbound_t estimate = 
					estimator.heuristic().simple_estimate((int)cells[j].size());
				lb_part[j] = estimate.steps;
			}
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
		int node_pos = (int)tree.nodes().size(); // -1;
		for (size_t j = 0; j < cells.size() && !pruned; ++j)
		{
			const CodewordCell &cell = cells[j];

			// Add this node to the strategy tree.
			StrategyTree::Node node(depth + 1, guess, cell.feedback);
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

			// Short-cut if only one additional guess is allowed
			// but we are left with more than one secret in this
			// partition.
			if (options.max_depth == 1 && cell.size() > 1)
			{
				pruned = true;
				break;
			}

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

				cell_cost = fill_strategy_tree(e, cell, new_filter.get(), estimator,
					depth + 1, cut_off - (lb - lb_part[j]), options, tree);
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
			if (lb >= cut_off + cut_off_delta)
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
			assert(lb < cut_off + cut_off_delta);
			assert(best < 0 || lb < best + cut_off_delta);
			best = lb;
			cut_off = best;
			VERBOSE_COUT("Improved cut-off to " << best);

			// Remove the previous best tree.
			tree.erase(best_pos, node_pos);
		}
		else
		{
			// Remove all nodes added just now.
			tree.erase(node_pos, tree.size());
		}
	}
	return best;
}

static StrategyTree build_optimal_strategy_tree(Engine &e)
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
	StrategyTree::Node root;
	tree.append(root);

	// Set options.
	OptimalStrategyOptions options;
#if 1
	options.max_depth = 1000;
#else
	options.max_depth = 5;
#endif
	options.min_worst = false;
	options.find_last = false;

	// Recursively find an optimal strategy.
	LowerBoundEstimator estimator(e, Heuristics::MinimizeLowerBound(e));
	int best = fill_strategy_tree(e, all, filter.get(), estimator,
		0, 1000000, options, tree);
	std::cout << "OPTIMAL: " << best << std::endl;
	return tree;
}

// Call statistics for optimal Mastermind (p4c6r) that finds the FIRST:
// Total # of calls : 5832
// Total # of ops   : 59209
// Avg ops per call : 10.15
// Time: 1.21 s
// Total   Avg    1    2    3    4    5    6
//  5625 4.340    1    8   92  648  542    5
//
// Call statistics for optimal Mastermind (p4c6r) that finds the LAST:
// Total # of calls : 12029
// Total # of ops   : 89738
// Avg ops per call : 7.46
// Time: 2.45
// Total   Avg    1    2    3    4    5    6
//  5625 4.340    1    8   90  648  548    1
//
// This gives a lower bound and upper bound of the time needed.
// And FIND_LAST happens to return a strategy with the least worst-case
// secrets.

void test_optimal_strategy(Engine &e)
{
	util::hr_timer t1;
	t1.start();
	StrategyTree tree = 	build_optimal_strategy_tree(e);
	double t = t1.stop();

	StrategyTreeInfo info("optimal", tree, t);
	std::cout << std::endl << util::header << info;
}

