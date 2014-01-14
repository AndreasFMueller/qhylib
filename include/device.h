/*
 * device.h -- Classes handling the communcation needs of a QHY device,
 *             This is the unexposed part of the API, it is not installed
 *             when the library is installed.
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef qhy_device_h
#define qhy_device_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif /* HAVE_PTHREAD_H */

// qhylib headers
#include <qhylib.h>
#include <reg.h>
#include <buffer.h>

// libusb
#include <libusb-1.0/libusb.h>

// standard C++ headers
#include <memory>
#include <set>


namespace qhy {

class PDevice; // forward declaration of the private device class

/**
 * \brief DC201 device abstraction
 *
 * The private version of the DC201 class hides the stuff nobody needs
 * to know about. It contains all the USB related variables, und
 * thus also uses a PDevice, which in turn contains the USB context
 * the device handle to use when talking to the device.
 */
class PDC201 : public DC201 {
static const unsigned char	read_endpoint;
static const unsigned char	write_endpoint;
	PDevice&	_device;
	unsigned int	transfer(unsigned char endpoint,
				unsigned char *buffer, unsigned int length);
	/**
	 * \brief cached value for Fan
	 *
	 * apparently it is  not possible to query fan or PWM from the DC201
	 * unit, one can only set it. This means that unless we keep the
	 * state in this class, it will be lost.
	 */
	bool 	_fan;
protected:
	unsigned int	read(unsigned char *buffer, unsigned int length);
	unsigned int	write(const unsigned char *buffer, unsigned int length);
private:
	void	setFanPwm();
public:
	PDC201(PDevice& device);
	virtual ~PDC201();
	bool	fan() const { return _fan; }
	void	fan(bool f);
	unsigned char	pwm() const { return _pwm; }
	void	pwm(unsigned char p);
	double	voltage();
	double	temperature();
private:
	// methods for temperature computation
	double	voltage2temperature(double voltage);
	double	temperature2voltage(double temperature);

	// members to handle the regulator thread
	pthread_t	thread;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	volatile bool	endthread;
	bool	interruptiblesleep(double seconds);
	double	cooltolimit(double limit);
public:
	void	main();
private:
	bool	_cooler;
public:
	bool	cooler() const { return _cooler; }
	void	startCooler();
	void	stopCooler();
public:
	void	settemperature(const double& t);
};

/**
 * \brief Camera class
 *
 * The camera class implements communication with the CCD part of the
 * camera.
 */
class PCamera : public Camera {
protected:
	PDevice&	_device;
	ccdreg	reg;
	
	// patch computation variables
	int	patch_size;
	int	total_patches;
	int	patch_number;
	unsigned long	transfer_size;
	virtual void	patch();

	int	readpatches(Buffer& buffer);

protected:
	// binnig modes available
	std::set<BinningMode>	binningmodes;
public:
	void	mode(const BinningMode& m);
	ImageSize	imagesize() const;
public:
	void	exposuretime(double seconds);
	void	startExposure();
	void	cancelExposure();
	virtual ImageBufferPtr	getImage();
protected:
	void	sendregisters();
	virtual void	demux(ImageBuffer& image, const Buffer& buffer);
public:
	PCamera(PDevice& device);
	virtual ~PCamera();
};

/**
 * \brief Device objects for QHY devices
 *
 * Device objects handle all the resources needed to talk to a qhy device.
 */
class PDevice : public Device {
	// libusb resources
	libusb_context	*ctx;
	libusb_device_handle	*handle;
public:
	PDevice(unsigned short idVendor, unsigned short idProduct);
	virtual ~PDevice();

	// control transfers
private:
	int	controltransfer(uint8_t bmRequestType, uint8_t bRequest,
			uint16_t wValue, uint16_t wIndex, unsigned char *data,
			uint16_t wLength, unsigned int timeout);
public:
	int	controlread(uint8_t bRequest, uint16_t wValue,
			uint16_t wIndex, unsigned char *data,
			uint16_t wLength, unsigned int timeout);
	int	controlwrite(uint8_t bRequest, uint16_t wValue,
			uint16_t wIndex, const unsigned char *data,
			uint16_t wLength, unsigned int timeout);

private:
	unsigned char	dataep;
	int	transfer(unsigned char ep, unsigned char *buffer, int length,
				unsigned int timeout);
public:
	int	read(unsigned char *buffer, int length,
				unsigned int timeout);
	int	write(const unsigned char *buffer, int length,
				unsigned int timeout);

private:
	PDC201	*_dc201;
public:
	DC201&	dc201();

	// allow the DC201 class access to the USB resources of the device
	friend class PDC201;

private:
	PCamera	*_camera;
public:
	Camera&	camera();
};

} // namespace qhy

#endif /* qhy_device_h */
