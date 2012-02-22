/// @defgroup BitMask Bit-Mask of fixed size
/// @ingroup util

#ifndef UTILITIES_BITMASK_HPP
#define UTILITIES_BITMASK_HPP

#include <cassert>
#include <type_traits>
#include <cstdint>
#include "intrinsic.hpp"

namespace util {

/**
 * Represents a bitmask of fixed size.
 * This class serves as a simpler and faster alternative to 
 * <code>std::bitset<N></code>.
 * @ingroup BitMask
 */
template <size_t Bits>
class bitmask
{
	static_assert(Bits <= 64, "Bitmask supports up to 64 bits.");

	typedef typename std::conditional<Bits <= 8, uint8_t,
		typename std::conditional<Bits <= 16, uint16_t,
		typename std::conditional<Bits <= 32, uint32_t,
		typename std::conditional<Bits <= 64, uint64_t,
		void>::type>::type>::type>::type value_type;

	value_type _value;

public:

	/// Creates an empty bitmask.
	bitmask() : _value(0) { }

	/// Creates a bitmask using the supplied mask.
	bitmask(value_type value) : _value(value) { }

	/// Gets the internal value of the mask.
	value_type value() const { return _value; }

	/// Tests a given bit.
	bool operator [] (size_t bit) const
	{
		assert(bit <= Bits);
		return (_value & ((value_type)1 << bit)) != 0;
	}

	/// Sets a given bit to zero.
	void reset(int bit)
	{
		assert(bit >= 0 && bit <= Bits);
		_value &= ~((value_type)1 << bit);
	}

	/// Sets all bits to zero.
	void reset() { _value = 0; }

	/// Returns @c true if there is exactly one bit set.
	bool unique() const
	{
		return _value && (_value & (_value - 1)) == 0;
	}

	/// Returns @c true if all bits are reset.
	bool empty() const { return _value == 0; }

	/// Returns the index of the least significant bit set.
	/// If no bit is set, returns @c -1.
	int smallest() const 
	{
		return _value == 0 ? -1 : util::intrinsic::bit_scan_forward(_value);
	}

#if 0
	int count() const
	{
		int n = 0;
		for (int i = 0; i < MM_MAX_COLORS; ++i)
		{
			if (value & (1 << i))
				++n;
		}
		return n;
	}

	void set_count(int count)
	{
		assert(count >= 0 && count <= MM_MAX_COLORS);
		value = (1 << count) - 1;
	}
#endif

	/// Returns a bitmask with the least significant @c count bits set.
	static bitmask<Bits> fill(size_t count)
	{
		assert(count >= 0 && count <= Bits);
		return ((value_type)1 << count) - 1;
	}
};

} // namespace util

#endif // UTILITIES_BITMASK_HPP
