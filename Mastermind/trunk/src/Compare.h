/// \file Compare.h
/// Declaration of codeword comparison routines.

#pragma once

#include "MMConfig.h"


typedef void (*COMPARISON_ROUTINE)(
	const codeword_t& secret, 
	const codeword_t guesses[], 
	unsigned int count, 
	unsigned char results[]);

extern COMPARISON_ROUTINE Compare_Impl;

typedef struct ComparisonRoutineEntry 
{
	const char *name;
	const char *description;
	COMPARISON_ROUTINE routine;
} ComparisonRoutineEntry;

extern ComparisonRoutineEntry Compare_Impls[];

void Compare_SelectImpl(const char *name);


/// Compares an array of guesses to the secret, allowing digit repetition
/// in the codewords.
void Compare_Rep(
	/// [in] The secret codeword
	const codeword_t& secret, 
	/// [in] An array of guesses
	const codeword_t guesses[], 
	/// [in] Number of guesses in the array
	unsigned int count, 
	/// [out] An array to store feedbacks
	unsigned char results[]);

/// Compares an array of guesses to the secret, assuming no 
/// digit repetition in the codewords.
/// This routine is almost twice as fast as the generic routine
/// <code>Compare_Rep()</code>, but gives undefined results if 
/// repetition actually exists in the codewords.
void Compare_NoRep(
	/// [in] The secret codeword
	const codeword_t& secret, 
	/// [in] An array of guesses
	const codeword_t guesses[], 
	/// [in] Number of guesses in the array
	unsigned int count, 
	/// [out] An array to store feedbacks
	unsigned char results[]);



