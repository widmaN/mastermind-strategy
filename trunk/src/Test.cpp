/// \file Test.cpp
/// Routines for testing the algorithms.

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <malloc.h>
#include <iomanip>
#include <memory>

#include "Rules.hpp"
#include "Codeword.hpp"
#include "Feedback.hpp"
#include "Algorithm.hpp"
#include "Engine.hpp"
#include "CodeBreaker.hpp"
#include "SimpleStrategy.hpp"
#include "HeuristicStrategy.hpp"
#include "OptimalStrategy.hpp"
#include "Equivalence.hpp"

#include "util/call_counter.hpp"
#include "util/hr_timer.hpp"
#include "util/io_format.hpp"

using namespace Mastermind;

void pause_output()
{
#ifdef _WIN32
	system("PAUSE");
#endif
}

// Dummy test driver that does nothing in the test and always returns success.
template <class Routine>
struct TestDriver
{
	Engine &e;
	Routine f;

	TestDriver(Engine &engine, Routine func) : e(engine), f(func) { }

	bool operator () () const { return true; }
};

// Compares the running time of two routines.
template <class Routine>
bool compareRoutines(
	Engine &e,
	const char *routine1,
	const char *routine2,
	long times)
{
	Routine func1 = RoutineRegistry<Routine>::get(routine1);
	Routine func2 = RoutineRegistry<Routine>::get(routine2);

	TestDriver<Routine> drv1(e, func1);
	TestDriver<Routine> drv2(e, func2);

	// Verify computation results.
	drv1();
	drv2();
	if (!(drv1 == drv2))
		return false;

	// Display results in debug mode.
	if (times == 0)
	{
		std::cout << "Result 1: " << std::endl << drv1 << std::endl;
		std::cout << "Result 2: " << std::endl << drv2 << std::endl;
	}

	// Time it.
	util::hr_timer timer;
	double t1 = 0, t2 = 0;

	for (int pass = 0; pass < 10; pass++)
	{
		timer.start();
		for (int k = 0; k < times / 10; k++)
			drv1();
		t1 += timer.stop();

		timer.start();
		for (int k = 0; k < times / 10; k++)
			drv2();
		t2 += timer.stop();
	}

	printf("Algorithm 1: %6.3f\n", t1);
	printf("Algorithm 2: %6.3f\n", t2);
	printf("Throughput Ratio: %5.2fX\n", (t1/t2));
	return true;
}

// Codeword generation benchmark.
// Test: Generate all codewords of 4 pegs, 10 colors, and no repeats.
//       Total 5040 items in each run.
// Results: (100,000 runs, Release mode)
// LexOrder: 4.43 s
// CombPerm: 8.54 s [legacy]
// CombPermParallel:  0.96 s [ASM][legacy]
// CombPermParallel2: 0.68 s [ASM][legacy]
template <> class TestDriver<GenerationRoutine>
{
	Engine &e;
	GenerationRoutine f;
	size_t count;
	CodewordList list;

public:
	TestDriver(Engine &engine, GenerationRoutine func)
		: e(engine), f(func), count(f(e.rules(), 0)), list(count) { }

	void operator()() { 	f(e.rules(), list.data()); }

	bool operator == (const TestDriver &r) const
	{
		return list == r.list;
	}

	friend std::ostream& operator << (std::ostream &os, const TestDriver &r)
	{
		// Display first 10 items.
		os << "First 10 of " << r.list.size() << " items:" << std::endl;
		for (size_t i = 0; i < 10 && i < r.list.size(); ++i)
			os << r.list[i] << std::endl;
		return os;
	}
};

// Codeword comparison benchmark.
// Test:     Compare a given codeword to 5040 non-repeatable codewords.
// Results:  (100,000 runs, Win32, VC++ 2011)
// generic:  1.68 s
// norepeat: 0.62 s
template <> class TestDriver<ComparisonRoutine>
{
	Engine &e;
	ComparisonRoutine f;
	CodewordList codewords;
	size_t count;
	Codeword secret;
	FeedbackList feedbacks;

public:
	TestDriver(Engine &env, ComparisonRoutine func)
		: e(env), f(func), codewords(e.generateCodewords()),
		count(codewords.size()), secret(codewords[count/2]),
		feedbacks(count) { }

	void operator()()
	{
		f(e.rules(), secret, &codewords.front(), &codewords.back()+1,
			feedbacks.data());
	}

	bool operator == (const TestDriver &r) const
	{
		if (count != r.count)
		{
			std::cout << "**** ERROR: Different sizes." << std::endl;
			return false;
		}
		for (size_t i = 0; i < count; i++)
		{
			if (feedbacks[i] != r.feedbacks[i])
			{
				std::cout << "**** ERROR: Inconsistent [" << i << "]: "
					<< "Compare(" << secret << ", " << codewords[i] << ") = "
					<< feedbacks[i] << " v " << r.feedbacks[i] << std::endl;
				return false;
			}
		}
		return true;
	}

	friend std::ostream& operator << (std::ostream &os, const TestDriver &r)
	{
		FeedbackFrequencyTable freq;
		r.e.countFrequencies(r.feedbacks.begin(), r.feedbacks.end(), freq);
		return os << freq;
	}
};

// Color-mask scanning benchmark.
// Test: Scan 5040 codewords for 100,000 times.
//
// **** Old Results ****
// These results are for "short" codewords.
//
// ScanDigitMask_v1 (C):              5.35 s
// ScanDigitMask_v2 (16-bit ASM):     2.08 s
// ScanDigitMask_v3 (v2 improved):    1.43 s
// ScanDigitMask_v4 (v3 improved):    1.12 s
// ScanDigitMask_v5 (32-bit ASM):     2.09 s
// ScanDigitMask_v6 (v5 improved):    1.10 s
// ScanDigitMask_v7 (v6 generalized): 1.10 s
//
// Observations:
//   - ASM with parallel execution and loop unrolling performs the best.
//   - There is little performance difference between 16-bit ASM and 32-bit ASM.
//   - Loop unrolling has limited effect. Seems 1.10s is the lower bound
//     the current algorithm can improve to.
//
// **** New Results ****
// These results are for "long" codewords.
// ScanDigitMask_v1 (SSE2): 0.40 s
template <> class TestDriver<MaskRoutine>
{
	Engine &e;
	MaskRoutine f;
	CodewordList list;
	unsigned short mask;

public:
	TestDriver(Engine &env, MaskRoutine func)
		: e(env), f(func), list(e.generateCodewords()), mask(0) { }

	void operator()()
	{
		mask = list.empty()? 0 : f(&list.front(), &list.back() + 1);
	}

	bool operator == (const TestDriver &r) const
	{
		if (mask != r.mask)
		{
			std::cout << "**** Inconsistent color mask ****" << std::endl;
			return false;
		}
		return true;
	}

	friend std::ostream& operator << (std::ostream &os, const TestDriver &r)
	{
		os << "Present digits:";
		unsigned short t = r.mask;
		for (int i = 0; i < 16; i++)
		{
			if (t & 1)
				os << " " << i;
			t >>= 1;
		}
		return os << std::endl;
	}
};

#if 0
/// Compares frequency counting algorithms.
///
/// Test: Compute the frequency table of 5040 feedbacks.
/// Run the test for 500,000 times.
///
/// Results:
/// <pre>
/// count_freq_v1 (plain C):              5.55 s
/// count_freq_v2 (ASM, 2-parallel OOE):  3.85 s
/// count_freq_v3 (C, 4-parallel OOE):    5.51 s
/// count_freq_v4 (ASM, loop-unroll):     4.54 s
/// count_freq_v5 (ASM, 4-parallel OOE):  2.99 s
/// count_freq_v6 (improved ASM from v5): 2.88 s
/// </pre>
/// Conclusion:
/// Due to the memory-intensive nature of the algorithm, the performance
/// cannot be improved much. count_freq_v6() is the chosen implementation.
int TestFrequencyCounting(const Rules &rules, long times)
{
	CodewordList list = generateCodewords(rules);
	FeedbackList fblist = compare(rules, list[0], list.cbegin(), list.cend());
	int count = fblist.size();
	const unsigned char *fbl = (const unsigned char *)fblist.data();
	const unsigned char maxfb = 63;
	unsigned int freq[(int)maxfb+1];

	FREQUENCY_COUNTING_ROUTINE *func1 = CountFrequenciesImpl->GetRoutine("c");
	FREQUENCY_COUNTING_ROUTINE *func2 = CountFrequenciesImpl->GetRoutine("c_luf4");

	if (times == 0) {
		int total = 0;
		//count = 11;
		func2(fbl, count, freq, maxfb);
		for (int i = 0; i <= maxfb; i++) {
			if (freq[i] > 0)
			{
				std::cout << Feedback(i) << " = " << freq[i] << std::endl;
				total += freq[i];
			}
		}
		std::cout << "Expected count: " << count << std::endl;
		std::cout << "Actual total:   " << total << std::endl;
		system("PAUSE");
		return 0;
	}

	HRTimer timer;
	double t1, t2;
	t1 = t2 = 0;

	for (int pass = 0; pass < 10; pass++) {
		timer.start();
		for (int j = 0; j < times / 10; j++) {
			func1(fbl, count, freq, maxfb);
		}
		t1 += timer.stop();

		timer.start();
		for (int j = 0; j < times / 10; j++) {
			func2(fbl, count, freq, maxfb);
		}
		t2 += timer.stop();
	}

	printf("Algorithm 1: %6.3f\n", t1);
	printf("Algorithm 2: %6.3f\n", t2);
	printf("Improvement: %5.1f%%\n", (t1/t2-1)*100);

	system("PAUSE");
	return 0;
}
#endif

#if 0
static void simulate_guessing(
	Engine &e, Strategy* strats[], size_t n,
	EquivalenceFilter *filter,
	const CodeBreakerOptions &options)
{
	CodewordList all = e.generateCodewords();
	Rules rules = e.rules();

	std::cout
		<< "Game Settings" << std::endl
		<< "---------------" << std::endl
		<< "Number of pegs:      " << rules.pegs() << std::endl
		<< "Number of colors:    " << rules.colors() << std::endl
		<< "Color repeatable:    " << std::boolalpha << rules.repeatable() << std::endl
		<< "Number of codewords: " << rules.size() << std::endl;

	// Pick a secret "randomly".
	Codeword secret = all[all.size()/4*3];
	std::cout << std::endl;
	std::cout << "Secret: " << secret << std::endl;

	// Use an array to store the status of each codebreaker.
	std::vector<bool> finished(n);

	// Create an array of codebreakers and output strategy names.
	// BUG: when changing from CodeBreaker* to CodeBreaker, there is
	// a memory error. Find out why.
	std::vector<CodeBreaker*> breakers;
	std::cout << std::left << " # ";
	for (size_t i = 0; i < n; ++i)
	{
		std::string name = strats[i]->name();
		std::cout << std::setw(10) << name;
		breakers.push_back(new CodeBreaker(e,
			std::unique_ptr<Strategy>(strats[i]),
			std::unique_ptr<EquivalenceFilter>(filter->clone()),
			options));
	}
	std::cout << std::right << std::endl;

	// Output horizontal line.
	std::cout << "---";
	for (size_t i = 0; i < n; ++i)
	{
		std::cout << "----------";
	}
	std::cout << std::endl;

	// Step-by-step guessing.
	int step = 0;
	for (size_t finished_count = 0; finished_count < n; )
	{
		std::cout << std::setw(2) << (++step);
		std::cout.flush();

		// Test each code breaker in turn.
		for (size_t i = 0; i < n; ++i)
		{
			if (finished[i])
			{
				std::cout << "          ";
				continue;
			}

			CodeBreaker &breaker = *breakers[i];
			Codeword guess = breaker.MakeGuess();
			if (guess.empty())
			{
				std::cout << " FAIL";
				finished[i] = true;
				++finished_count;
			}
			else
			{
				Feedback feedback = e.compare(secret, guess);
				std::cout << " " << guess << ":" << feedback;
				std::cout.flush();
				if (feedback == Feedback::perfectValue(e.rules()))
				{
					finished[i] = true;
					++finished_count;
				}
				breaker.AddConstraint(guess, feedback);
			}
		}
		std::cout << std::endl;
	}
}
#endif

#if 1
static void test_strategy_tree(
	Engine &e,
	Strategy *strategies[],
	size_t n,
	EquivalenceFilter *filter,
	const CodeBreakerOptions &options)
	// const Codeword& first_guess)
{
	//CodewordList all = e.generateCodewords();
	Rules rules = e.rules();
	//Feedback target = Feedback::perfectValue(rules);
	util::hr_timer timer;

	std::cout
		<< "Game Settings" << std::endl
		<< "---------------" << std::endl
		<< "Number of pegs:      " << rules.pegs() << std::endl
		<< "Number of colors:    " << rules.colors() << std::endl
		<< "Color repeatable:    " << std::boolalpha << rules.repeatable() << std::endl
		<< "Number of codewords: " << rules.size() << std::endl;

	std::cout << std::endl
		<< "Options" << std::endl
		<< "---------" << std::endl
		<< "Optimize obvious guess: " << std::boolalpha
			<< options.optimize_obvious << std::endl
		<< "Guess possibility only: " << std::boolalpha
			<< options.possibility_only << std::endl;


	std::cout << std::endl << util::header;

	for (size_t i = 0; i < n; ++i)
	{
		Strategy *strat = strategies[i];

		// Build a strategy tree of this code breaker
		timer.start();
		EquivalenceFilter *copy = filter->clone();
		StrategyTree tree = BuildStrategyTree(e, strat, copy, options);
		delete copy;
		double t = timer.stop();

		StrategyTreeInfo info(strat->name(), tree, t);
		std::cout << info;
	}
}
#endif

static void display_canonical_guesses(
	Engine &e,
	const EquivalenceFilter *filter,
	int max_level,
	int level = 0)
{
	CodewordConstRange candidates = e.universe();
	CodewordList canonical = filter->get_canonical_guesses(candidates);

	// Display each canonical guess, and expand one more level is needed.
	if (level >= max_level)
	{
		std::cout << "[" << level << ":" << canonical.size() << "]";
#if 1
		if (canonical.size() > 20)
		{
			std::cout << " ... " << std::endl;
		}
		else
#endif
		{
			for (size_t i = 0; i < canonical.size(); ++i)
			{
				Codeword guess = canonical[i];
				std::cout << " " << guess;
			}
			std::cout << std::endl;
		}
	}
	else
	{
		for (size_t i = 0; i < canonical.size(); ++i)
		{
			Codeword guess = canonical[i];
			std::cout << "[" << level << ":" << i << "] " << guess << std::endl;

			EquivalenceFilter *child = filter->clone();
			child->add_constraint(guess, Feedback(), candidates);
			display_canonical_guesses(e, child, max_level, level+1);
			delete child;
			//std::cout << "[" << level << "] Total: " << canonical.size() << std::endl;
		}
	}
}

static void test_initial_guesses_in_equivalence_filter(
	Engine &e, const EquivalenceFilter *filter)
{
	display_canonical_guesses(e, filter, 2);
	delete filter;
}

static void display_partition_size(
	Engine &e, CodewordRange secrets, const Codeword &guess)
{
	FeedbackFrequencyTable freq = e.partition(secrets, guess);
	for (size_t i = 0; i < freq.size(); ++i)
	{
		if (i > 0)
			std::cout << ",";
		std::cout << freq[i];
	}
}

static void test_partition_size(Engine &e, const char *name)
{
	auto filter = std::unique_ptr<EquivalenceFilter>(
		RoutineRegistry<CreateEquivalenceFilterRoutine>::get(name)(e));

	// Get an initial canonical guess.
	CodewordList all = e.generateCodewords();
	CodewordList initials = filter->get_canonical_guesses(e.universe());
	Codeword g1 = initials[0];
	// std::cout << g1 << std::endl;

	// Partition the codeword set by g1.
	FeedbackFrequencyTable freq = e.partition(all, g1);
	int k = 0;
	for (size_t i = 0; i < freq.size(); ++i)
	{
		if (freq[i] > 0)
		{
			// Add constraint.
			auto child = std::unique_ptr<EquivalenceFilter>(filter->clone());
			CodewordRange part(all.begin() + k, all.begin() + k + freq[i]);
			child->add_constraint(g1, Feedback((unsigned char)i), part);
			CodewordList canonical = child->get_canonical_guesses(e.universe());
			std::cout << "Feedback: " << Feedback((unsigned char)i) << ", #remaining = "
				<< part.size() <<  ", #{g2} = " << canonical.size() << std::endl;

			if (1)
			{
				// Display how each guess partitions the remaining secrets.
				for (size_t j = 0; j < canonical.size() && j < 5; ++j)
				{
					std::cout << canonical[j] << ": ";
					display_partition_size(e, part, canonical[j]);
					std::cout << std::endl;
				}
			}
			k += freq[i];
		}
	}
}

static void test_serialization()
{
	Rules rules(4, 6, true);
	StrategyTree tree(rules);

	if (1)
	{
		std::cout << "File 1" << std::endl;
		std::ifstream fs("strats/p4c6r-koyama-1.txt");
		fs >> tree;
	}
	if (1)
	{
		std::cout << "File 2" << std::endl;
		std::ifstream fs("strats/p4c6r-koyama-2.txt");
		fs >> tree;
	}
	if (1)
	{
		std::cout << "File 3" << std::endl;
		std::ifstream fs("strats/p4c6r-knuth.txt");
		fs >> tree;
	}
	if (1)
	{
		std::cout << "File 4" << std::endl;
		std::ifstream fs("strats/p4c6r-neuwirth.txt");
		fs >> tree;
	}
	if (1)
	{
		std::cout << "File 5" << std::endl;
		std::ifstream fs("strats/p4c6r-irving.txt");
		fs >> tree;
	}
}

/// Runs regression and benchmark tests.
int test(const Rules &rules, bool /* verbose */)
{
#ifdef NDEBUG
#define LOOP_FLAG 1
#else
#define LOOP_FLAG 0
#endif

#if 0
	extern int interactive(const Rules &rules);
	return interactive(rules);
#endif

	// Set up the standard engine.
	Engine e(rules);

#if 0
	//compareRoutines<GenerationRoutine>(e, "generic", "generic", 100*LOOP_FLAG);
	//compareRoutines<ComparisonRoutine>(e, "generic", "norepeat", 100000*LOOP_FLAG);
	compareRoutines<MaskRoutine>(e, "generic", "unrolled", 100000*LOOP_FLAG);

	//testSumSquares(rules, "generic", "generic", 10000000*LOOP_FLAG);
	//return TestFrequencyCounting(rules, 250000*LOOP_FLAG);
	//return TestEquivalenceFilter(rules, 10000*LOOP_FLAG);
	system("PAUSE");
	return 0;
#endif

	// Choose an equivalence filter.
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

#if 0
	test_initial_guesses_in_equivalence_filter(e, filter.get());
	system("PAUSE");
	return 0;
#endif

#if 0
	test_partition_size(e, filter_name);
	system("PAUSE");
	return 0;
#endif

#if 0
	extern void test_optimal_strategy(Engine &);
	test_optimal_strategy(e);

	std::cout << std::endl;
	std::cout << "Call statistics for OptimalRecursion:" << std::endl;
	std::cout << util::call_counter::get("OptimalRecursion") << std::endl;

#if 0
	std::cout << "Call statistics for Comparison:" << std::endl;
	std::cout << util::call_counter::get("Comparison") << std::endl;
#endif

	pause_output();
	return 0;
#endif

#if 1
	test_serialization();
	pause_output();
	return 0;
#endif

	// @todo output strategy tree size info.
	CodeBreakerOptions options;
	options.optimize_obvious = true;
	options.possibility_only = false;

	//Codeword first_guess = Codeword::emptyValue();
	//Codeword first_guess = Codeword::Parse("0011", rules);

	using namespace Mastermind::Heuristics;
	Strategy* strats[] = {
		new SimpleStrategy(e),
		new HeuristicStrategy<MinimizeWorstCase<1>>(e),
#if 0
		new HeuristicStrategy<MinimizeAverage>(e),
		new HeuristicStrategy<MaximizeEntropy<false>>(e),
		new HeuristicStrategy<MaximizeEntropy<true>>(e),
		new HeuristicStrategy<MaximizePartitions>(e),
		new HeuristicStrategy<MinimizeLowerBound>(e, MinimizeLowerBound(e))
#endif
		//new OptimalCodeBreaker(rules),
	};
	int nstrat = sizeof(strats)/sizeof(strats[0]);

	// Note: Only one of the following can be invoked because
	// a code breaker makes a unique pointer of the strategy.
	//simulate_guessing(e, strats, nstrat, filter.get(), options);
	test_strategy_tree(e, strats, nstrat, filter.get(), options);

#if 0
	std::cout << "Call statistics for ConstraintEquivalence:" << std::endl;
	std::cout << util::call_counter::get("ConstraintEquivalence") << std::endl;

	std::cout << "Call statistics for ConstraintEquivalencePermutation:" << std::endl;
	std::cout << util::call_counter::get("ConstraintEquivalencePermutation") << std::endl;

	std::cout << "Call statistics for ConstraintEquivalenceCrossout:" << std::endl;
	std::cout << util::call_counter::get("ConstraintEquivalenceCrossout") << std::endl;
#endif

#if 0
	// Display some statistics.
	std::cout << "Call statistics for Comparison:" << std::endl;
	std::cout << util::call_counter::get("Comparison") << std::endl;
#endif

#if 0
	if (0) {
		printf("\nRun again:\n");
		CountFrequenciesImpl->SelectRoutine("c_p8_il_os");
		TestGuessingByTree(rules, breakers, sizeof(breakers)/sizeof(breakers[0]), first_guess);
		printf("\n");
	}

	void PrintFrequencyStatistics();
	//PrintFrequencyStatistics();

	void PrintCompareStatistics();
	//PrintCompareStatistics();

	void PrintMakeGuessStatistics();
	//PrintMakeGuessStatistics();

	void OCB_PrintStatistics();
	OCB_PrintStatistics();

#endif

	pause_output();
	return 0;
}

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
#endif