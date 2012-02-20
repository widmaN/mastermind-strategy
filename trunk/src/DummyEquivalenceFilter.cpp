#include "Equivalence.hpp"

namespace Mastermind {

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

static EquivalenceFilter* CreateDummyEquivalenceFilter(Engine&)
{
	return new DummyEquivalenceFilter();
}

REGISTER_ROUTINE(CreateEquivalenceFilterRoutine,
				 "Dummy",
				 CreateDummyEquivalenceFilter)

} // namespace Mastermind
