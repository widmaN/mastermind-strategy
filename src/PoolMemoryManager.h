#pragma once

#include <stdlib.h>
// #include <memory.h>

namespace Utilities
{
	template <class element_type, int bits_per_page>
	class PoolMemoryManager
	{
	public:
		typedef element_type elem_t;

	private:
		elem_t** m_pages;
		int* m_freelist;
		int m_npages;

	public:

		/// Creates a first-in-first-out memory manager.
		PoolMemoryManager()
		{
			m_npages = 1;
			m_pages = (elem_t**)malloc(sizeof(elem_t*));
			m_pages[0] = (elem_t*)malloc(sizeof(elem_t)*(1<<bits_per_page));
			m_freelist = (int*)malloc(sizeof(int)*(1<<bits_per_page));
			for (int i = 0; i < (1<<bits_per_page)-1; i++) {
				m_freelist[i] = i+1;
			}
			m_freelist[(1<<bits_per_page)-1] = 0;
		}

		/// Destructor.
		~PoolMemoryManager()
		{
			for (int i = 0; i < m_npages; i++) {
				free(m_pages[i]);
			}
			free(m_pages);
			free(m_freelist);
			m_npages = 0;
		}

	public:

		/// Allocates one element.
		elem_t* Alloc()
		{
			if (m_freelist[0] == 0) { // no more space
				m_npages++;
				m_pages = (elem_t**)realloc(m_pages, sizeof(elem_t*)*m_npages);
				m_pages[m_npages-1] = (elem_t*)malloc(sizeof(elem_t)*(1<<bits_per_page));
				m_freelist = (int*)realloc(m_freelist, sizeof(int)*(1<<bits_per_page)*m_npages);
				int i;
				for (i = (m_npages-1)<<bits_per_page; i < m_npages<<bits_per_page; i++) {
					m_freelist[i] = i+1;
				}
				m_freelist[i] = 0;
				m_freelist[0] = (m_npages-1) << bits_per_page;
			}

			int j = m_freelist[0];
			m_freelist[0] = m_freelist[j];
			return &m_pages[j>>bits_per_page][j&((1<<bits_per_page)-1)];
		}

		/// Frees the given element.
		void Free(elem_t *elem)
		{
			// Locate the page where this element reside
			int i;
			for (i = m_npages - 1; i >= 0; i--) {
				size_t offset = elem - m_pages[i]; // unsigned!!
				if (offset >= 0 && offset < (1 << bits_per_page)) {
					// Adds this element to the free list
					int j = (i << bits_per_page) + (int)offset;
					m_freelist[j] = m_freelist[0];
					m_freelist[0] = j;
					return;
				}
			}
			assert(0);
		}
	};
}
