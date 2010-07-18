#include <assert.h>
#include <stdio.h>

#include "Codeword.h"
#include "CodeBreaker.h"

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

void CodeBreaker::BuildStrategyTree(StrategyTree *tree)
{
	Codeword first_guess = Codeword::Empty();
	BuildStrategyTree(tree, first_guess);
}

///////////////////////////////////////////////////////////////////////////
// SimpleCodeBreaker implementations
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

void SimpleCodeBreaker::FillStrategy(StrategyTree *tree, CodewordList possibilities, const Codeword &first_guess)
{
	Codeword guess = first_guess.IsEmpty()? MakeGuess(possibilities) : first_guess;
	FeedbackList fbl(guess, possibilities);
	FeedbackFrequencyTable freq;
	fbl.CountFrequencies(&freq);

	for (unsigned char fbv = 0; fbv < MM_FEEDBACK_COUNT; fbv++) {
		Feedback fb(fbv);
		if (freq[fb] > 0) {
			tree->Push(guess, fb);
			if (fb != Feedback(m_rules.length, 0)) {
				Codeword t = Codeword::Empty();
				FillStrategy(tree, possibilities.FilterByFeedback(guess, fb), t);
			}
			tree->Pop();
		}
	}
}

void SimpleCodeBreaker::BuildStrategyTree(StrategyTree *tree, const Codeword& first_guess)
{
	CodewordList all = m_all.Copy();
	tree->Clear();
	FillStrategy(tree, all, first_guess);
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
// MinimaxCodeBreaker implementations
//

const char * HeuristicCodeBreaker::GetName() const
{
	const char *s;
	switch (m_criteria) {
	default:
	case DefaultCriteria:
	case MinimizeAverage:
		s = "minavg";
		break;
	case MinimizeWorstCase:
		s = "minmax";
		break;
	case MaximizeEntropy:
		s = "entropy";
		break;
	case MaximizeParts:
		s = "maxparts";
		break;
	}
	return s;
}

std::string HeuristicCodeBreaker::GetDescription() const
{
	std::string s;

	switch (m_criteria) {
	default:
	case DefaultCriteria:
	case MinimizeAverage:
		s = "min_avg:minimizes the expected number of possibilities";
		break;
	case MinimizeWorstCase:
		s = "min_max:minimizes the worst-case number of possibilities";
		break;
	case MaximizeEntropy:
		s = "entropy:minimizes the expected number of guesses (entropy method)";
		break;
	case MaximizeParts:
		s = "max_parts:maximizes the number of partitions after a guess";
		break;
	}

	if (m_posonly) {
		s += ", chosen from possibility only";
	}
	return s;
}

void HeuristicCodeBreaker::FillStrategy(
	StrategyTree *tree, 
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	const Codeword& first_guess,
	int *progress)
{
	Codeword guess = first_guess.IsEmpty()?
		MakeGuess(possibilities, unguessed_mask, impossible_mask) : first_guess;
	FeedbackList fbl(guess, possibilities);
	FeedbackFrequencyTable freq;
	fbl.CountFrequencies(&freq);

	for (unsigned char fbv = 0; fbv < MM_FEEDBACK_COUNT; fbv++) {
		Feedback fb(fbv);
		if (freq[fb] > 0) {
			tree->Push(guess, fb);
			if (fb != Feedback(m_rules.length, 0)) {
				Codeword t = Codeword::Empty();
				CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
				FillStrategy(tree, filtered,
					unguessed_mask & ~guess.GetDigitMask(),
					((1<<m_rules.ndigits)-1) & ~filtered.GetDigitMask(),
					t,
					progress);
			} else {
				(*progress)++;
				double pcnt = (double)(*progress) / m_all.GetCount();
				printf("\rProgress: %3.0f%%", pcnt*100);
				fflush(stdout);
			}
			tree->Pop();
		}
	}
}

void HeuristicCodeBreaker::BuildStrategyTree(StrategyTree *tree, const Codeword& first_guess)
{
	CodewordList all = m_all.Copy();
	tree->Clear();
	unsigned short impossible_mask = 0;
	unsigned short unguessed_mask = (1 << m_rules.ndigits) - 1;
	int progress = 0;
	FillStrategy(tree, all, unguessed_mask, impossible_mask, first_guess, &progress);
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

Codeword HeuristicCodeBreaker::MakeGuess()
{
	if (m_possibilities.GetCount() <= 2) {
		return m_possibilities[0];
	}

	// Calculate the worst-case number of possibilities for each guess
	CodewordList candidates = m_posonly? m_possibilities : m_all;
	candidates = candidates.FilterByEquivalence(m_unguessed, m_impossible);
	//if (m_all.GetCount() == m_possibilities.GetCount()) {
	//	return m_possibilities[0];
	//}

	double choose_dscore = 1.0e10;
	int choose_score = 0x7fffffff;
	int choose_i = -1;
	int choose_ispos = false;
	Feedback target = Feedback(m_rules.length, 0);
	FeedbackList fbl(m_possibilities.GetCount());
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];
		guess.CompareTo(m_possibilities, fbl);
		FeedbackFrequencyTable freq;
		fbl.CountFrequencies(&freq);

		// Evaluate each potential guess, and find the minimum
		int score = 0;
		double dscore = 0.0;
		bool use_double = false;
		switch (m_criteria) {
		default:
		case DefaultCriteria:
		case MinimizeAverage:
			score = freq.GetSumOfSquares();
			break;
		case MinimizeWorstCase:
			score = freq.GetMaximum();
			break;
		case MaximizeEntropy:
			dscore = freq.GetModifiedEntropy();
			use_double = true;
			break;
		case MaximizeParts:
			score = -freq.GetPartitionCount();
			break;
		}

		if (use_double) {
			if ((dscore < choose_dscore) || 
				(score == choose_dscore && !choose_ispos && freq[target] > 0)) {
				choose_dscore = dscore;
				choose_i = i;
				choose_ispos = (freq[target] > 0);
			}
		} else {
			if ((score < choose_score) || 
				(score == choose_score && !choose_ispos && freq[target] > 0)) {
				choose_score = score;
				choose_i = i;
				choose_ispos = (freq[target] > 0);
			}
		}
	}
	return candidates[choose_i];
}

Codeword HeuristicCodeBreaker::MakeGuess(
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask)
{
	assert(possibilities.GetCount() > 0);

	if (possibilities.GetCount() <= 2) {
		return possibilities[0];
	}

	// Calculate a score for each guess
	CodewordList candidates = m_posonly? possibilities : m_all;
	candidates = candidates.FilterByEquivalence(unguessed_mask, impossible_mask);

	float choose_dscore = 1.0e10;
	int choose_score = 0x7fffffff;
	int choose_i = -1;
	int choose_ispos = false;
	Feedback target = Feedback(m_rules.length, 0);
	FeedbackList fbl(possibilities.GetCount());
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];
		guess.CompareTo(possibilities, fbl);
		FeedbackFrequencyTable freq;
		fbl.CountFrequencies(&freq);

		// Evaluate each potential guess, and find the minimum
		int score = 0;
		float dscore = 0.0;
		bool use_double = false;
		switch (m_criteria) {
		default:
		case DefaultCriteria:
		case MinimizeAverage:
			score = freq.GetSumOfSquares();
			break;
		case MinimizeWorstCase:
			score = freq.GetMaximum();
			break;
		case MaximizeEntropy:
			dscore = freq.GetModifiedEntropy();
			use_double = true;
			break;
		case MaximizeParts:
			score = -freq.GetPartitionCount();
			break;
		}

		if (use_double) {
			if ((dscore < choose_dscore) || 
				(score == choose_dscore && !choose_ispos && freq[target] > 0)) {
				choose_dscore = dscore;
				choose_i = i;
				choose_ispos = (freq[target] > 0);
			}
		} else {
			if ((score < choose_score) || 
				(score == choose_score && !choose_ispos && freq[target] > 0)) {
				choose_score = score;
				choose_i = i;
				choose_ispos = (freq[target] > 0);
			}
		}
	}
	return candidates[choose_i];
}


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

void OptimalCodeBreaker::FillStrategy(
	StrategyTree *tree, 
	CodewordList possibilities,
	unsigned short unguessed_mask,
	unsigned short impossible_mask,
	const Codeword& first_guess,
	int *progress)
{
	Codeword guess = first_guess.IsEmpty()?
		MakeGuess(possibilities, unguessed_mask, impossible_mask) : first_guess;
	FeedbackFrequencyTable freq;
	if (1) {
		FeedbackList fbl(guess, possibilities);
		fbl.CountFrequencies(&freq);
	}

	Feedback perfect = Feedback(m_rules.length, 0);
	for (unsigned char fbv = 0; fbv < MM_FEEDBACK_COUNT; fbv++) {
		Feedback fb(fbv);
		if (freq[fb] > 0) {
			tree->Push(guess, fb);
			if (fb != perfect) {
				Codeword t = Codeword::Empty();
				CodewordList filtered = possibilities.FilterByFeedback(guess, fb);
				FillStrategy(tree, filtered,
					unguessed_mask & ~guess.GetDigitMask(),
					((1<<m_rules.ndigits)-1) & ~filtered.GetDigitMask(),
					t,
					progress);
			} else {
				(*progress)++;
				//double pcnt = (double)(*progress) / m_all.GetCount();
				//printf("\rProgress: %3.0f%%", pcnt*100);
				//fflush(stdout);
			}
			tree->Pop();
		}
	}
}

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
		FeedbackFrequencyTable freq;
		if (1) {
			FeedbackList fbl(guess, possibilities);
			fbl.CountFrequencies(&freq);
		}
		//assert(freq.GetPartitionCount() > 1);
		if (freq.GetPartitionCount() <= 1) {
			continue;
		}

		// Find the "best" guess route for each possible feedback
		int total_steps = 0;
		int depth = (*pdepth) + 1;
		for (unsigned char j = 0; j < MM_FEEDBACK_COUNT; j++) {
			Feedback fb = Feedback(j);
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

void OptimalCodeBreaker::BuildStrategyTree(StrategyTree *tree, const Codeword& first_guess)
{
	CodewordList all = m_all.Copy();
	tree->Clear();
	unsigned short impossible_mask = 0;
	unsigned short unguessed_mask = m_rules.GetFullDigitMask();
	int progress = 0;
	FillStrategy(tree, all, unguessed_mask, impossible_mask, first_guess, &progress);
}

