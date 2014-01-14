/*
 * camera.cpp -- camera class implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <qhylib.h>

namespace qhy {

/**
 * \brief Create a camera object
 */
Camera::Camera() : size(0, 0), _mode(1, 1), _exposuretime(0) {
}

/**
 * \brief Destroy a camera object
 */
Camera::~Camera() {
}

} // namespace qhy
