/// \file Test.h
/// Declaration of test routines.

#pragma once

#ifndef NTEST

#include "Codeword.h"

using namespace Mastermind;

int TestCompare(CodewordRules rules, const char *routine1, const char *routine2, long times);

int TestEnumeration(CodewordRules rules, long times);
int TestEnumerationDirect(long times);
int TestEquivalenceFilter(CodewordRules rules, long times);
int TestScan(CodewordRules rules, long times);
int TestFrequencyCounting(CodewordRules rules, long times);
int TestNewCompare(CodewordRules rules, long times);
int TestNewScan(CodewordRules rules, long times);
int TestSumOfSquares(CodewordRules rules, long times);

#endif
