#ifndef MASTERMIND_ALGORITHM_H
#define MASTERMIND_ALGORITHM_H

#include "Codeword.hpp"
#include "CodewordRules.hpp"
#include "CodewordList.hpp"
#include "Feedback.h"

namespace Mastermind {

	
// Returns a bit-mask of the colors that are present in the codeword.
unsigned short getDigitMask(const Codeword &c);

/// Returns a bit-mask of all the colors that appeared in the list.
unsigned short getDigitMask(const CodewordList &list);

unsigned short getDigitMask(
	CodewordList::const_iterator first, 
	CodewordList::const_iterator last);

/// Compares two codewords.
/// @returns The feedback.
Feedback compare(const Codeword& secret, const Codeword& guess);

/// Compares a codeword to a list of codewords.
FeedbackList compare(
	const CodewordRules &rules, 
	const Codeword& guess,
	CodewordIterator first,
	CodewordIterator last);

CodewordList filterByFeedback(
	const CodewordList &list,
	const CodewordRules &rules, 
	const Codeword &guess, 
	Feedback feedback);

/// Partitions a codeword list by a supplied _guess_.
/// The elements in the codeword list are re-ordered in-memory, 
/// such that codewords with the same feedback when compared to
/// the guess are stored consecutively in memory.
/// The feedback frequency table is also filled as a by-product.
void partition(
	CodewordIterator first, 
	CodewordIterator last,
	const CodewordRules &rules,
	const Codeword &guess, 
	FeedbackFrequencyTable &freq);



} // namespace

#endif // MASTERMIND_ALGORITHM_H
