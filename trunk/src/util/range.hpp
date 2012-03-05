/// @defgroup Range Iterator Range
/// @ingroup util

#ifndef UTILITIES_RANGE_HPP
#define UTILITIES_RANGE_HPP

#include <utility>
#include <iterator>

namespace util {

/// Represents a range specified by a pair of <code>[begin,end)</code> 
/// iterators.
/// @ingroup Range
template <class RanIt>
struct range : private std::pair<RanIt,RanIt>
{
	/// Type of the value stored in this range.
	typedef typename std::iterator_traits<RanIt>::value_type value_type;

	/// Type of reference to the value stored in this range.
	typedef typename std::iterator_traits<RanIt>::reference reference;

	/// Constructs a range with a pair of iterators.
	range(RanIt first, RanIt last)
		: std::pair<RanIt,RanIt>(first, last) { }

	/// Constructs a range from another range.
	template <class Iter>
	range(const range<Iter> &r)
		: std::pair<RanIt,RanIt>(r.begin(), r.end()) { }

	/// Constructs the entire range of a container.
	template <class Container>
	range(Container &c)
		: std::pair<RanIt,RanIt>(c.begin(), c.end()) { }

	/// Returns the begin iterator of the range.
	RanIt begin() const { return std::pair<RanIt,RanIt>::first; }

	/// Returns the end iterator of the range.
	RanIt end() const { return std::pair<RanIt,RanIt>::second; }

	/// Returns the number of elements in the range.
	size_t size() const { return end() - begin(); }

	/// Tests whether the range is empty.
	bool empty() const { return size() == 0; }

	/// Returns the element at a given index. 
	/// The underlying iterators must be random iterators.
	reference operator[](size_t i) { return std::pair<RanIt,RanIt>::first[i]; }
};

} // namespace util


#endif // UTILITIES_RANGE_HPP
