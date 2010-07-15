/// \file Compare.h
/// Declaration of codeword comparison routines.

#pragma once

#include "MMConfig.h"

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
