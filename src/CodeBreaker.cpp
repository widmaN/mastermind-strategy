#include "CodeBreaker.hpp"
#include "ObviousStrategy.hpp"

namespace Mastermind {

// Create a free-standing function to make a guess.
Codeword MakeGuess(
	Engine &e,
	CodewordConstRange secrets,
	Strategy *strat,
	const EquivalenceFilter *filter,
	const CodeBreakerOptions &options)
{
	size_t count = secrets.size();
	if (count == 0)
		return Codeword();

	// stat->NPossibilities = count;

	// Check for obvious guess.
	if (options.optimize_obvious)
	{
		Codeword guess = ObviousStrategy(e).make_guess(secrets, secrets);
		if (!guess.empty())
		{
			//stat->NCandidates = (it - possibilities.begin()) + 1;
			//stat->Guess = *it;
			return guess;
		}
	}

	// Initialize the set of candidate guesses.
	CodewordConstRange candidates = options.possibility_only ?
		secrets : e.universe();

	// Filter the candidate set to remove "equivalent" guesses.
	CodewordList canonical = filter->get_canonical_guesses(candidates);

	// Make a guess using the strategy provided.
	Codeword guess = strat->make_guess(secrets, canonical);
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
static void FillStrategy(
	StrategyTree &tree,
	const StrategyTree::Node &partial_node,
	Engine &e,
	CodewordRange secrets,
	Strategy *strat,
	const EquivalenceFilter *filter,
	const CodeBreakerOptions &options,
	int *progress)
{
	// Create a node.
	StrategyTree::Node node(partial_node);
		
	// Make a guess.
	Codeword guess = MakeGuess(e, secrets, strat, filter, options);
	if (guess.empty())
	{
		tree.append(node);
		return;
	}

	// Fill strategy-related fields and output this node/state.
	//node.npossibilities = secrets.size();
	//node.ncandidates = 0; // TBD
	//node.suggestion = Codeword::pack(guess);
	tree.append(node);

	// Partition the possibility set using this guess.
	CodewordPartition cells = e.partition(secrets, guess);

	// Recursively fill the strategy for each non-empty cell in the partition.
	Feedback perfect = Feedback::perfectValue(e.rules());
	for (size_t k = 0; k < cells.size(); ++k)
	{
		Feedback feedback(k);
		CodewordRange cell = cells[k];
		if (cell.empty())
			continue;

		// Prepare a node for the child state.
		StrategyTree::Node child(node.depth() + 1, guess, feedback);

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
			// Create a child filter.
			std::unique_ptr<EquivalenceFilter> new_filter(filter->clone());
			new_filter->add_constraint(guess, feedback, cell);

			// Recursively build the strategy tree.
			FillStrategy(tree, child, e, cell, strat, new_filter.get(), options, progress);
		}
	}
}

StrategyTree BuildStrategyTree(
	Engine &e, 
	Strategy *strat, 
	const EquivalenceFilter *filter,
	const CodeBreakerOptions &options)
{
	StrategyTree tree(e.rules());
	CodewordList all = e.generateCodewords();
	StrategyTree::Node root;
	int progress = 0;
	FillStrategy(tree, root, e, all, strat, filter, options, &progress);
	return tree;
}

} // namespace Mastermind
