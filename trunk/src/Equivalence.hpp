#ifndef MASTERMIND_EQUIVALENCE_HPP
#define MASTERMIND_EQUIVALENCE_HPP

// This file defines an interface for equivalence filters.

#include "Engine.hpp"

namespace Mastermind {

/// Defines the interface for an equivalence filter which filters
/// canonical guesses from a set of candidate codewords.
class EquivalenceFilter
{
public:

	virtual EquivalenceFilter* clone() const = 0;

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates
		) const = 0;

	virtual void add_constraint(
		const Codeword &guess,
		Feedback response, 
		CodewordConstRange remaining
		) = 0;
};

/// Typedef of pointer to function that creates an equivalence filter.
/// @ingroup equiv
typedef EquivalenceFilter* (*CreateEquivalenceFilterRoutine)(Engine &e);

} // namespace Mastermind

#endif // MASTERMIND_EQUIVALENCE_HPP
