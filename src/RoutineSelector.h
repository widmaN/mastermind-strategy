#ifndef ROUTINESELECTOR_H
#define ROUTINESELECTOR_H

#include <assert.h>
#include <string.h>

namespace Utilities 
{
	/// Routine selector that allows the user to pick an active implementation 
	/// from a list of candidate implementations.
	template <class ROUTINE_SIGNATURE>
	class RoutineSelector
	{
	public:
		/// Information about a candidate routine.
		struct RoutineEntry 
		{
			/// The name of the routine
			const char *name;
			/// A description of the routine
			const char *description;
			/// Function pointer to the routine
			ROUTINE_SIGNATURE *routine;
		};

	protected:
		/// An array of candidate routines. The array must terminate with
		/// NULL.
		RoutineEntry *m_entries;

	public:
		RoutineSelector(RoutineEntry* entries, const char *default_name);

		void SelectRoutine(const char *name);

		/// Finds a routine of the given name and returns a function pointer
		/// to it.
		/// @param[in]	name	Name of the routine
		/// @returns	A function pointer to the routine if found, or 
		/// <code>NULL</code> if not found.
		ROUTINE_SIGNATURE* GetRoutine(const char *name);

	public:
		/// Function pointer to the currently selected routine.
		ROUTINE_SIGNATURE *Run;
	};

	template <class ROUTINE_SIGNATURE>
	RoutineSelector<ROUTINE_SIGNATURE>::RoutineSelector(RoutineEntry* entries, const char *default_name)
	{
		this->m_entries = entries;
		SelectRoutine(default_name);
	}

	template <class ROUTINE_SIGNATURE>
	ROUTINE_SIGNATURE* RoutineSelector<ROUTINE_SIGNATURE>::GetRoutine(const char *name)
	{
		const RoutineEntry *entry = m_entries;
		for (; entry->name != NULL; entry++) {
			if (strcmp(entry->name, name) == 0) {
				return entry->routine;
			}
		}
		return NULL;
	}


	template <class ROUTINE_SIGNATURE>
	void RoutineSelector<ROUTINE_SIGNATURE>::SelectRoutine(const char *name)
	{
		ROUTINE_SIGNATURE *routine = GetRoutine(name);
		assert(routine != NULL);
		this->Run = routine;
	}
}

#endif
