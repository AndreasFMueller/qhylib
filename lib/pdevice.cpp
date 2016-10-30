/*
 * device.cpp -- implementation of the QHY device class
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#include <device.h>
#include <qhylib.h>
#include <qhydebug.h>
#include <utils.h>

namespace qhy {

/**
 * \brief Create a new Device object
 */
PDevice::PDevice(unsigned short idVendor, unsigned short idProduct) {
	// initialize context and handle, to make sure 
	ctx = NULL;
	handle = NULL;

	// initialize the pointers
	_dc201 = NULL;
	_camera = NULL;

	// initialize the USB context for this device
	int	rc = libusb_init(&ctx);
	if (rc) {
		qhydebug(LOG_ERR, DEBUG_LOG, 0, "cannot init USB: %s (%d)",
			libusb_strerror((enum libusb_error)rc), rc),
		throw USBError(rc);
	}

	// get a device handle for the device
	handle = libusb_open_device_with_vid_pid(ctx, idVendor, idProduct);
	if (NULL == handle) {
		qhydebug(LOG_ERR, DEBUG_LOG, 0, "could not open %04x/%04x",
			idVendor, idProduct);
		libusb_exit(ctx);
		throw USBError(LIBUSB_ERROR_NOT_FOUND);
	}

#if HAVE_LIBUSB_SET_DEBUG
#if USBDEBUG
	if (qhydebuglevel == LOG_DEBUG) {
		libusb_set_debug(ctx, 4 /* LIBUSB_LOG_LEVEL_DEBUG */);
	}
#endif
#endif

	// get the device
	libusb_device	*dev = libusb_get_device(handle);

	// get the active config descriptor
	libusb_config_descriptor	*config = NULL;
	libusb_get_active_config_descriptor(dev, &config);

	// get a descriptor for interface 0
	const libusb_interface	*interface = config->interface;
	const libusb_interface_descriptor	*ifdesc = interface->altsetting;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "interface has %d endpoints",
		ifdesc->bNumEndpoints);
	for (int epidx = 0; epidx < ifdesc->bNumEndpoints; epidx++) {
		const libusb_endpoint_descriptor	*endpoint
			= ifdesc->endpoint + epidx;
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "endpoint %d: %02x", epidx,
			endpoint->bEndpointAddress);
		// scan all endpoints and look for an IN endpoint with 512
		// bytes maximum packet size, this is the data endpoint
		if (endpoint->bEndpointAddress & 0x80) {
			qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
				"%02x has packet size %d",
				(int)endpoint->bEndpointAddress,
				(int)endpoint->wMaxPacketSize);
			if (endpoint->wMaxPacketSize >= 512) {
				dataep = endpoint->bEndpointAddress;
			}
		}
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "data endpoint is %02x", (int)dataep);

	// clean up the descriptors
	libusb_free_config_descriptor(config);

	// claim the interface
	rc = libusb_claim_interface(handle, 0);
	if (rc) {
		qhydebug(LOG_ERR, DEBUG_LOG, 0, "cannot claim interface 0: %s",
			libusb_strerror((enum libusb_error)rc));
		libusb_close(handle);
		libusb_exit(ctx);
		throw USBError(rc);
	}

	// report success in openeing the device
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "device %04x/%04x opened",
		idVendor, idProduct);
}

/**
 * \brief Destroy the device object
 */
PDevice::~PDevice() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "destroying Device");
	if (_dc201) {
		delete _dc201;
	}
	if (_camera) {
		delete _camera;
	}
	// close the device
	if (handle) {
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		handle = NULL;
	}
	// deinitialize the context
	if (ctx) {
		libusb_exit(ctx);
		ctx = NULL;
	}
}

/**
 * \brief Get the DC201 object
 */
DC201&	PDevice::dc201() {
	if (!_dc201) {
		_dc201 = new PDC201(*this);
	}
	return *_dc201;
}

/**
 * \brief control transfer encapsulation
 *
 * Performs a single control transfer
 */
int	PDevice::controltransfer(uint8_t bmRequestType, uint8_t bRequest,
		uint16_t wValue, uint16_t wIndex, unsigned char *data,
		uint16_t wLength, unsigned int timeout) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "%02x %02x v=%04x, i=%04x, l=%d",
		(int)bmRequestType,
		(int)bRequest, (int)wValue, (int)wIndex, (int)wLength);
	int	rc = libusb_control_transfer(handle, bmRequestType, bRequest,
			wValue, wIndex, data, wLength, timeout);
	if (rc < 0) {
		qhydebug(LOG_ERR, DEBUG_LOG, 0,
			"control transfer failed: %s (%d)",
			libusb_strerror((enum libusb_error)rc), rc);
		throw USBError(rc);
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "%d bytes transferred", rc);
	return rc;
}

/**
 * \brief Control transfer from device to host
 */
int	PDevice::controlread(uint8_t bRequest, uint16_t wValue,
		uint16_t wIndex, unsigned char *data,
		uint16_t wLength, unsigned int timeout) {
	int	rc = controltransfer(0xc0, bRequest, wValue, wIndex,
		data, wLength, timeout);
	if (rc < 0) {
		qhydebug(LOG_ERR, DEBUG_LOG, 0, "cannot read control: %s",
			libusb_strerror((enum libusb_error)rc));
	} else {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "receive buffer:");
	}
	logbuffer(data, wLength);
	return rc;
}

/**
 * \brief Control transfer from host to device
 */
int	PDevice::controlwrite(uint8_t bRequest, uint16_t wValue,
		uint16_t wIndex, const unsigned char *data,
		uint16_t wLength, unsigned int timeout) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "send buffer:");
	logbuffer(data, wLength);
	return controltransfer(0x40, bRequest, wValue, wIndex,
		const_cast<unsigned char *>(data), wLength, timeout);
}

/**
 * \brief Perform a data transfer with the data endpoint
 */
int	PDevice::transfer(unsigned char ep, unsigned char *buffer,
		int length, unsigned int timeout) {
	int	transferred;
	int	rc = libusb_bulk_transfer(handle, ep, buffer, length,
			&transferred, timeout);
	if (rc < 0) {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "transfer failed: %s (%d)",
			libusb_strerror((enum libusb_error)rc), rc);
		throw USBError(rc);
	}
	return transferred;
}

/**
 * \brief Read data from the data endpoint
 */
int	PDevice::read(unsigned char *buffer, int length,
		unsigned int timeout) {
	return transfer(dataep | 0x80, buffer, length, timeout);
}

/**
 * \brief Write data to the data endpoint
 */
int	PDevice::write(const unsigned char *buffer, int length,
		unsigned int timeout) {
	return transfer(dataep & ~0x80, const_cast<unsigned char *>(buffer),
		length, timeout);
}

} // namespace qhy
