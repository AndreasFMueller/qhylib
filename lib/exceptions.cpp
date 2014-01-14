/*
 * exceptions.cpp
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libusb-1.0/libusb.h>
#include <qhylib.h>
#include <utils.h>

namespace qhy {

#if HAVE_LIBUSB_STRERROR
std::string	usbcause(int usberror) {
	return std::string(libusb_strerror((enum libusb_error)usberror));
}
#else
std::string	usbcause(int usberror) {
	switch (usberror) {
	case LIBUSB_SUCCESS:
		return std::string("Success");
	case LIBUSB_ERROR_IO:
		return std::string("IO Error");
	case LIBUSB_ERROR_INVALID_PARAM:
		return std::string("Invalid Parameter");
	case LIBUSB_ERROR_ACCESS:
		return std::string("Access Error");
	case LIBUSB_ERROR_NO_DEVICE:
		return std::string("No such device");
	case LIBUSB_ERROR_NOT_FOUND:
		return std::string("Not found");
	case LIBUSB_ERROR_BUSY:
		return std::string("Busy");
	case LIBUSB_ERROR_TIMEOUT:
		return std::string("Timeout");
	case LIBUSB_ERROR_OVERFLOW:
		return std::string("Overflow");
	case LIBUSB_ERROR_PIPE:
		return std::string("Pipe error");
	case LIBUSB_ERROR_INTERRUPTED:
		return std::string("Interrupted");
	case LIBUSB_ERROR_NO_MEM:
		return std::string("Out of memory");
	case LIBUSB_ERROR_NOT_SUPPORTED:
		return std::string("Not supported");
	case LIBUSB_ERROR_OTHER:
		return std::string("Other error");
	}
	return std::string("unknown");
}
#endif

USBError::USBError(int usberror) : std::runtime_error(usbcause(usberror)) {
}

} // namespace qhy
