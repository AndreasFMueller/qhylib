/*
 * factory.cpp -- the is the camera factory part of the device class
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <device.h>
#include <qhydebug.h>
#include <qhylib.h>
#include <qhy8pro.h>

namespace qhy {

/**
 * \brief Camera factory method
 */
Camera&	PDevice::camera() {
	// if there already is a camera object, just return it
	if (_camera) {
		return *_camera;
	}

	// if there is no camera yet, we have to create a camera. What camera
	// object we construct depends on the vendor id and product id of
	// the camera
	struct libusb_device_descriptor	desc;
	int	rc = libusb_get_device_descriptor(libusb_get_device(handle),
			&desc);
	if (rc < 0) {
		qhydebug(LOG_ERR, DEBUG_LOG, 0,
			"cann0t get device descriptor: %s",
			libusb_strerror((enum libusb_error)rc));
		throw USBError(rc);
	}
	if (desc.idVendor != 0x1618) {
		throw NotSupported("camera vendor not known");
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "creating device %04x/%04x",
		desc.idVendor, desc.idProduct);

	// create cameras depending on the product id
	switch (desc.idProduct) {
	case 0x6003:
		_camera = new Qhy8Pro(*this);
		break;
	default:
		break;
	}

	// if we have constructed a camera
	if (_camera) {
		return *_camera;
	}

	// if we get to this point, then we were not successful in creating
	// a camera, so we throw an exception indicating that the device
	// is not supported as a camera
	throw NotSupported("no camera found");
}

} // namespace qhy
