/**
 * @defgroup type Data Types
 *
 * @defgroup algo Basic Operations
 *
 * @defgroup equiv Equivalence Filters
 *
 * @defgroup strat Strategies
 *
 * @defgroup prog Program
 *
 * @defgroup util Utility Routines
 */

#include <iostream>
#include <vector>

#include <iostream>
#include "Rules.hpp"
#include "Codeword.hpp"
//#include "CodeBreaker.h"
//#include "SimpleCodeBreaker.hpp"

using namespace Mastermind;

#if 0
static int RegressionTestUnit()
{
	Rules rules;
	rules.length = 4;
	rules.ndigits = 10;
	rules.allow_repetition = false;

	// Codeword creation
	Codeword cw;
	if (!cw.IsEmpty()) {
		printf("FAILED: Codeword::IsEmpty()\n");
		return -1;
	}

	// Enumeration
	CodewordList list;
	list = CodewordList::Enumerate(rules);
	if (!(list.GetCount() == 5040)) {
		printf("FAILED: Wrong enumeration count\n");
		return -1;
	}
	if (!(list[357].ToString() == "0741")) {
		printf("FAILED: Wrong enumeration result\n");
		return -1;
	}

	// Comparison
	FeedbackList fbl(list[0], list);
	if (!(fbl.GetCount() == 5040)) {
		printf("FAILED: Codeword::CompareTo() returns wrong length of feedback\n");
		return -1;
	}
	if (!(fbl[3] == Feedback(3, 0))) {
		printf("FAILED: Incorrect comparison results\n");
		return -1;
	}

	// Count frequency
	return 0;
}

static int RegressionTest()
{
	printf("Running regression test...\n");
	if (RegressionTestUnit() == 0) {
		printf("Passed\n");
	} else {
		printf("Failed\n");
	}

	system("PAUSE");
	return 0;
}

static void DumpCodewordList(const CodewordList *list)
{
	for (int i = 0; i < list->GetCount(); i++) {
		cout << (*list)[i].ToString() << " ";
	}
}

static void TestGuessing(Rules rules, CodeBreaker *breakers[], int nb)
{
	CodewordList all = CodewordList::Enumerate(rules);
	Feedback target(rules.length, 0);
	Utilities::HRTimer timer;

	printf("\n");
	printf("Algorithm Descriptions\n");
	printf("------------------------\n");
	for (int i = 0; i < nb; i++) {
		printf("  A%d: %s\n", (i + 1), breakers[i]->GetDescription().c_str());
	}

	printf("\n");
	printf("Frequency Table\n");
	printf("-----------------\n");
	printf("  ##:   Avg    1    2    3    4    5    6    7    8    9   >9     Time(s)\n");

	for (int ib = 0; ib < nb; ib++) {
		CodeBreaker *breaker = breakers[ib];

		// Initialize frequency table
		const int max_freq = 10;
		int freq[max_freq+1];
		for (int i = 1; i <= max_freq; i++) {
			freq[i] = 0;
		}

		int max_round = 0;
		int sum_round = 0;
		int count = all.GetCount();
		int pct = -1;

		// Test each possible secret
		timer.Start();
		for (int i = 0; i < count; i += 1) {
			if (i*100/count > pct) {
				pct = i*100/count;
				printf("\r  A%d: running... %2d%%", ib + 1, pct);
				fflush(stdout);
			}

			Codeword secret = all[i];

			int round = 0;
			breaker->Reset();
			for (round = 1; round < max_freq; round++) {
				Codeword guess = breaker->MakeGuess();
				Feedback fb = secret.CompareTo(guess);
				if (fb == target) {
					break;
				}
				breaker->AddFeedback(guess, fb);
			}

			if (round > max_round)
				max_round = round;
			sum_round += round;
			freq[round]++;
			//printf("%s : %d\n", secret.ToString().c_str(), round);
		}
		double t = timer.Stop();

		// Display statistics
		printf("\r  A%d: %5.3f ", (ib + 1), (double)sum_round / count);
		for (int i = 1; i <= max_freq; i++) {
			if (freq[i] > 0)
				printf("%4d ", freq[i]);
			else
				printf("   - ");
		}
		printf("%8.2f\n", t);

	}

}



int TestOutputStrategyTree(Rules rules)
{
	//CodeBreaker *b = new HeuristicCodeBreaker<Heuristics::MinimizeAverage>(rules);
	CodeBreaker *b = new OptimalCodeBreaker(rules);
	Codeword first_guess;
	StrategyTree *tree = b->BuildStrategyTree(first_guess);
	//const char *filename = "E:/good-strat.txt";
	char filename[100];
	sprintf_s(filename, "./strats/mm-%dp%dc-%s-%s.xml", rules.length, rules.ndigits,
		(rules.allow_repetition? "r":"nr"), b->GetName());
	FILE *fp = fopen(filename, "wt");
	tree->WriteToFile(fp, StrategyTree::XmlFormat, rules);
	fclose(fp);
	delete tree;

	system("PAUSE");
	return 0;
}

// c.f. http://www.javaworld.com.tw/jute/post/view?bid=35&id=138372&sty=1&tpg=1&ppg=1&age=0#138372

static int GetMaxBreakableWithin(
	int nsteps,
	int f[],
	CodewordList possibilities,
	CodewordList all,
	unsigned short unguessed_mask,
	int expand_levels)
{
	assert(nsteps >= 0);
	assert(expand_levels >= 0);

	if (nsteps == 0) {
		return 0;
	} else if (nsteps == 1) {
		return 1;
	}
	//else if (nsteps == 2) {
	//	int npegs = possibilities.GetRules().length;
	//	return std::min(possibilities.GetCount(), npegs*(npegs+3)/2);
	//}

	unsigned short impossible_mask = ((1<<all.GetRules().ndigits)-1) & ~possibilities.GetDigitMask();
	CodewordList candidates = all.FilterByEquivalence(unguessed_mask, impossible_mask);
	int best = 0;
	for (int i = 0; i < candidates.GetCount(); i++) {
		Codeword guess = candidates[i];

		int nguessable = 0;
		if (expand_levels == 0) {
			FeedbackFrequencyTable freq(FeedbackList(guess, possibilities));
			for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
				int partition_size = freq[Feedback(fbv)];
				nguessable += std::min(f[nsteps-1], partition_size);
			}
		} else {
			FeedbackFrequencyTable freq;
			possibilities.Partition(guess, freq);
			int partition_start = 0;
			for (int fbv = 0; fbv <= freq.GetMaxFeedbackValue(); fbv++) {
				int partition_size = freq[Feedback(fbv)];
				if (partition_size == 0)
					continue;

				CodewordList filtered(possibilities, partition_start, partition_size);
				int tmpcount = GetMaxBreakableWithin(nsteps-1, f, filtered, all,
					unguessed_mask & ~guess.GetDigitMask(),
					expand_levels-1);
				nguessable += std::min(tmpcount, partition_size);
				partition_start += partition_size;
			}
		}

		best = std::max(best, nguessable);
	}
	return best;
}

static int TestBound(Rules rules)
{
	// Find out the maximum size of any partition
	CodewordList all = CodewordList::Enumerate(rules);
	FeedbackFrequencyTable freq(FeedbackList(all[0], all));

	int f[100];
	f[0] = 0;
	f[1] = 1;
	int min_rounds = 0;
	int expand_levels = 1;
	for (int n = 2; n < 100; n++) {
		unsigned short unguessed_mask = ((1<<all.GetRules().ndigits)-1);
		f[n] = GetMaxBreakableWithin(n, f, all.Copy(), all, unguessed_mask, expand_levels);
		if (f[n] >= all.GetCount()) {
			min_rounds = n;
			break;
		}
	}
	for (int n = 1; n <= min_rounds; n++) {
		printf("Maximum number of secrets that can be guessed in %2d rounds: <= %d\n",
			n, f[n]);
	}
	printf("**** Minimum number of rounds necessary to guess all secrets: >= %d\n",
		min_rounds);

	int min_steps = all.GetCount()*min_rounds;
	for (int n = 1; n < min_rounds; n++) {
		min_steps -= f[n];
	}
	printf("**** Minimum total steps necessary to guess all secrets: >= %d\n",
		min_steps);
	printf("**** Minimum average steps necessary to guess all secrets: >= %5.3f\n",
		(double)min_steps/all.GetCount());

	int *S = Mastermind::Heuristics::MinimizeSteps::partition_score;
	S[0] = 0;
	for (int i = 1; i <= all.GetCount(); i++) {
		int nr = 0;
		for (nr = 1; nr <= 100; nr++) {
			if (f[nr] >= i)
				break;
		}
		int score = i*nr;
		for (int j = 1; j < nr; j++) {
			score -= f[j];
		}
		S[i] = score;
		//if (i % 40==0)
		//printf("S[%d] = %d\n", i, score);
	}

#if 0
	int m=14;

	const int nmax=20;
	int S[nmax+1];
	S[0]=0;
	int n;
	for (n = 1; n <= nmax; n++) {
		if (n <= m) {
			S[n] = 1+(n-1)*2;
		} else {
			S[n]=n;
			int nper = (n-1)/(m-1);
			S[n] += S[nper]*(m-2);
			S[n] += S[(n-1)-(nper*(m-2))];
		}
		printf("S[%4d] = %d\n", n, S[n]);
	}
#endif

	//system("PAUSE");
	return 0;
}
#endif

static void usage()
{
	std::cerr <<
		"Mastermind [options] [rules] [-s strategy] [action]\n"
		"Actions:\n"
		"    (none)     interactive mode\n"
		"    strat      build and output strategy tree\n"
		"    test name  run regression or benchmark tests\n"
		"Rules:\n"
		"    -p pegs    set the number of pegs [default=4]\n"
		"    -c colors  set the number of colors [default=10]\n"
		"    -n         do not allow colors to repeat [default]\n"
		"    -r         allow colors to repeat\n"
		"Strategies: (only relevant in strat mode)\n"
		"    simple     simple strategy\n"
		"    heuristic  heuristic strategy\n"
		"    optimal    optimal strategy\n"
		"Tests:\n"
		"    comp       (benchmark) comparison routines\n"
		"Options:\n"
		"    -h         display this help screen\n"
		"    -v         verbose\n"
		"";
}

// TODO: Refactor MakeGuess() code to make each call longer and fewer calls
//       to take advantage of OpenMP.
// TODO: Add progress display to OptimalCodeBreaker
// TODO: Output strategy tree after finishing a run

extern int interactive(const Rules &rules);
extern int test(const Rules &rules);
extern void pause_output();

int main(int argc, char* argv[])
{
	using namespace Mastermind;

	// Default argument values.
#if 0 // (3,5)
	int pegs = 4;
	int colors = 3;
	bool repeatable = true;
#elif 0
	int pegs = 4;
	int colors = 10;
	bool repeatable = false;
#else
	int pegs = 4;
	int colors = 6;
	bool repeatable = true;
#endif
	bool verbose = false;
	enum class Mode
	{
		Interactive = 0,
		Strategy = 1,
		Test = 2,
	};
	Mode mode = Mode::Interactive;

	// Parse command line arguments.
	for (int i = 1; i < argc; i++)
	{
		const char *s = argv[i];
		if (*s == '-') // option
		{
			switch (s[1])
			{
			case 'p':
				pegs = atoi(s+2);
				break;
			case 'c':
				colors = atoi(s+2);
				break;
			case 'r':
				repeatable = true;
				break;
			case 'n':
				repeatable = false;
				break;
			case 'h':
				usage();
				pause_output();
				return 0;
				break;
			case 'v':
				verbose = true;
				break;
			default:
				std::cerr << "Unknown option: " << s[1] << std::endl;
				usage();
				pause_output();
				break;
			}
		}
		else // mode
		{
			if (strcmp(s, "strat") == 0)
				mode = Mode::Strategy;
			else if (strcmp(s, "test") == 0)
				mode = Mode::Test;
			else
			{
				usage();
				pause_output();
				break;
			}
		}
	}

	// Check if the rule specified is valid.
	if (!(pegs > 0 && colors > 0 && (repeatable || colors >= pegs)))
	{
		std::cerr << "Error: Invalid rules: pegs=" << pegs
			<< ", colors=" << colors << ", repeatable="
			<< std::boolalpha << repeatable << std::endl;
		return 2;
	}
	if (pegs > MM_MAX_PEGS)
	{
		std::cerr << "Error: Too many pegs: max=" << MM_MAX_PEGS
			<< ", supplied=" << pegs << std::endl;
		return 2;
	}
	if (colors > MM_MAX_COLORS)
	{
		std::cerr << "Error: Too many colors: max=" << MM_MAX_COLORS
			<< ", supplied=" << pegs << std::endl;
		return 2;
	}

	// Construct the rules.
	Rules rules(pegs, colors, repeatable);

	// Execute the selected action.
	switch (mode)
	{
	case Mode::Interactive:
		return interactive(rules);
	case Mode::Strategy:
		break;
	case Mode::Test:
		return test(rules);
	default:
		break;
	}
	return 0;

#if 0
	Rules rules(4, 10, false); // Guess Number rules
	Rules rules(4, 6, true);   // Mastermind rules
	Rules rules(5, 8, false);
#endif

#if 0
	TestBound(rules);
	//return system("PAUSE");

	// There is an interesting bug with the FilterEquivalence_Rep
	// If the first guess is 0011 and feedback is 0A1B,
	// Then the filter will think 1223 and 1233 are equivalent,
	// and keep the first one only.
	// But actually, the second codeword has better score
	// in the worst-case heuristic criteria.
	// Let's find out why.
	if (0) {
		CodewordList all = CodewordList::Enumerate(rules);
		Codeword guess = Codeword::Parse("0011", rules);
		Feedback fb = Feedback(0, 1);
		CodewordList list = all.FilterByFeedback(guess, fb);

		unsigned char eqclass[16] = {
			0,1,3,4, 5,2,6,7, 8,9,10,11, 12,13,14,15 };
		codeword_t output[1296];
		int count = FilterByEquivalence_Rep(list.GetData(), list.GetCount(), eqclass, output);

		Feedback check(1, 3);
		// Find out the partition of 1223
		if (1) {
			const char *s = "1233";
			guess = Codeword::Parse(s, rules);
			FeedbackList fbl(guess, list);
			FeedbackFrequencyTable freq(fbl);
			//printf("Frequency table %s:\n", s);
			//freq.DebugPrint();
			printf("Guess=0011, Feedback=0A1B\n");
			printf("Guess=%s, Feedback=%s: ", s, check.ToString().c_str());
			list.FilterByFeedback(guess, check).DebugPrint();
			printf("\n");
		}

		// Find out the partition of 1233
		if (1) {
			const char *s = "1223";
			guess = Codeword::Parse(s, rules);
			FeedbackList fbl(guess, list);
			FeedbackFrequencyTable freq(fbl);
			//printf("Frequency table %s:\n", s);
			//freq.DebugPrint();
			printf("Guess=0011, Feedback=0A1B\n");
			printf("Guess=%s, Feedback=%s: ", s, check.ToString().c_str());
			list.FilterByFeedback(guess, check).DebugPrint();
			printf("\n");
		}

		system("PAUSE");
		return 0;
	}

	//return RegressionTest();

	//return TestOutputStrategyTree(rules);

#endif
}
