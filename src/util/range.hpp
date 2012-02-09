#ifndef UTILITIES_RANGE_HPP
#define UTILITIES_RANGE_HPP

#include <utility>

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
};

} // namespace util


#endif // UTILITIES_RANGE_HPP
