#ifndef MASTERMIND_EQUIVALENCE_HPP
#define MASTERMIND_EQUIVALENCE_HPP

// This file defines an interface for equivalence filters.

#include <memory>
#include "Engine.hpp"
#include "util/aligned_allocator.hpp"

namespace Mastermind {

/// Represents an equivalence filter which filters canonical guesses
/// from a set of candidate codewords.
class EquivalenceFilter
{
public:

	virtual std::unique_ptr<EquivalenceFilter> clone() const = 0;

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates
		) const = 0;

	virtual void add_constraint(
		const Codeword &guess,
		Feedback response, 
		CodewordConstRange remaining
		) = 0;
};

/// Represents an incremental constraint equivalence filter.
class ConstraintEquivalenceFilter : public EquivalenceFilter
{
	Engine &e;
	Rules rules;
	std::vector<
		CodewordPermutation,
		util::aligned_allocator<CodewordPermutation,16>
	> pp;

public:
	ConstraintEquivalenceFilter(Engine &engine);

	virtual std::unique_ptr<EquivalenceFilter> clone() const
	{
		return std::unique_ptr<EquivalenceFilter>(
			new ConstraintEquivalenceFilter(*this));
	}

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates) const;

	virtual void add_constraint(
		const Codeword &guess,
		Feedback response, 
		CodewordConstRange remaining
		);
};

} // namespace Mastermind

#endif // MASTERMIND_EQUIVALENCE_HPP
