/*
 * qhycooler.cpp -- the qhycooler program
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <qhydebug.h>
#include <device.h>
#include <sys/time.h>

namespace qhy {

static double	gettime() {
	double	t;
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	t += 0.0000001 * tv.tv_usec;
	return t;
}

DevicePtr	device;

void	inthandler(int sig) {
	device->dc201().stopCooler();
	device->dc201().pwm(0);
}

/**
 * \brief Main function for the qhyccd program
 */
int	qhycooler_main(int argc, char *argv[]) {
	qhydebugthreads = 1;
	qhydebugtimeprecision = 3;
	int	c;
	while (EOF != (c = getopt(argc, argv, "d")))
		switch (c) {
		case 'd':
			qhydebuglevel = LOG_DEBUG;
			break;
		}

	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "qhycooler started");

	// open a device, just for testing purposes
	device = getDevice(0x1618, 0x6003);

	double	temp = device->dc201().temperature();
	std::cout << "temperature is " << temp << std::endl;

	double	starttime = gettime();

	// turn of the cooler and plot data for a minute (to verify it is
	// turned off)
	device->dc201().pwm(0);

	int	counter = 20;
#if 0
	while (counter--) {
		temp = device->dc201().temperature();
		std::cout << (gettime() - starttime) << "," << temp << std::endl;
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
			temp = device->dc201().temperature();
			std::cout << (gettime() - starttime);
			std::cout << ",";
			std::cout << (int)testpwm;
			std::cout << ",";
			std::cout << temp;
			std::cout << std::endl;
			sleep(1);
		}
	}

	// turn of again and watch the temperature normalize again
	device->dc201().pwm(0);
	counter = 240;
	while (counter--) {
		temp = device->dc201().temperature();
		std::cout << (gettime() - starttime) << "," << temp << std::endl;
		sleep(1);
	}
#endif

#if 1
	// install the signal handler
	signal(SIGINT, inthandler);

	// start the regulator and try to reach absolute temperature 280
	device->dc201().settemperature(280.);
	device->dc201().startCooler();
	counter = 600;
	while ((counter--) && (device->dc201().cooler())) {
		sleep(1);
		temp = device->dc201().temperature();
		std::cout << (gettime() - starttime);
		std::cout << ",";
		std::cout << (int)device->dc201().pwm();
		std::cout << ",";
		std::cout << temp;
		std::cout << std::endl;
	}

	device->dc201().settemperature(270.);
	device->dc201().startCooler();
	counter = 600;
	while ((counter--) && (device->dc201().cooler())) {
		sleep(1);
		temp = device->dc201().temperature();
		std::cout << (gettime() - starttime);
		std::cout << ",";
		std::cout << (int)device->dc201().pwm();
		std::cout << ",";
		std::cout << temp;
		std::cout << std::endl;
	}

	device->dc201().settemperature(280.);
	device->dc201().startCooler();
	counter = 600;
	while ((counter--) && (device->dc201().cooler())) {
		sleep(1);
		temp = device->dc201().temperature();
		std::cout << (gettime() - starttime);
		std::cout << ",";
		std::cout << (int)device->dc201().pwm();
		std::cout << ",";
		std::cout << temp;
		std::cout << std::endl;
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
