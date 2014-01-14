/*
 * dc201.cpp -- DC201 implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller; Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_MATH_H
#include <math.h>
#endif /* HAVE_MATH_H */

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif /* HAVE_PTHREAD_H */

#include <qhylib.h>
#include <device.h>
#include <qhydebug.h>
#include <utils.h>

#include <libusb-1.0/libusb.h>

namespace qhy {

/**
 * \brief Locker class
 *
 * Auxiliary class to take care of locks when the go out of scope
 */
class Locker {
	pthread_mutex_t	*_lock;
public:
	Locker(pthread_mutex_t *lock) : _lock(lock) {
		pthread_mutex_lock(_lock);
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "lock acquired");
	}
	~Locker() {
		pthread_mutex_unlock(_lock);
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "lock released");
	}
};

#define	DC201_TIMEOUT	1000

const unsigned char	PDC201::read_endpoint = 0x81;
const unsigned char	PDC201::write_endpoint = 0x01;

/**
 * \brief PDC201 constructor
 *
 * We turn off the fan and the PWM in this class just to make sure
 * we know the state
 */
PDC201::PDC201(PDevice& device) : _device(device) {
	_pwm = 0;
	_fan = false;
	setFanPwm();

	// initialize the lock and conditation variable
	pthread_mutexattr_t	mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &mattr);
	pthread_mutex_unlock(&mutex);
	pthread_condattr_t	cattr;
	pthread_condattr_init(&cattr);
	pthread_cond_init(&cond, &cattr);

	// set temperature to something that we can always achieve
	_cooler = false;
	_settemperature = 273.15;
}

/**
 * \brief Destroy the resources associated with the PDC201
 */
PDC201::~PDC201() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "destructor called");
	// lock the mutexes
	pthread_mutex_lock(&mutex);

	// stop the thread if it is running
	if (cooler()) {
		stopCooler();
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "temp regulator thread stopped");

	// destroy the resources
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

/**
 * \brief Private class executing the actual transfers
 */
unsigned int	PDC201::transfer(unsigned char endpoint,
			unsigned char *buffer, unsigned int length) {
	//qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
	//	"transfer request for %lu bytes, endpoint %02x",
	//	length, endpoint);
	int	transferred_length;
	int	rc = libusb_bulk_transfer(_device.handle, endpoint,
			buffer, length, &transferred_length, DC201_TIMEOUT);
	if (rc) {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "transfer failed: %s",
			usbcause(rc).c_str());
		throw USBError(rc);
	}
	//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "%d bytes transferred",
	//	transferred_length);
	return transferred_length;
}

/**
 * \brief read a block of data from the camera into a buffer
 *
 * This method throws an USBError if the data cannot be transferred within
 * a short timeout (5 seconds)
 *
 * \return The number of bytes read
 */
unsigned int	PDC201::read(unsigned char *buffer, unsigned int length) {
	return transfer(read_endpoint, buffer, length);
}

/**
 * \brief write a block of data to the camera from a buffer
 *
 * This method throws an USBError if the transfer does not complete within
 * a short timeout
 */
unsigned int	PDC201::write(const unsigned char *buffer, unsigned int length) {
	return transfer(write_endpoint,
		const_cast<unsigned char *>(buffer), length);
}

static const double	a = 0.002679;
static const double	b = 0.000291;
static const double	c = 4.28e-7;

/**
 * \brief voltage to temperature conversion
 *
 * \param voltage	voltage drop on thermistor in mV
 * \return		absolute temperature in K
 */
double	PDC201::voltage2temperature(double voltage) {
	// this voltage must now be converted to a resistance value
	double	r = 33. / ((voltage / 1000) + 1.625) - 10;

	// and the resistance must be converted to a temperature
	if (r > 400) { r = 400; }
	if (r < 1) { r = 1; }
	double	lnr = log(r);
	double	T = 1 / (a + b * lnr + c * pow(lnr, 3));
	return T;
}

/**
 * \brief Clamp temperature to acceptable range
 *
 * \param temperature	absolute temperature
 */
static double	clampTemperature(const double temperature) {
	if (temperature < 223.15) { return 223.15; }
	if (temperature > 323.15) { return 323.15; }
	return temperature;
}

/**
 * \brief temperature to voltage conversion
 *
 * \param temperature	absolute chip temperature
 * \return		voltage on thermistor in mV
 */
double	PDC201::temperature2voltage(double temperature) {
	// make sure the temperature is in a valid range
	temperature = clampTemperature(temperature);

	// The inverse function uses the Cardano formula for the solution
	// of a cubic equation
	double	y = (a - 1/temperature) / c;
	double	x = sqrt(pow(b / (3 * c), 3) + (y * y) / 4);
	double	r = exp(cbrt(x - y/2) - cbrt(x + y/2));

	// convert the resistance to a voltage
	double	v = 33000 / (r + 10) - 1625;
	return v;
}

/**
 * \brief Read the temperature from the camera
 */
double	PDC201::voltage() {
	// perform a read of 4 bytes from the camera
	unsigned char	buffer[4];
	read(buffer, sizeof(buffer));
	//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "4 bytes: %02x %02x %02x %02x",
	//	buffer[0], buffer[1], buffer[2], buffer[3]);
	
	// convert bytes 1 and 2 to a signed short
	signed short	ts = buffer[1] * 256 + buffer[2];

	// convert to a double voltage
	double	v = 1.024 * ts;

	// return the voltage
	return v;
}

/**
 * \brief Read absolute temperature from the camera
 */
double	PDC201::temperature() {
	// convert the voltage to absolute temperature
	double	v = voltage();
	double	T = voltage2temperature(v);
	//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "v = %f, T = %f", v, T);
	return T;
}

/**
 * \brief Set Fan and PWM.
 *
 * We use this private function because fan and PWM can only be set together
 */
void	PDC201::setFanPwm() {
	unsigned char	buffer[3];
	buffer[0] = 0x01;
	buffer[1] = _pwm;
	// turn on the most significant bit to enable the PWM,
	// and the least significant to turn on the fan
	buffer[2] = ((_fan) ? 0x01 : 0x00) | ((_pwm) ? 0x80 : 0x00);

	//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "fan/pwm: send %02x %02x %02x",
	//	buffer[0], buffer[1], buffer[2]);
	write(buffer, 3);
}

/**
 * \brief Set the PWM to a certain value
 */
void	PDC201::pwm(unsigned char p) {
	_pwm = p;
	setFanPwm();
}

/**
 * \brief Turn the fan on/off
 */
void	PDC201::fan(bool f) {
	_fan = f;
	setFanPwm();
}

/**
 * \brief Set target temperature for cooler
 */
void	PDC201::settemperature(const double& t) {
	_settemperature = clampTemperature(t);
}

/**
 * \brief main function for the cooler
 *
 * This function just hands over control to the main function of the DC201
 * class.
 */
void	*cooler_main(void *arg) {
	PDC201	*dc201 = (PDC201 *)arg;
	dc201->main();
	return arg;
}

/**
 * \brief compute the new PWM value
 *
 * The PWM value must be an unsigned char between 0 and max, but the computation
 * in the main function could lead to negative values or values larger than
 * 255. This method takes care of these cases and always returns an
 * unsigned char value in the allowed range.
 */
static unsigned char	newpwm(double p, unsigned char max) {
	if (p < 0) {
		return 0;
	}
	if (p > max) {
		return max;
	}
	unsigned char	result = (unsigned char)p;
	return result;
}

/**
 * \brief Sleep for a number of seconds, but return early on cond variable
 *
 * \param seconds	time to sleep in seconds
 */
bool	PDC201::interruptiblesleep(double seconds) {
	// create a locally used mutex, because the cond variable functions
	// need one.
	pthread_mutex_t	tmutex;
	pthread_mutex_init(&tmutex, NULL);

	// compute the time specification for the end of the wait
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	struct timespec	ts;
	ts.tv_sec = tv.tv_sec + floor(seconds);
	ts.tv_nsec = tv.tv_usec * 1000 + (seconds - floor(seconds)) * 1000000000;
	if (ts.tv_nsec > 1000000000) {
		ts.tv_sec += ts.tv_nsec / 1000000000;
		ts.tv_nsec = ts.tv_nsec % 1000000000;
	}

	// wait for the timeout, or the signal
	pthread_mutex_lock(&tmutex);
	int	rc = pthread_cond_timedwait(&cond, &tmutex, &ts);
	bool	result = false;
	if (rc) {
		//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "wait timed out");
		result = false;
	} else {
		//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "condition signaled");
		result = true;
	}

	// cleanup the mutex
	pthread_mutex_unlock(&tmutex);
	pthread_mutex_destroy(&tmutex);
	return result;
}

/**
 * \brief Cool to a certain temperature
 *
 * \return the average cooling rate for the cooling operation
 */
double	PDC201::cooltolimit(double limit) {
	double	starttemperature = temperature();
	double	starttime = gettime();
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "starttemp: %f, starttime: %f",
		starttemperature, starttime);
	if (limit > starttemperature) {
		pwm(0);
		while (temperature() < limit) {
			if (interruptiblesleep(0.1)) {
				throw Interrupted();
			}
		}
	} else {
		pwm(255);
		while (temperature() > limit) {
			if (interruptiblesleep(0.1)) {
				throw Interrupted();
			}
		}
	}
	double	endtemperature = temperature();
	double	endtime = gettime();
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "endtemp: %f, endtime: %f",
		endtemperature, endtime);

	return fabs((endtemperature - starttemperature)
			/ (endtime - starttime));
}

/**
 * \brief Main method for the cooler
 */
void	PDC201::main() {
	// we lock the mutx, which will block the thread until the mutex
	// is release in the startCooler method
	pthread_mutex_lock(&mutex);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "start the regulator thread");
	pthread_mutex_unlock(&mutex);

	// the current temperature offsets defines what we do first
	double	delta = temperature() - _settemperature;

	// first cool or let heat as quickly as possible, then do the oposite
	// the difference in speed determines the initial PWM
	time_t	start = time(NULL);
	double	vup, vdown;
	try {
		if (delta < 0) {
			qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "heating required");
			vup = cooltolimit(_settemperature);
			vup = cooltolimit(_settemperature + 1);
			vdown = cooltolimit(_settemperature);
		} else {
			qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "cooling required");
			vdown = cooltolimit(_settemperature);
			vdown = cooltolimit(_settemperature - 1);
			vup = cooltolimit(_settemperature);
		}
	} catch (std::exception& x) {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
			"initial cooling phase stopped");
		pwm(0);
		return;
	}
	time_t	end = time(NULL);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
		"target temperature reached: %f in %d seconds",
		temperature(), end - start);

	// set the PWM to the previously computed target value
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "vup = %f, vdown = %f", vup, vdown);
	double	y = vup / vdown;
	double	initialpwm = 255 * y / (1 + y);
	pwm(newpwm(initialpwm, 255));
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "initial PWM %f", initialpwm);

	// the following code implements a PID controller for the temperature
	// A PID controller needs the integral and the derivative of the
	// errors we allocate some variables here for this purpose
	double	previous_voltage = voltage();
	double	previous_error = 0, current_error = 0;
	double	integral = 0;
	double	dt = 3.1;

	// The PID controller is defined by three constant gain factors
	double	Ku = 0.0365;
	double	Tu = 56;
#if 0	/* CLASSIC */
	double	Kp = 0.6 * Ku;
	double	Ki = 2 * Kp / Tu;
	double	Kd = Kp * Tu / 8;
#endif
#if 0	/* PESSEN */
	double	Kp = 0.7 * Ku;
	double	Ki = 2.5 * Kp / Tu;
	double	Kd = 0.15 * Kp * Tu;
#endif
#if 0	/* some overshoot */
	double	Kp = 0.33 * Ku;
	double	Ki = 2 * Kp / Tu;
	double	Kd = Kp * Tu / 3;
#endif
#if 0	/* no overshoot */
	double	Kp = 0.2 * Ku;
	double	Ki = 2 * Kp / Tu;
	double	Kd = Kp * Tu / 3;
#endif
	double	Kp = 0.02;
	double	Ki = 0.002;
	double	Kd = 0.2;

	// we also need the control variable, which may be outside the
	// range of what we can actually control, but the newpwm function
	// will take care of that
	double	control = pwm(); // get the current cooler PWM value

	// keep checking the 
	while (!endthread) {
		// find the current voltage, the regulator is based on the
		// voltage, not the temperature
		double	v = voltage();
		double	target = temperature2voltage(_settemperature);
		
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
			"new round v = %f, target = %f", v, target);

		// now update the state variables of the controller
		previous_error = current_error;
		current_error = v - target;
		integral += current_error;
		double	differential = current_error - previous_error;

		// regulator code: compute the new PWM value
#if 0
		if (v > temperature2voltage(_settemperature + 5)) {
			pwm(newpwm(pwm() + 5, 255));
		} else if (v > temperature2voltage(_settemperature + 1)) {
			pwm(newpwm(pwm() + 1, 255));
		} else if (v > temperature2voltage(_settemperature + 0.1)) {
			pwm(newpwm(pwm() + 0.1, 255));
		}
		if (v < temperature2voltage(_settemperature - 5)) {
			pwm(newpwm(pwm() - 5, 255));
		} else if (v < temperature2voltage(_settemperature - 1)) {
			pwm(newpwm(pwm() - 1, 255));
		} else if (v < temperature2voltage(_settemperature - 0.1)) {
			pwm(newpwm(pwm() - 0.1, 255));
		}
#endif
		control += Kp * current_error
			+ Ki * integral + Kd * differential;
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
			"error: %f, integral: %f, diff: %f, control: %f",
			current_error, integral, differential, control);

		// compute a pwm value that can actuall be applied to the
		// the chip
		unsigned char	newpwmvalue = newpwm(control, 255);
		pwm(newpwmvalue);
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "new PWM value: %d", _pwm);
		if (control < 0) { control = 0; }
		if (control > 255) { control = 255; }

		// test whether the thread should end
		if (endthread) {
			qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
				"end thread requested");
			return;
		}

		// now wait at most one second or until some other function
		// signals that we should 
		interruptiblesleep(3.1);
	}

	// clean up
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "regulator thread exiting");
}

/**
 * \brief start the cooler
 */
void	PDC201::startCooler() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "startCooler called");
	// lock the mutex to ensure we are the only ones manipulating
	// the thread resources
	Locker	locker(&mutex);

	// if the cooler is already turned on, just return
	if (cooler()) {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "cooler already running");
		return;
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "starting regulator thread");

	// ensure that the endthread variable is false, so we don't immediately
	// return
	endthread = false;

	// ok, the cooler is not on, so we start a new thread
	if (pthread_create(&thread, NULL, cooler_main, this)) {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "cannot launch the thread");
		pthread_mutex_unlock(&mutex);
		throw std::runtime_error("cannot start thread");
	}

	// if we get to this point, the we were successful starting the new
	// thread
	_cooler = true;

	// release the lock 
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "regulator thread started");
}

/**
 * \brief stop the cooler
 */
void	PDC201::stopCooler() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "stopCooler called");
	Locker	locker(&mutex);

	// if there is no cooler, just return
	if (!cooler()) {
		return;
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "stopping the regulator");

	// signal to the thread to stop
	endthread = true;
	pthread_cond_signal(&cond);

	// join the thread
	void	*result;
	pthread_join(thread, &result);

	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "thread has terminated");
	
	// when the join was successful, we know that the thread has stopped
	// so we can now change the local variables reflecting that
	_cooler = false;

	// release the lock
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "cooler stopped");
}

} // namespace qhy
