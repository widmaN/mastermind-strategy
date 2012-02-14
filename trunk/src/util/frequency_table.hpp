#ifndef UTILITIES_FREQUENCY_TABLE_HPP
#define UTILITIES_FREQUENCY_TABLE_HPP

#include <cassert>
#include <iostream>
#include <algorithm>
#include <numeric>

namespace util {


template <class TKey, class TVal, size_t Capacity>
class frequency_table
{
	TVal _freq[Capacity];
	size_t _count;

public:
	frequency_table() : _count(0) { }

	size_t size() const { return _count; }

	void resize(size_t n)
	{
		assert(n <= Capacity);
		_count = n;
	}

	TVal* data() { return _freq; }
	const TVal* data() const { return _freq; }

	const TVal* begin() const { return _freq + 0; }
	const TVal* end() const { return _freq + _count; }

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

#if 0
	/// Computes the entropy of the frequencies.
	double entropy() const
	{
		double s = 0.0;
		for (size_t i = 0; i < _count; ++i)
		{
			TVal f = _freq[i];
			if (f > 0)
				s += std::log((double)f) * (double)f;
		}
		return s;
	}
#endif

};

/**
 * Outputs a frequency table to a stream.
 * @timecomplexity <code>O(N)</code> where @c N is the size of the table.
 * @spacecomplexity Constant.
 */
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
