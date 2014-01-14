/*
 * buffer.cpp -- buffer allocation implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <buffer.h>
#include <stdexcept>
#include <string.h>
#include <qhydebug.h>

namespace qhy {

/**
 * \brief Create a buffer of a given name
 */
Buffer::Buffer(unsigned long l) : _length(l) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "allocated %d bytes", l);
	_data = new unsigned char[l];
}

/**
 * \brief Destroy a buffer
 */
Buffer::~Buffer() {
	delete[] _data;
}

/**
 * \brief Access a buffer
 */
const unsigned char&	Buffer::operator[](unsigned int i) const {
	if (i > _length) {
		throw std::range_error("index too large");
	}
	return _data[i];
}

/**
 * \brief Access a buffer
 */
unsigned char&	Buffer::operator[](unsigned int i) {
	if (i > _length) {
		throw std::range_error("index too large");
	}
	return _data[i];
}

/**
 * \brief Construct a BufferPointer
 */
BufferPointer::BufferPointer(Buffer& buffer) : _buffer(buffer) {
	_offset = 0;
}

/**
 * \brief Add a block of data to a buffer 
 *
 * This method adds a number of bytes to the buffer at a position the
 * bufferpointer points to.
 */
void	BufferPointer::append(unsigned char *newdata, unsigned long datalength) {
	if ((datalength + _offset) > _buffer.length()) {
		throw std::runtime_error("buffer too small");
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "append %lu bytes at offset %lu",
		datalength, _offset);
	memcpy(_buffer.data() + _offset, newdata, datalength);
	_offset += datalength;
}

/**
 * \brief Append another buffer
 */
void	BufferPointer::append(Buffer& buffer) {
	append(buffer, buffer.length());
}

/**
 * \brief Some data from another buffer
 */
void	BufferPointer::append(Buffer& buffer, unsigned long datalength) {
	append(buffer.data(), datalength);
}

} // namespace qhy
