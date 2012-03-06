/// @defgroup FreqTable Frequency Table
/// @ingroup util

#ifndef UTILITIES_FREQUENCY_TABLE_HPP
#define UTILITIES_FREQUENCY_TABLE_HPP

#include <cassert>
#include <iostream>
#include <algorithm>
#include <numeric>

namespace util {

/// Represents a frequency table of (value, frequency) pairs where
/// the values are mapped to and from zero-based indices.
/// @ingroup FreqTable
template <class TKey, class TVal, size_t Capacity>
class frequency_table
{
	TVal _freq[Capacity];
	size_t _count;

public:

	/// Constructs an empty frequency table.
	frequency_table() : _count(0) { }

	/// Constructs a frequency table of the given size.
	frequency_table(size_t n) : _count(n) 
	{
		assert(n <= Capacity);
		std::fill(_freq + 0, _freq + n, TVal(0));
	}

	/// Returns the size of the frequency table.
	size_t size() const { return _count; }

#if 0
	/// Sets the size of the frequency table.
	void resize(size_t n)
	{
		assert(n <= Capacity);
		_count = n;
	}
#endif

	/// Returns a pointer to the first frequency value.
	TVal* data() { return _freq; }

	/// Returns a const pointer to the first frequency value.
	const TVal* data() const { return _freq; }

	/// Returns a begin iterator of the frequency table.
	const TVal* begin() const { return _freq + 0; }

	/// Returns an end iterator of the frequency table.
	const TVal* end() const { return _freq + _count; }

	/// Returns the frequency of the <code>k</code>-th element.
	TVal operator [] (size_t k) const
	{
		assert(k < _count);
		return _freq[k];
	}

	/// Returns the maximum frequency value.
	TVal max() const
	{
		return _count == 0? 0 : *std::max_element(begin(), end());
	}

	/// Returns the number of non-zero categories.
	size_t nonzero_count() const
	{
		return std::count_if(begin(), end(), [](TVal f) -> bool {
			return f > 0;
		});
	}

};

/// Outputs a frequency table to a stream.
/// @ingroup FreqTable
template <class TKey, class TVal, size_t Capacity>
std::ostream&
operator << (std::ostream& os, const frequency_table<TKey,TVal,Capacity> &f)
{
	size_t total = 0;
	for (size_t i = 0; i < f.size(); i++)
	{
		if (f[i] != 0)
		{
			os << TKey(i) << " => " << f[i] << std::endl;
			total += f[i];
		}
	}
	os << "Total: " << total << std::endl;
	return os;
}

} // namespace util

#endif // UTILITIES_FREQUENCY_TABLE_HPP
