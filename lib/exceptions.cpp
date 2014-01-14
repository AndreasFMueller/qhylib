/*
 * exceptions.cpp
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <libusb-1.0/libusb.h>
#include <qhylib.h>

namespace qhy {

static std::string	usbcause(int usberror) {
	return std::string(libusb_strerror((enum libusb_error)usberror));
}

USBError::USBError(int usberror) : std::runtime_error(usbcause(usberror)) {
}

} // namespace qhy
