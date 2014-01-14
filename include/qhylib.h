/*
 * qhylib.h -- exposed API of the QHYCCD library
 *
 * (c) 2014 Prof Dr Andreas Mueller,
 */
#ifndef qhy_qhylib_h
#define qhy_qhylib_h

#include <stdexcept>
#include <set>
#include <string>
#include <memory>

namespace qhy {

/**
 * \brief Exception for missing devices
 *
 * This exception is thrown if a device cannot be found
 */
class DeviceNotFound : public std::runtime_error {
public:
	DeviceNotFound(const std::string& cause) : std::runtime_error(cause) { }
};

/**
 * \brief Exception for USB errors
 *
 * When any USB related error happens, an USBError exception is thrown.
 */
class USBError : public std::runtime_error {
public:
	USBError(int usberror);
};

/**
 * \brief Exception for unsupported functions
 *
 * This exception is thrown if a camera does not support a certain function.
 * 
 */
class NotSupported : public std::runtime_error {
public:
	NotSupported(const std::string& cause) : std::runtime_error(cause) { }
};

/**
 * \brief Exception for unimplemented functions
 *
 * This is typically an internal error of the library, it indicates that
 * the programmer has not implemented a function that he should have
 * implemented. This is the reason why it derives from logic_error, not
 * from runtime_error.
 */
class NotImplemented : public std::logic_error {
public:
	NotImplemented(const std::string& cause) : std::logic_error(cause) { }
};

/**
 * \brief Exception for interrupts
 *
 * If some function is interrepted unexpectedly, this exception is thrown.
 */
class Interrupted : public std::runtime_error {
public:
	Interrupted(const std::string& cause) : std::runtime_error(cause) { }
	Interrupted() : std::runtime_error("interrupted") { }
};

/**
 * \brief DC201 device abstraction
 *
 * QHYCCD cameras use a power converter called DC201 which provides the
 * power for the cooler. This device is controlled over the separate 
 * endpoint 0x01. This class performs the communication with the
 * DC201 unit, and can thus be used to control the cooler
 */
class DC201 {
protected:
	/**
	 * \brief cache value for PWM
	 *
	 * PWM values can only be written, not read. If we want to
	 * be able to query the DC201 object for the current PWM
	 * value, we have to cache it here.
 	 */
	unsigned char	_pwm;
public:
	unsigned char	pwm() const { return _pwm; }
	virtual void	pwm(unsigned char p) = 0;
private:
	// private copy/assign prevents copying
	DC201(const DC201& other);
	DC201&	operator=(const DC201& other);
protected:
	DC201();
	virtual ~DC201();
public:
	virtual double	temperature() = 0;
private:
	bool	_cooler;
public:
	bool	cooler() const { return _cooler; }
	virtual void	startCooler() = 0;
	virtual void	stopCooler() = 0;
protected:
	double	_settemperature;
public:
	const double&	settemperature() const { return _settemperature; }
	virtual void	settemperature(const double& t) = 0;
};

/**
 * \brief Image Point class
 */
class ImagePoint : public std::pair<int, int> {
public:
	ImagePoint(int x, int y) : std::pair<int, int>(x, y) { }
	const int&	x() const { return first; }
	const int&	y() const { return second; }
	int&	x() { return first; }
	int&	y() { return second; }
};

/**
 * \brief Image Size class
 */
class ImageSize : public std::pair<int, int> {
public:
	ImageSize(int width, int height)
		: std::pair<int, int>(width, height) { }
	const int&	width() const { return first; }
	const int&	height() const { return second; }
	int&	width() { return first; }
	int&	height() { return second; }
};

/**
 * \brief Image Buffer returned by the camera
 *
 * An image buffer is just a block of memory consisting unsigned short
 * pixel values. It also knows its height and width, and pixels
 * can be accessed using pixel coordinates.
 */
class ImageBuffer {
	unsigned int	_width;
	unsigned int	_height;
	unsigned int	_npixels;
	unsigned int	buffersize;
	unsigned short	*_pixelbuffer;
public:
	unsigned int	width() const { return _width; }
	unsigned int	height() const { return _height; }
	unsigned int	npixels() const { return _npixels; }
	unsigned int	size() const { return buffersize; }
	unsigned short	*pixelbuffer() const { return _pixelbuffer; }
private:
	ImageBuffer(const ImageBuffer& other);
	ImageBuffer&	operator=(const ImageBuffer& other);
	void	setup();
public:
	ImageBuffer(unsigned int width, unsigned int height);
	ImageBuffer(const ImageSize& size);
	~ImageBuffer();
	unsigned short	p(unsigned int x, unsigned int y) const;
	unsigned short&	p(unsigned int x, unsigned int y);
	unsigned short	p(const ImagePoint& q) const { return p(q.x(), q.y()); }
	unsigned short&	p(const ImagePoint& q) { return p(q.x(), q.y()); }
	const unsigned char&	operator[](unsigned int i) const;
	unsigned char&	operator[](unsigned int i);
};

typedef std::shared_ptr<ImageBuffer>	ImageBufferPtr;

/**
 * \brief Binning mode class
 */
class BinningMode : public std::pair<int, int> {
public:
	BinningMode(int x, int y) : std::pair<int, int>(x, y) { }
	const int&	x() const { return first; }
	const int&	y() const { return second; }
	int&	x() { return first; }
	int&	y() { return second; }
};

/**
 * \brief Image rectangle class
 */
class ImageRectangle {
public:
	ImagePoint	origin;
	ImageSize	size;
	ImageRectangle(const ImagePoint o, const ImageSize s)
		: origin(o), size(s) { }
};

/**
 * \brief Camera class
 *
 * The camera class implements communication with the CCD part of the
 * camera.
 */
class Camera {
protected:
	// size of the CCD chip
	ImageSize	size;
public:
	const ImageSize&	chipsize() const { return size; }
protected:
	BinningMode	_mode;
public:
	const BinningMode&	mode() const { return _mode; }
	virtual	void	mode(const BinningMode& m) = 0;
	virtual ImageSize	imagesize() const = 0;
protected:
	double	_exposuretime;
public:
	const double&	exposuretime() const { return _exposuretime; }
	virtual void	exposuretime(double seconds) = 0;
	virtual void	startExposure() = 0;
	virtual void	cancelExposure() = 0;
	virtual ImageBufferPtr	getImage() = 0;
private:
	Camera(const Camera& other);
	Camera&	operator=(const Camera& other);
protected:
	Camera();
	virtual ~Camera();
};

/**
 * \brief Device objects for QHY devices
 *
 * Device objects handle all the resources needed to talk to a qhy device.
 */
class Device {
private:
	// protect the class from being copied
	Device(const Device& other);
	Device&	operator=(const Device& other);
protected:
	Device();
	virtual ~Device();
public:
	virtual DC201&	dc201() = 0;
	virtual Camera&	camera() = 0;
};

typedef std::shared_ptr<Device>	DevicePtr;

/**
 * \brief Function to create a new device
 */
DevicePtr	getDevice(unsigned short idVendor, unsigned short idProduct);

} // namespace qhy

#endif /* qhy_qhylib_h */
