#ifndef MASTERMIND_EQUIVALENCE_HPP
#define MASTERMIND_EQUIVALENCE_HPP

// This file defines an interface for equivalence filters.

#include "Engine.hpp"
#include "Permutation.hpp"

namespace Mastermind {

/// Represents an equivalence filter which filters canonical guesses
/// from a set of candidate codewords.
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

#if 1
/// Represents a dummy equivalence filter that doesn't filter anything.
class DummyEquivalenceFilter : public EquivalenceFilter
{
public:

	virtual EquivalenceFilter* clone() const 
	{
		return new DummyEquivalenceFilter();
	}

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates
		) const 
	{
		return CodewordList(candidates.begin(), candidates.end());
	}

	virtual void add_constraint(
		const Codeword & /* guess */,
		Feedback /* response */, 
		CodewordConstRange /* remaining */)
	{
	}
};
#endif

EquivalenceFilter* CreateConstraintEquivalenceFilter(Engine &e);

} // namespace Mastermind

#endif // MASTERMIND_EQUIVALENCE_HPP
