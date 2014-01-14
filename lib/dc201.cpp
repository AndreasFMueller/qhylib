/*
 * dc201.cpp -- DC201 implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller; Hochschule Rapperswil
 */
#include <qhylib.h>

namespace qhy {

/**
 * \brief Construct a new DC201 object
 */
DC201::DC201() {
	_pwm = 0;
	_settemperature = 271.25;
	_cooler = false;
}

/**
 * \brief Destroy the DC201 object
 */
DC201::~DC201() {
}


} // namespace qhy
