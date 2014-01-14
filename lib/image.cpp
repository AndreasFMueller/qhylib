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
	if ((x < _width) && (y < _height)) {
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
	if ((x < _width) && (y < _height)) {
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

} // namespace qhy
