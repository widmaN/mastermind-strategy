#ifndef MASTERMIND_EQUIVALENCE_HPP
#define MASTERMIND_EQUIVALENCE_HPP

// This file defines an interface for equivalence filters.

#include <memory>
#include "Engine.hpp"

namespace Mastermind {

/// Defines the interface for an equivalence filter which filters
/// canonical guesses from a set of candidate codewords.
class EquivalenceFilter
{
public:

	virtual ~EquivalenceFilter() { }

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

/// Represents a composite equivalence filter which chains two 
/// individual equivalence filters.
class CompositeEquivalenceFilter : public EquivalenceFilter
{
	std::unique_ptr<EquivalenceFilter> _filter1, _filter2;

public:

	CompositeEquivalenceFilter(
		const EquivalenceFilter *filter1, 
		const EquivalenceFilter *filter2)
		: _filter1(filter1->clone()), _filter2(filter2->clone())
	{
	}

	CompositeEquivalenceFilter(Engine &e, const char *name1, const char *name2)
		: _filter1(RoutineRegistry<CreateEquivalenceFilterRoutine>::get(name1)(e)),
		 _filter2(RoutineRegistry<CreateEquivalenceFilterRoutine>::get(name2)(e))
	{
	}


	virtual EquivalenceFilter* clone() const 
	{
		return new CompositeEquivalenceFilter(_filter1.get(), _filter2.get());
	}

	virtual CodewordList get_canonical_guesses(
		CodewordConstRange candidates
		) const 
	{
		CodewordList temp = _filter1->get_canonical_guesses(candidates);
		return _filter2->get_canonical_guesses(temp);
	}

	virtual void add_constraint(
		const Codeword & guess,
		Feedback response, 
		CodewordConstRange remaining)
	{
		_filter1->add_constraint(guess, response, remaining);
		_filter2->add_constraint(guess, response, remaining);
	}
};

} // namespace Mastermind

#endif // MASTERMIND_EQUIVALENCE_HPP
