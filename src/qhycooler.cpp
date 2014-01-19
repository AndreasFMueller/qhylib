/*
 * qhycooler.cpp -- the qhycooler program
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <qhydebug.h>
#include <device.h>
#include <utils.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */

namespace qhy {

DevicePtr	device;

void	inthandler(int sig) {
	device->dc201().stopCooler();
	device->dc201().pwm(0);
}

static double	starttime = gettime();

class temppoint {
public:
	unsigned char	pwm;
	double	temp;
	temppoint(DC201& dc201) {
		pwm = device->dc201().pwm();
		temp = device->dc201().temperature();
	}
};

std::ostream&	operator<<(std::ostream& out, const temppoint& tp) {
	out << (gettime() - starttime);
	out << ",";
	out << (int)tp.pwm;
	out << ",";
	out << tp.temp;
	return out;
}

/**
 * \brief Main function for the qhyccd program
 */
int	qhycooler_main(int argc, char *argv[]) {
	qhydebugthreads = 1;
	qhydebugtimeprecision = 3;
	
	int	c;
	std::ostream	*f = NULL;
	while (EOF != (c = getopt(argc, argv, "dc:")))
		switch (c) {
		case 'd':
			qhydebuglevel = LOG_DEBUG;
			break;
		case 'c':
			f = new std::ofstream(optarg);
			break;
		}
	if (f == NULL) {
		f = &std::cout;
	}

	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "qhycooler started");

	// open a device, just for testing purposes
	device = getDevice(0x1618, 0x6003);

	double	temp = device->dc201().temperature();
	std::cout << "temperature is " << temp << std::endl;

	starttime = gettime();

	// turn of the cooler and plot data for a minute (to verify it is
	// turned off)
	device->dc201().pwm(0);

	int	counter = 20;
#if 1
	while (counter--) {
		(*f) << temppoint(device->dc201()) << std::endl;
		sleep(1);
	}
#endif

#if 0
	// now turn on the cooler to various values and find out what 
	// equilibrium temperature we will reach
	for (unsigned char testpwm = 0; testpwm <= 255; testpwm += 10) {
		device->dc201().pwm(testpwm);
		counter = 30;
		while (counter--) {
			(*f) << temppoint(device->dc201()) << std::endl;
			sleep(1);
		}
	}

	// turn of again and watch the temperature normalize again
	device->dc201().pwm(0);
	counter = 240;
	while (counter--) {
		sleep(1);
		(*f) << temppoint(device->dc201()) << std::endl;
	}
#endif

#if 1
	// install the signal handler
	signal(SIGINT, inthandler);

	// start the regulator and try to reach absolute temperature 280
	device->dc201().startCooler();

	int	oldt = 0, newt = 5;
	for (int i = 0; i < 24; i++) {
		double	temp = 290 - 10 * newt;
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "switch cooler to %.0f K", temp);
		device->dc201().settemperature(temp);
		counter = 600;
		while ((counter--) && (device->dc201().cooler())) {
			sleep(1);
			(*f) << temppoint(device->dc201()) << std::endl;
		}
		oldt = newt;
		do {
			newt = random() % 6;
		} while (newt == oldt);
	}

	// turn of the cooler
	device->dc201().stopCooler();
	device->dc201().pwm(0);
#endif

	return EXIT_SUCCESS;
}

} // namespace qhy

int	main(int argc, char *argv[]) {
	try {
		return qhy::qhycooler_main(argc, argv);
	} catch (const std::runtime_error& x) {
		std::cerr << "error in qhycooler: " << x.what() << std::endl;
	}
}
