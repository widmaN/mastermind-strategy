#ifndef UTILITIES_ZIP_ITERATOR
#define UTILITIES_ZIP_ITERATOR

#include <utility>

namespace util {

template <class TKey, class TVal>
struct key_value_pair : public std::pair<TKey,TVal>
{
	key_value_pair(const TKey &key, const TVal &value)
		: std::pair<TKey,TVal>(key, value) { }

	key_value_pair(const key_value_pair<TKey&,TVal&> &other)
		: std::pair<TKey,TVal>(other.first, other.second) { }

	template <class U1, class U2>
	key_value_pair& operator = (const key_value_pair<U1,U2> &other)
	{
		first = other.first;
		second = other.second;
		return *this;
	}

	TKey& key() { return first; }
	const TKey& key() const { return first; }

	TVal& value() { return value; }
	const TVal& value() const { return value; }

	template <class U1, class U2>
	bool operator < (const key_value_pair<U1,U2> &other) const
	{
		return key() < other.key();
	}
};

template <class Iter1, class Iter2>
class zip_iterator : private std::pair<Iter1,Iter2>
{
	typedef typename std::iterator_traits<Iter1> _t1;
	typedef typename std::iterator_traits<Iter2> _t2;

public:

	typedef std::random_access_iterator_tag iterator_category;
	typedef typename _t1::difference_type difference_type;
#if 0
	typedef std::pair<typename _t1::value_type, typename _t2::value_type> value_type;
	typedef std::pair<typename _t1::value_type&, typename _t2::value_type&> reference;
#else
	typedef key_value_pair<typename _t1::value_type, typename _t2::value_type> value_type;
	typedef key_value_pair<typename _t1::value_type&, typename _t2::value_type&> reference;
#endif
	typedef void pointer;

	zip_iterator(Iter1 it1, Iter2 it2) 
		: std::pair<Iter1,Iter2>(it1, it2) { }

	reference operator * ()
	{
		return reference(*first, *second);
	}

	zip_iterator& operator ++ ()
	{
		++first;
		++second;
		return *this;
	}

	zip_iterator operator ++ (int)
	{
		zip_iterator tmp = *this;
		++first;
		++second;
		return tmp;
	}

	zip_iterator& operator -- ()
	{
		--first;
		--second;
		return *this;
	}

	zip_iterator operator -- (int)
	{
		zip_iterator tmp = *this;
		--first;
		--second;
		return tmp;
	}

	zip_iterator operator + (difference_type n) const 
	{
		return zip_iterator(first + n, second + n);
	}

	zip_iterator operator - (difference_type n) const 
	{
		return zip_iterator(first - n, second - n);
	}

	difference_type operator - (const zip_iterator &it) const
	{
		return first - it.first;
	}

	bool operator == (const zip_iterator &it) const
	{
		return first == it.first && second == it.second;
	}

	bool operator != (const zip_iterator &it) const
	{
		return ! operator == (it);
	}

	bool operator < (const zip_iterator &it) const
	{
		return first < it.first;
	}

	Iter1 key() { return first; }
	Iter2 value() { return second; }
};

template <class Iter1, class Iter2>
zip_iterator<Iter1,Iter2> make_zip(Iter1 it1, Iter2 it2)
{
	return zip_iterator<Iter1,Iter2>(it1, it2);
}

/*
template <class Iter1, class Iter2>
bool zip_comparer(const zip_iterator<Iter1,Iter2> &a, const zip_iterator<Iter1,Iter2> &b)
{
	return *a.key() < *b.key();
}
*/

} // namespace util

#endif // UTILITIES_ZIP_ITERATOR
