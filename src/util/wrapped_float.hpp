#ifndef UTILITIES_WRAPPED_FLOAT_HPP
#define UTILITIES_WRAPPED_FLOAT_HPP

#include <limits>

namespace util {

template <class T, unsigned int NEps>
class wrapped_float
{
	T _value;

public:
	wrapped_float() : _value(0) { }
	explicit wrapped_float(const T &value) : _value(value) { }
	T value() const { return _value; }
};

template <class T, unsigned int NEps>
bool operator < (const wrapped_float<T,NEps> &x, const wrapped_float<T,NEps> &y)
{
	return x.value() < y.value() - NEps * std::numeric_limits<T>::epsilon();
}

#if 0
template <class T, T Eps>
bool operator == (const wrapped_float<T,Eps> &x, const wrapped_float<T,Eps> &y)
{
	return !(x < y || y < x);
}
#endif

} // namespace util

#endif // UTILITIES_WRAPPED_FLOAT_HPP
