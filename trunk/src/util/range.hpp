#ifndef UTILITIES_RANGE_HPP
#define UTILITIES_RANGE_HPP

#include <utility>
#include <iterator>

namespace util {

template <class RanIt>
struct range : private std::pair<RanIt,RanIt>
{
	range(RanIt first, RanIt last)
		: std::pair<RanIt,RanIt>(first, last) { }

	template <class Iter>
	range(const range<Iter> &r)
		: std::pair<RanIt,RanIt>(r.begin(), r.end()) { }

	template <class Container>
	range(Container &c)
		: std::pair<RanIt,RanIt>(c.begin(), c.end()) { }

	RanIt begin() const { return std::pair<RanIt,RanIt>::first; }
	RanIt end() const { return std::pair<RanIt,RanIt>::second; }
	size_t size() const { return end() - begin(); }
	bool empty() const { return size() == 0; }

	typedef typename std::iterator_traits<RanIt>::value_type value_type;
	typedef typename std::iterator_traits<RanIt>::reference reference;
	reference operator[](size_t i) { return std::pair<RanIt,RanIt>::first[i]; }
};

} // namespace util


#endif // UTILITIES_RANGE_HPP
