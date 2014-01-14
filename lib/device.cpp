/*
 * device.cpp -- implementation of the QHY device class
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <qhylib.h>
#include <device.h>

namespace qhy {

/**
 * \brief Construct a new Device
 */
Device::Device() {
	// this is just an empty constructor, only the derived classes
	// have real work to do in the constructor
}

/**
 * \brief Destroy the Device
 */
Device::~Device() {
}


DevicePtr	getDevice(unsigned short idVendor, unsigned short idProduct) {
	return DevicePtr(new PDevice(idVendor, idProduct));
}

} // namespace qhy
