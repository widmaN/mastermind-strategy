#ifndef ROUTINESELECTOR_H
#define ROUTINESELECTOR_H

#include <assert.h>
#include <string.h>

namespace Utilities 
{
	template <class ROUTINE_SIGNATURE>
	class RoutineSelector
	{
	public:
		typedef struct RoutineEntry 
		{
			const char *name;
			const char *description;
			ROUTINE_SIGNATURE *routine;
		} RoutineEntry;

	protected:
		RoutineEntry *m_entries;

	public:
		RoutineSelector(RoutineEntry* entries, const char *default_name);
		void SelectRoutine(const char *name);
		ROUTINE_SIGNATURE* GetRoutine(const char *name);

	public:
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
