
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
