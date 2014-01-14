/*
 * image.cpp -- image buffer implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <stdexcept>
#include <qhylib.h>
#include <qhydebug.h>

namespace qhy {

void	ImageBuffer::setup() {
	_npixels = _height * _width;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "allocating buffer for %d pixels", _npixels);
	buffersize = 2 * _npixels;
	_pixelbuffer = new unsigned short[_npixels];
}

ImageBuffer::ImageBuffer(unsigned int width, unsigned int height)
	: _width(width), _height(height) {
	setup();
}

ImageBuffer::ImageBuffer(const ImageSize& size) {
	_width = size.width();
	_height = size.height();
	setup();
}

ImageBuffer::~ImageBuffer() {
	delete[] _pixelbuffer;
}

unsigned short	ImageBuffer::p(unsigned int x, unsigned int y) const {
	if ((x < _width) && (y < _height)) {
		throw std::range_error("pixel coordinates outside image");
	}
	return _pixelbuffer[x + _width * y];
}

unsigned short&	ImageBuffer::p(unsigned int x, unsigned int y) {
	if ((x < _width) && (y < _height)) {
		throw std::range_error("pixel coordinates outside image");
	}
	return _pixelbuffer[x + _width * y];
}

const unsigned char&	ImageBuffer::operator[](unsigned int i) const {
	if (i >= buffersize) {
		throw std::range_error("index too large");
	}
	return ((unsigned char *)_pixelbuffer)[i];
}

unsigned char&	ImageBuffer::operator[](unsigned int i) {
	if (i >= buffersize) {
		throw std::range_error("index too large");
	}
	return ((unsigned char *)_pixelbuffer)[i];
}

} // namespace qhy
