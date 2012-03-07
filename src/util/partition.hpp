#ifndef UTILITIES_PARTITION_H
#define UTILITIES_PARTITION_H

#include <cassert>
#include "range.hpp"
#include "frequency_table.hpp"

namespace util {

template <class Iter, int Capacity>
class partition_cells
{
	size_t _size;
	Iter _begin[Capacity+1];

public:

	// Creates an empty partition, which contains no cells.
	partition_cells() : _size(0) { }

	// Creates a unit partition, which contains only one cell.
	partition_cells(range<Iter> cell) : _size(1) 
	{
		_begin[0] = cell.begin();
		_begin[1] = cell.end();
	}

	// Creates a partition from a frequency table and its corresponding range.
	template <class TKey, class TVal>
	partition_cells(range<Iter> all, const frequency_table<TKey,TVal,Capacity> &freq)
		: _size(freq.size())
	{
		_begin[0] = all.begin();
		for (size_t j = 0; j < freq.size(); ++j)
		{
			_begin[j+1] = _begin[j] + (size_t)freq[j];
		}
		assert(_begin[_size] == all.end());
	}

	// Returns the number of cells in this partition.
	size_t size() const { return _size; }

	// Returns the given cell.
	range<Iter> operator [] (size_t i) const
	{
		assert(i >= 0 && i < _size);
		return range<Iter>(_begin[i], _begin[i+1]);
	}
};

} // namespace util

#endif // UTILITIES_PARTITION_H
