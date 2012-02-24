#ifndef UTILITIES_IO_FORMAT_HPP
#define UTILITIES_IO_FORMAT_HPP

#include <iostream>

namespace util {

/// Defines the file format to serialize a stratey tree.
enum FileFormat
{
	DefaultFormat = 0,
	TextFormat = 1,
	XmlFormat = 2,
	BinaryFormat = 3,
	CompressedFormat = 4,
};

#if 0
class withheader
{
	bool _value;

	static int Index() 
	{ 
		static int i = std::ios_base::xalloc();
		return i;
	}

public:
	withheader() : _value(true) { }
	withheader(bool value) : _value(value) { }

	friend std::ostream& operator << (std::ostream &os, const withheader
};
#endif

inline int header_index() 
{
	static int i = std::ios_base::xalloc();
	return i;
}

inline std::ostream& header(std::ostream &os)
{
	os.iword(header_index()) = 1;
	return os;
}

inline std::ostream& noheader(std::ostream &os)
{
	os.iword(header_index()) = 0;
	return os;
}

} // namespace util

#endif // UTILITIES_IO_FORMAT_HPP
