/*
 * camera.h -- some derived classes for cameras
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef qhy_camera_h
#define qhy_camera_h

#include <device.h>

namespace qhy {

/**
 * \brief Old camera class
 *
 * Old camera types have a slightly different method to compute the patch 
 * parameters.
 */
class CameraOld : public PCamera {
protected:
	virtual void	patch();
public:
	CameraOld(PDevice& device) : PCamera(device) { }
};

/**
 * \brief New camera class
 *
 * New cameras have their own patch method to compute the patch parameters
 */
class CameraNew : public PCamera {
protected:
	virtual void	patch();
public:
	CameraNew(PDevice& device) : PCamera(device) { }
};

} // namespace qhy

#endif /* qhy_device_h */
