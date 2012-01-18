#pragma once

#include <stdlib.h>
// #include <memory.h>

namespace Utilities
{
	template <class element_type, int bits_per_page>
	class FifoMemoryManager
	{
	public:
		typedef element_type elem_t;

	private:
		struct page_t {
			elem_t data[1 << bits_per_page];
			unsigned int used;
			page_t *nextpage;
		};
		page_t *m_firstpage;

	public:

		/// Creates a first-in-first-out memory manager.
		FifoMemoryManager()
		{
			m_firstpage = (page_t*)malloc(sizeof(page_t));
			m_firstpage->nextpage = NULL;
			m_firstpage->used = 0;
		}

		/// Destructor.
		~FifoMemoryManager()
		{
			page_t *page;
			while ((page = m_firstpage) != NULL) {
				m_firstpage = page->nextpage;
				free(page);
			}
		}

	public:
		
		/// Allocates one element.
		elem_t* Alloc()
		{
			if (m_firstpage->used == (1 << bits_per_page)) {
				page_t *page = (page_t*)malloc(sizeof(page_t));
				page->used = 0;
				page->nextpage = m_firstpage;
				m_firstpage = page;
			}
			return &m_firstpage->data[m_firstpage->used++];
		}

		/// Frees the space up-till and including the given element.
		void FreeUntil(elem_t *elem)
		{
			// Locate the page where this element reside
			page_t *page;
			for (page = m_firstpage; page != NULL; page = page->nextpage) {
				size_t offset = elem - &page->data[0]; // unsigned!!
				if (offset >= 0 && offset < (1 << bits_per_page)) {
					break;
				}
			}
			assert(page != NULL);

			// Remove all pages before that page
			while (m_firstpage != page) {
				page_t *p = m_firstpage->nextpage;
				free(m_firstpage);
				m_firstpage = p;
			}

			// Remove all elements beyond and including the given element 
			// in that page
			page->used = (elem - &page->data[0]);
		}
	};
}