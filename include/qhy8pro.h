/*
 * qhy8pro.h -- declarations for the QHY8PRO camera
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <device.h>
#include <camera.h>

namespace qhy {

/**
 * \brief Camera class for the QHY8PRO camera
 *
 * This class encapsulates all the methods needed to handle the QHY8PRO
 * camera. This camera has a somewhat strange CCD from SONY, which needs
 * rather extensive demultiplexing. 
 */
class Qhy8Pro : public CameraOld {
public:
	Qhy8Pro(PDevice &device);
	virtual void	mode(const BinningMode& m);
protected:
	virtual void	demux(ImageBuffer& image, const Buffer& buffer);
private:
	virtual void	demux11(ImageBuffer& image, const Buffer& buffer);
	virtual void	demux22(ImageBuffer& image, const Buffer& buffer);
	virtual void	demux44(ImageBuffer& image, const Buffer& buffer);
};

} // namespace qhy
