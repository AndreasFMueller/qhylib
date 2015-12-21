/*
 * qhytransfer.cpp
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <qhylib.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <qhydebug.h>

namespace qhy {

DevicePtr	device;

double	transfer(double omega) {
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "omega = %f", omega);
	device->dc201().pwm(64);
	sleep(60);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "initial sequence complete");
	double	T_min = 1000;
	double	T_max = 0;
	double	t_max = 2 * M_PI / omega;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "max time: %f", t_max);
	for (double t = 0; t <= t_max; t += 3.) {
		unsigned char	x = 64 + 32 * sin(omega * t);
		device->dc201().pwm(x);
		double	T = device->dc201().temperature();
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "%d %f", (int)x, T);
		if (T > T_max) { T_max = T; }
		if (T < T_min) { T_min = T; }
		usleep(3000000);
	}
	double	a = (T_max - T_min) / 2;
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "a = %f", a);
	return a;
}

int	qhytransfer_main(int argc, char *argv[]) {
	int	n = 20;
	int	max = 20;
	int	start = 0;
	double	omega_max = 2 * M_PI / 6;
	double	omega_min = omega_max / pow(10., n / 10);

	// parse the command line
	int	c;
	while (EOF != (c = getopt(argc, argv, "dm:n:s:")))
		switch (c) {
		case 'd':
			qhydebuglevel = LOG_DEBUG;
			break;
		case 'm':
			max = atoi(optarg);
			break;
		case 's':
			start = atoi(optarg);
			break;
		case 'n':
			n = atoi(optarg);
			omega_min = omega_max / pow(10., n / 10);
			break;
		}

	// the next argument must be the file were we write the results
	const char	*filename = "transfer.csv";
	if (argc > optind) {
		filename = optarg;
	}

	// open the file for the out
	std::ofstream	out(filename);

        // open a device, just for testing purposes
        device = getDevice(0x1618, 0x6003);

	// compute the transfer function
	double	q = pow(10., 1 / 10.);
	for (int f = start; f <= max; f++) {
		double	omega = omega_min * pow(q, f);
		double	a = transfer(omega);
		out << omega << "," << a << std::endl;
	}
		
	// close the file
	out.close();

	// so we were successful
	return EXIT_SUCCESS;
}

} // namespace qhy

int	main(int argc, char *argv[]) {
	try {
		return qhy::qhytransfer_main(argc, argv);
	} catch (const std::exception& x) {
		std::cerr << "error in qhytransfer: " << x.what() << std::endl;
	}
	return EXIT_FAILURE;
}
