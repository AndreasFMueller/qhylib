/*
 * image.cpp -- image buffer implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <stdexcept>
#include <qhylib.h>
#include <qhydebug.h>

namespace qhy {

/**
 * \brief common memory allocation for ImageBuffers
 */
void	ImageBuffer::setup() {
	_npixels = _height * _width;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "allocating buffer for %d pixels", _npixels);
	buffersize = 2 * _npixels;
	_pixelbuffer = new unsigned short[_npixels];
}

/**
 * \brief Create an ImageBuffer
 */
ImageBuffer::ImageBuffer(unsigned int width, unsigned int height)
	: _width(width), _height(height) {
	setup();
}

/**
 * \brief Create an ImageBuffer 
 */
ImageBuffer::ImageBuffer(const ImageSize& size) {
	_width = size.width();
	_height = size.height();
	setup();
}

/**
 * \brief Destroy an ImageBuffer
 */
ImageBuffer::~ImageBuffer() {
	delete[] _pixelbuffer;
}

/**
 * \brief Access a pixel of the image buffer
 *
 * If range checking is enabled, this method throws a std::range_error
 * exception if the pixel coordinates are outside the image.
 */
unsigned short	ImageBuffer::p(unsigned int x, unsigned int y) const {
#if ENABLE_RANGECHECK
	if (!((x < _width) && (y < _height))) {
		throw std::range_error("pixel coordinates outside image");
	}
#endif /* ENABLE_RANGECHECK */
	return _pixelbuffer[x + _width * y];
}

/**
 * \brief Modifying access a pixel of the image buffer
 *
 * If range checking is enabled, this method throws a std::range_error
 * exception if the pixel coordinates are outside the image.
 */
unsigned short&	ImageBuffer::p(unsigned int x, unsigned int y) {
#if ENABLE_RANGECHECK
	if (!((x < _width) && (y < _height))) {
		throw std::range_error("pixel coordinates outside image");
	}
#endif /* ENABLE_RANGECHECK */
	return _pixelbuffer[x + _width * y];
}

/**
 * \brief Access a pixel of the raw pixel buffer
 *
 * For demultplexing operations, direct access to the pixel buffer
 * is needed, as provided by this function.
 * If range checking is enabled, this method throws a std::range_error
 * exception if the index is outside pixel buffer.
 */
const unsigned char&	ImageBuffer::operator[](unsigned int i) const {
#if ENABLE_RANGECHECK
	if (i >= buffersize) {
		throw std::range_error("index too large");
	}
#endif /* ENABLE_RANGECHECK */
	return ((unsigned char *)_pixelbuffer)[i];
}

/**
 * \brief Modifying access a pixel of the raw pixel buffer
 *
 * For demultplexing operations, direct access to the pixel buffer
 * is needed, as provided by this function.
 * If range checking is enabled, this method throws a std::range_error
 * exception if the index is outside pixel buffer.
 */
unsigned char&	ImageBuffer::operator[](unsigned int i) {
#if ENABLE_RANGECHECK
	if (i >= buffersize) {
		throw std::range_error("index too large");
	}
#endif /* ENABLE_RANGECHECK */
	return ((unsigned char *)_pixelbuffer)[i];
}

/**
 * \brief Access the active area of an image buffer
 *
 * This method uses points from within the active area, the point (0,0)
 * is a corner of the active area.
 */
unsigned short	ImageBuffer::ap(unsigned int x, unsigned int y) const {
#if ENABLE_RANGECHECK
	if (x >= _active.size.width()) {
		throw std::range_error("x offset too large");
	}
	if (y >= _active.size.height()) {
		throw std::range_error("y offset too large");
	}
#endif /* ENABLE_RANGECHECK */
	return p(x + _active.origin.x(),
		_active.size.height() - 1 - y + _active.origin.y());
}

/**
 * \brief Access pixels from active are or image buffer
 *
 * This method uses access to the active area if the active area is
 * defined, and returns the whole buffer otherwise.
 */
unsigned short	ImageBuffer::pixel(unsigned int x, unsigned int y) const {
	if (_active.size.empty()) {
		return ap(x, y);
	}
	return p(x, y);
}

/**
 * \brief Extract the active pixels from an image buffer
 */
ImageBufferPtr	ImageBuffer::active_buffer() const {
	// handle the case that we don't have an active area defined,
	// just copy the whole image
	if (_active.empty()) {
		ImageBufferPtr	result(new ImageBuffer(_width, _height));
		long	s = _width * _height;
		for (long i = 0; i < s; i++) {
			result->pixelbuffer()[i] = _pixelbuffer[i];
		}
		return result;
	}
	ImageSize	s = active_size();
	ImageBufferPtr	result(new ImageBuffer(s));
	long	i = 0;
	for (int y = 0; y < s.height(); y++) {
		for (int x = 0; x < s.width(); x++) {
			result->pixelbuffer()[i++] = ap(x, y);
		}
	}
	return result;
}

} // namespace qhy
