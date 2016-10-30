/*
 * camera.cpp -- camera class implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <device.h>
#include <qhylib.h>
#include <qhydebug.h>
#include <buffer.h>
#include <camera.h>
#include <cstring>

namespace qhy {

extern void     logbuffer(const unsigned char *data, int length);

#define CONTROL_TIMEOUT	500

/**
 * \brief Create a camera object
 */
PCamera::PCamera(PDevice& device) : _device(device) {
}

/**
 * \brief Destroy a camera object
 */
PCamera::~PCamera() {
}

void	PCamera::sendregisters() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "sendregisters()");
	// convert the register class into a control block
	register_block	block(reg);

	// compute patch parameters
	this->patch();
	block.setpatchnumber(patch_number);

	// log the information about the patch stuff
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
		"patch_size = %d, transfer_size = %d, total_patches = %d, "
		"patch_number = %d",
		patch_size, transfer_size, total_patches, patch_number);

	// send the control request to the camera
	_device.controlwrite(0xb5, 0, 0, block.block(), 64, CONTROL_TIMEOUT);
	//_device.controlwrite(0xb5, 0, 0, block.block(), 64, CONTROL_TIMEOUT);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "control transfer complete");
}

/**
 * \brief basic common patch computation part
 */
void	PCamera::patch() {
	transfer_size = reg.LineSize * reg.VerticalSize * 2
				+ reg.TopSkipPix * 2;
	if (transfer_size % patch_size) {
		total_patches = transfer_size / patch_size  + 1;
	} else {
		total_patches = transfer_size / patch_size;
	}
}

/**
 * \brief Check whether the binning mode is actually supported by the camera
 *
 * \param m	Binning mode parameters
 */
void	PCamera::mode(const BinningMode& m) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "binning mode: %dx%d",
		m.x(), m.y());
	if (binningmodes.find(m) == binningmodes.end()) {
		throw NotSupported("binning mode not supported");
	}
	_mode = m;
}

/**
 * \brief set the exposure time
 *
 * The exposure time of the camera is in milliseconds, while the
 * argument of this method is in seconds.
 * \param seconds	desired exposure time in seconds
 */
void	PCamera::exposuretime(double seconds) {
	_exposuretime = seconds;
	reg.Exptime = 1000 * seconds;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "exposure time: %d ms",
		reg.Exptime);
}

/**
 * \brief read the image as a set of patches
 *
 * QHY cameras deliver data in patches so we need method to read these
 * patches into a contiguous buffer.
 */
int	PCamera::readpatches(Buffer& target) {
	// start reading patches
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
		"read %d data patches into buffer of size %ld",
		patch_number, target.length());

	// verify that the buffer is large enough
	unsigned int	l = patch_size * total_patches;
	if (target.length() < l) {
		throw std::runtime_error("buffer is not large enough");
	}

	// compute the timeout which we want to apply
	int	timeout = 1000 * (_exposuretime + 30);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "exposuretime = %f, timeout %d",
		_exposuretime, timeout);

	// allocate a buffer for reading data 
	Buffer	buffer(patch_size);
	BufferPointer	bp(target);
	int	transferred;
	int	totalbytes = 0;
	for (int patchno = 0; patchno < total_patches; patchno++) {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "reading %d bytes",
			patch_size);
		transferred = _device.read(buffer.data(), patch_size, timeout);
//logbuffer(buffer.data(), patch_size);

		bp.append(buffer, transferred);
		totalbytes += transferred;
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "patch %d: size %d",
			patchno, transferred);
		// all following transfers should be done with a shorter
		// timeout of at most 1 second
		timeout = 1000;
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "all patches read, %d bytes",
		totalbytes);
	return totalbytes;
}

/**
 * \brief Cancel an exposure
 */
void	PCamera::cancelExposure() {
	throw NotImplemented("cancelling an exposure is not yet supported");
}

/**
 * \brief Start an exposure
 */
void	PCamera::startExposure() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "start an exposure");
	// send the registers with all the parameters to the camera
	sendregisters();

	// start video 
	unsigned char	buf[1];
	buf[0] = 100;
	_device.controlwrite(0xb3, 0, 0, buf, 1, CONTROL_TIMEOUT);
}

/**
 * \brief Compute the image size with this binning mode
 */
ImageSize	PCamera::imagesize() const {
	int	width = size.width() / _mode.x();
	int	height = size.height() / _mode.y();
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "image size: %d x %d",
		width, height);
	return ImageSize(width, height);
}

/**
 * \brief Get an image from the camera
 *
 * This includes waiting for the exposure to complete
 */
ImageBufferPtr	PCamera::getImage() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "retrieving the image");

	// create a data buffer
	Buffer	rawbuffer(total_patches * patch_size);

	int	l = readpatches(rawbuffer);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "%d bytes received", l);

	// prepare a pixel buffer
	ImageSize	imgsize = imagesize();
	ImageBuffer	*image = new ImageBuffer(imgsize);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "%d x %d image buffer allocated",
		image->width(), image->height());

	// convert the pixel into the image buffer
	this->demux(*image, rawbuffer);

	return ImageBufferPtr(image);
}

/**
 * \brief Patch size computations for old cameras
 */
void	CameraOld::patch() {
	PCamera::patch();
	// compute patch parameters for old cameras
	if (transfer_size % patch_size) {
		patch_number = total_patches * patch_size - transfer_size;
	} else {
		patch_number = 0;
	}
}

/**
 * \brief Patch size computations for new cameras
 */
void	CameraNew::patch() {
	PCamera::patch();
	// compute patch parameters for new cameras
	if (transfer_size % patch_size) {
		patch_number = (total_patches * patch_size - transfer_size) / 2 + 16;
	} else {
		patch_number = 16;
	}
}

/**
 * \brief Demultiplex the image
 */
void	PCamera::demux(ImageBuffer& image, const Buffer& buffer) {
	long	l = image.width() * image.height() * 2;
//	logbuffer(buffer.data(), l);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "copy %d bytes pixels", l);
	memcpy(image.pixelbuffer(), buffer.data(), l);
//	logbuffer((unsigned char *)image.pixelbuffer(), l);
}

/**
 * \brief Set the download speed
 */
void	PCamera::downloadSpeed(enum DownloadSpeed speed) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "download speed: %s",
		(speed == Camera::Low) ? "low" : "high");
	switch (speed) {
	case Low:
		reg.DownloadSpeed = 0;
		break;
	case High:
		reg.DownloadSpeed = 1;
		break;
	}
}

} // namespace qhy
