/// \file Test.cpp
/// Routines for testing the algorithms.

#include <iostream>
#include <stdio.h>
#include <malloc.h>
#include <iomanip>

#include "HRTimer.h"

#include "Codeword.hpp"
#include "Feedback.hpp"
#include "Algorithm.hpp"
#include "CodeBreaker.h"
#include "SimpleCodeBreaker.hpp"
#include "HeuristicCodeBreaker.hpp"
#include "Engine.hpp"

using namespace Mastermind;
using namespace Utilities;

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
	HRTimer timer;
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
int TestEquivalenceFilter(const CodewordRules &rules, long times)
{
	CodewordList list = generateCodewords(rules);
	int total = (int)list.size();

	unsigned char eqclass[16] = {
		//0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15 }; // all diff
		//0,3,2,5, 4,7,6,9, 8,1,10,11, 12,13,14,15 }; // 1,3,5,7,9 same
		//1,2,3,4, 5,6,7,8, 9,0,10,11, 12,13,14,15 }; // all same
		0,3,2,5, 4,1,6,7, 8,9,10,11, 12,13,14,15 }; // 1,3,5 same

	//__m128i *output = (__m128i*)_aligned_malloc(16*total, 16);
	util::aligned_allocator<__m128i,16> alloc;
	__m128i* output = alloc.allocate(total);

	int count;

	if (times == 0) {
		//count = FilterByEquivalenceClass_norep_v3(
		count = FilterByEquivalenceClass_rep_v1(
			(codeword_t*)list.data(), total, eqclass, (codeword_t *)output);
		if (1) {
			for (int i = 0; i < count; i++)
			{
				std::cout << Codeword(output[i]) << " ";
			}
		}
		alloc.deallocate(output,total);
		//_aligned_free(output);
		printf("\nCount: %d\n", count);
		system("PAUSE");
		return 0;
	}

	HRTimer timer;
	double t1, t2;

	t1 = t2 = 0;
	for (int pass = 0; pass < 10; pass++) {
		timer.start();
		for (int k = 0; k < times / 10; k++) {
//			count = FilterByEquivalenceClass_norep_v1(
//				(__m128i*)list.GetData(), list.GetCount(), eqclass, output);
			count = FilterByEquivalenceClass_norep_v2(
				(codeword_t*)list.data(), list.size(), eqclass, (codeword_t *)output);
		}
		t1 += timer.stop();

		timer.start();
		for (int k = 0; k < times / 10; k++) {
			count = FilterByEquivalenceClass_norep_v3(
				(codeword_t*)list.data(), list.size(), eqclass, (codeword_t *)output);
		}
		t2 += timer.stop();
	}
	printf("Equivalence 1: %6.3f\n", t1);
	printf("Equivalence 2: %6.3f\n", t2);
	printf("Count: %d\n", count);

	alloc.deallocate(output,total);
	//_aligned_free(output);
	system("PAUSE");
	return 0;
}
#endif

#if 0
// Compare FilterByEquivalence Algorithms
int TestEquivalenceFilter(CodewordRules rules, long times)
{
	CodewordList list = CodewordList::Enumerate(rules);

	unsigned char eqclass[16] = {
		//0,1,2,3, 4,5,6,7, 8,9,10,11, 12,13,14,15 }; // all diff
		//0,3,2,5, 4,7,6,1, 8,9,10,11, 12,13,14,15 }; // 1,3,5,7 same
		1,2,3,4, 5,6,7,8, 9,0,10,11, 12,13,14,15 }; // all same
	unsigned short output[5040];
	int count;

	if (times == 0) {
		count = FilterByEquivalenceClass_norep_16_v1(
			list.GetData(), list.GetCount(), eqclass, output);
		if (1) {
			for (int i = 0; i < count; i++) {
				printf("%04x ", output[i]);
			}
		}
		printf("\nCount: %d\n", count);
		system("PAUSE");
		return 0;
	}

	HRTimer timer;
	double t1, t2;

	t1 = t2 = 0;
	for (int pass = 0; pass < 10; pass++) {
		timer.Start();
		for (int k = 0; k < times / 10; k++) {
			count = FilterByEquivalenceClass_norep_16_v1(
				list.GetData(), list.GetCount(), eqclass, output);
		}
		t1 += timer.Stop();

		timer.Start();
		for (int k = 0; k < times / 10; k++) {
			count = FilterByEquivalenceClass_norep_16_v2(
				list.GetData(), list.GetCount(), eqclass, output);
		}
		t2 += timer.Stop();
	}
	printf("Equivalence 1: %6.3f\n", t1);
	printf("Equivalence 2: %6.3f\n", t2);
	printf("Count: %d\n", count);

	system("PAUSE");
	return 0;
}
#endif

#if 0
#ifndef NTEST
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
int TestFrequencyCounting(const CodewordRules &rules, long times)
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
#endif

static bool testSumSquares(
	const Engine &e,
	const char *routine1, 
	const char *routine2, 
	long times)
{
	CodewordList list = e.generateCodewords();
	FeedbackList fbl = e.compare(list[0], list.begin(), list.end());
	FeedbackFrequencyTable freq;
	e.countFrequencies(fbl.begin(), fbl.end(), freq);
	unsigned char maxfb = Feedback::maxValue(e.rules()); // fbl.GetMaxFeedbackValue();
	size_t count = maxfb + 1;

	SumSquaresRoutine func1 = RoutineRegistry<SumSquaresRoutine>::get(routine1);
	SumSquaresRoutine func2 = RoutineRegistry<SumSquaresRoutine>::get(routine2);

	// Verify results.
	unsigned int ss1 = func1(freq.data(), freq.data() + count);
	unsigned int ss2 = func2(freq.data(), freq.data() + count);
	if (ss1 != ss2) 
	{
		std::cout << "**** ERROR: Result mismatch: " << ss1 
			<< " v " << ss2 << std::endl;
		return false;
	}

	// Print result if in debug mode
	if (times == 0) 
	{
		std::cout << "SS1 = " << ss1 << std::endl;
		std::cout << "SS2 = " << ss2 << std::endl;
		return true;
	}

	HRTimer timer;
	double t1 = 0, t2 = 0;

	for (int pass = 0; pass < 10; pass++) 
	{
		timer.start();
		for (int j = 0; j < times / 10; j++) 
		{
			ss1 = func1(freq.data(), freq.data() + count);
		}
		t1 += timer.stop();

		timer.start();
		for (int j = 0; j < times / 10; j++) 
		{
			ss2 = func2(freq.data(), freq.data() + count);
		}
		t2 += timer.stop();
	}

	printf("Algorithm 1: %6.3f\n", t1);
	printf("Algorithm 2: %6.3f\n", t2);
	// printf("Speed Ratio: %5.2fX\n", t1/t2);

	// system("PAUSE");
	return 0;
}

static void TestGuessingByTree(
	const Engine &e,
	CodeBreaker *breakers[], 
	int nb,
	const Codeword& first_guess)
{
	CodewordList all = e.generateCodewords();
	CodewordRules rules = e.rules();
	Feedback target = Feedback::perfectValue(rules);
	Utilities::HRTimer timer;

	std::cout 
		<< "Game Settings" << std::endl
		<< "---------------" << std::endl
		<< "  Number of pegs:      " << rules.pegs() << std::endl
		<< "  Number of colors:    " << rules.colors() << std::endl
		<< "  Color repeatable:    " << rules.repeatable() << std::endl
		<< "  Number of codewords: " << all.size() << std::endl;

	//printf("\n");
	//printf("Algorithm Descriptions\n");
	//printf("------------------------\n");
	//for (int i = 0; i < nb; i++) {
	//	printf("  A%d: %s\n", (i + 1), breakers[i]->GetDescription().c_str());
	//}

	std::cout << std::endl
		<< "Frequency Table" << std::endl
		<< "-----------------" << std::endl
		<< "Strategy: Total   Avg    1    2    3    4    5    6    7    8    9   >9   Time" << std::endl;

	for (int ib = 0; ib < nb; ib++) {
		CodeBreaker *breaker = breakers[ib];

		// Build a strategy tree of this code breaker
		timer.start();
		StrategyTree *tree = breaker->BuildStrategyTree(first_guess);
		double t = timer.stop();

		// Count the steps used to get the answers
		const int max_rounds = 10;
		int freq[max_rounds+1];
		int sum_rounds = tree->GetDepthInfo(freq, max_rounds);
		int count = all.size();

//			if (i*100/count > pct) {
//				pct = i*100/count;
//				printf("\r  A%d: running... %2d%%", ib + 1, pct);
//				fflush(stdout);
//			}

		// Display statistics
		std::cout << "\r" << std::setw(8) << breaker->name() << ":"
			<< std::setw(6) << sum_rounds << " "
			<< std::setw(5) << std::setprecision(3) 
			<< (double)sum_rounds / count;

		for (int i = 1; i <= max_rounds; i++) {
			if (freq[i] > 0) 
				std::cout << std::setw(4) << freq[i] << ' ';
			else
				std::cout << "   - ";
		}
		std::cout << std::setw(6) << std::setprecision(1) << t << std::endl;

		// delete tree;
		// TODO: garbage collection!
	}
}

/// Runs regression and benchmark tests.
int test(const CodewordRules &rules)
{
#ifdef NDEBUG
#define LOOP_FLAG 1
#else
#define LOOP_FLAG 0
#endif

	// Set up the standard engine.
	Engine e(rules);

#if 1
	//compareRoutines<GenerationRoutine>(e, "generic", "generic", 100*LOOP_FLAG);
	//compareRoutines<ComparisonRoutine>(e, "generic", "norepeat", 100000*LOOP_FLAG);
	compareRoutines<MaskRoutine>(e, "generic", "unrolled", 100000*LOOP_FLAG);

	//testSumSquares(rules, "generic", "generic", 10000000*LOOP_FLAG);

	system("PAUSE");
	return 0;
#endif

	//return TestFrequencyCounting(rules, 250000*LOOP_FLAG);
	//return TestEquivalenceFilter(rules, 10000*LOOP_FLAG);
	//return BuildLookupTableForLongComparison();
	//return TestEnumerationDirect(200000*1);

	Codeword first_guess = Codeword::emptyValue();
	//Codeword first_guess = Codeword::Parse("0011", rules);
	bool posonly = false; // only guess from remaining possibilities
	CodeBreaker* breakers[] = {
		new SimpleCodeBreaker(e),
#if 0
		new HeuristicCodeBreaker<Heuristics::MinimizeWorstCase>(e, posonly),
		new HeuristicCodeBreaker<Heuristics::MinimizeAverage>(e, posonly),
		new HeuristicCodeBreaker<Heuristics::MaximizeEntropy>(e, posonly),
		new HeuristicCodeBreaker<Heuristics::MaximizePartitions>(e, posonly),
#endif
		//new HeuristicCodeBreaker<Heuristics::MinimizeSteps>(rules, posonly),
		//new OptimalCodeBreaker(rules),
	};

	// CountFrequenciesImpl->SelectRoutine("c");
	TestGuessingByTree(rules, breakers, sizeof(breakers)/sizeof(breakers[0]), first_guess);
	printf("\n");

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

	system("PAUSE");
	return 0;
}
