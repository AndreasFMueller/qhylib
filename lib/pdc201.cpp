/*
 * dc201.cpp -- DC201 implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller; Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <cmath>
#include <chrono>
#include <qhylib.h>
#include <device.h>
#include <qhydebug.h>
#include <utils.h>

#include <libusb-1.0/libusb.h>

namespace qhy {

#define	DC201_TIMEOUT	10000

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
	std::unique_lock<std::recursive_mutex>	(_mutex);

	// stop the thread if it is running
	if (cooler()) {
		stopCooler();
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "temp regulator thread stopped");
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
	int	rc = libusb_bulk_transfer(_device.handle,
			endpoint, buffer, length, &transferred_length,
			DC201_TIMEOUT);
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
void	cooler_main(void *arg) {
	try {
		PDC201	*dc201 = (PDC201 *)arg;
		dc201->main();
	}  catch (const std::exception& x) {
		qhydebug(LOG_ERR, DEBUG_LOG, 0, "cooler thread failed: %s",
			x.what());
	}
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
	std::recursive_mutex	tmutex;

	// compute the time specification for the end of the wait
	long long	d = floor(1000000000 * seconds);
	std::chrono::nanoseconds	timeout(d);

	// wait for the timeout, or the signal
	std::unique_lock<std::recursive_mutex>	lock(tmutex);
	std::cv_status	status = _cond.wait_for(lock, timeout);
	bool	result = false;
	switch (status) {
	case std::cv_status::timeout:
		//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "wait timed out");
		result = false;
		break;
	case std::cv_status::no_timeout:
		//qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "condition signaled");
		result = true;
		break;
	}
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
 *
 * This method implements a PID controller, although with some quirks.
 *
 * Since the PWM value must be between 0 and 255, the controller very
 * often operates in a regime where it cannot be linear. In particular,
 * because it cannot drive the control input outside that range, it
 * takes much longer to correct an error, and as a consequence, the 
 * integral (I) term builds up heavily.
 *
 * To correct that, the integral term is reset to zero whenever the
 * error changes sign. The purpose of the integral term is to remove
 * a remaining offset, but when the error has changed sign, there is
 * no "remaining offset", so the integral term should be 0. At the
 * same time, since the control variable may now be completely different
 * from that actual pwm value, and it would take the controller quite
 * some time to get it back into the reasonable range, we reset the
 * control variable to the current pwm value.
 */
void	PDC201::main() {
	
	// we lock the mutx, which will block the thread until the mutex
	// is release in the startCooler method
	{
		std::unique_lock<std::recursive_mutex>	lock(_mutex);
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "start the regulator thread");
	}

	// the following code implements a PID controller for the temperature
	// A PID controller needs the integral and the derivative of the
	// errors we allocate some variables here for this purpose
	double	previous_temperature = temperature();
	double	current_temperature = previous_temperature;
	double	previous_error = previous_temperature - _settemperature;
	double	current_error = previous_error;
	double	dt = 3.1;
	double	differential = 0, previous_differential = 0;
	double	integral = 0;

	// if cooling is needed, we turn it up to a value proportional
	// to the difference
	double	initialpwm = 2 * current_error;
	if (initialpwm > 255) {
		pwm(255);
	} else if (initialpwm < 0) {
		pwm(0);
	} else {
		unsigned char	p = initialpwm;
		pwm(p);
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "initial pwm: %d", (int)pwm());

	// The PID controller is defined by three constant gain factors,
	// which are derived from T, T_t and k_s, which were determined
	// by experiments with the qhytransfer program
	double	T = 71;
	double	T_t = 4;
	double	k_s = 10. / 32.;

	// these values are standard Ziegler-Nichols gain factors
	double	k_P = 1.2 * (1 / k_s) * (T / T_t);
	double	T_I = 2 * T_t;
	double	T_D = 0.5 * T_t;

	double	k_I = k_P / T_I;
	double	k_D = k_P * T_D;

	// however, it turned out that they are a bit too aggressive, so
	// they were tuned by hand. The following values seem to work well
	// for at least one QHY8PRO
	k_P *= 0.4;
	k_I *= 0.2;
	k_D *= 0.25;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "k_P = %f, k_I = %f, k_D = %f",
		k_P, k_I, k_D);

	// We also need the control variable, which may be outside the
	// range of what we can actually control, but the newpwm function
	// will take care of that. The control variable only contains the
	// the P and D terms, because we want to be able to reset the
	// I term to zero on a zero crossing of the error. So we keep the
	// I term separate in the integral variable, and add it when we
	// compute the actual PWM value to apply to the camera using the
	// newpwm function.
	double	control = pwm(); // get the current cooler PWM value
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "initial control: %f", control);

	// keep checking the 
	while (!endthread) {
		// find the current voltage, the regulator is based on the
		// voltage, not the temperature
		current_temperature = temperature();
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
			"new round temp = %f, settemp = %f",
			current_temperature, _settemperature);

		// now update the state variables of the controller
		previous_error = current_error;
		current_error = current_temperature - _settemperature;
		previous_differential = differential;
		double	differential = (previous_error - current_error) / dt;

		// whenever the error changes sign, we reset the integral
		if ((previous_error * current_error) < 0) {
			qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "integral reset");
			integral = 0;
			control = pwm();
		}
		integral += current_error * dt;

		// regulator code: compute the new PWM value
		double	d_control
			= k_P * (current_error - previous_error);
		// If we get close to the target, we stat to use the D term.
		// This is intended to decrease overshoot
		if (fabs(current_error) < 10) {
			d_control += k_D * (previous_differential - differential);
		}
		control += d_control;
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
			"error: %f, diff: %f, integral: %f, d_control: %f, "
			"control: %f",
			current_error, differential, integral, d_control,
			control);

		// Compute a pwm value that can actuall be applied to the
		// the chip. At this point, we add in the integral term
		unsigned char	newpwmvalue = newpwm(control + k_I * integral,
			255);
		pwm(newpwmvalue);
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "new PWM value: %d", _pwm);

		// test whether the thread should end
		if (endthread) {
			qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
				"end thread requested");
			return;
		}

		// now wait at most one second or until some other function
		// signals that we should 
		interruptiblesleep(dt);
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
	std::unique_lock<std::recursive_mutex>	lock(_mutex);

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
	try {
		_thread = std::thread(cooler_main, this);
	} catch (std::exception& x) {
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "cannot launch thread: %s",
			x.what());
		throw std::runtime_error("cannot start thread");
	}

	// if we get to this point, the we were successful starting the new
	// thread
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "set _cooler to true");
	_cooler = true;

	// release the lock 
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "regulator thread created");
}

/**
 * \brief stop the cooler
 */
void	PDC201::stopCooler() {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "stopCooler called");
	std::unique_lock<std::recursive_mutex>	lock(_mutex);

	// if there is no cooler, just return
	if (!cooler()) {
		return;
	}
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "stopping the regulator");

	// signal to the thread to stop
	endthread = true;
	_cond.notify_all();

	// join the thread
	_thread.join();

	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "thread has terminated");
	
	// when the join was successful, we know that the thread has stopped
	// so we can now change the local variables reflecting that
	_cooler = false;

	// release the lock
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "cooler stopped");
}

} // namespace qhy
