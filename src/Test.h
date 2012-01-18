/// \file Test.h
/// Declaration of test routines.

#pragma once

#ifndef NTEST

#include "Codeword.hpp"
#include "CodewordList.hpp"
#include "CodewordRules.hpp"

using namespace Mastermind;

int TestCompare(const CodewordRules &rules, const char *routine1, const char *routine2, long times);
int TestSumOfSquares(const CodewordRules &rules, const char *routine1, const char *routine2, long times);

int TestEnumeration(const CodewordRules &rules, long times);
int TestEnumerationDirect(long times);
int TestEquivalenceFilter(const CodewordRules &rules, long times);
int TestScan(const CodewordRules &rules, long times);
int TestFrequencyCounting(const CodewordRules &rules, long times);
int TestNewCompare(const CodewordRules &rules, long times);
int TestNewScan(const CodewordRules &rules, long times);

#endif
