/*
 * qhycamera.cpp -- the qhycamera program
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <qhylib.h>
#include <qhydebug.h>
#include <sys/time.h>
#include <fitsio.h>
#include <fitsio2.h>
#include <utils.h>

namespace qhy {

DevicePtr	device;

/**
 * \brief Main function for the qhyccd program
 */
int	qhycamera_main(int argc, char *argv[]) {
	qhydebugthreads = 1;
	qhydebugtimeprecision = 3;
	int	c;
	while (EOF != (c = getopt(argc, argv, "d")))
		switch (c) {
		case 'd':
			qhydebuglevel = LOG_DEBUG;
			break;
		}

	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "qhycamera started");

	// open a device, just for testing purposes
	device = getDevice(0x1618, 0x6003);

	// turn of the cooler
	device->dc201().pwm(0);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "temperature: %f",
		device->dc201().temperature());

	double	starttime = gettime();

	// create the camera
	Camera&	camera = device->camera();
	camera.mode(BinningMode(1, 1));
	camera.exposuretime(0.2);
	camera.startExposure();
	ImageBufferPtr	image = camera.getImage();

	double	endtime = gettime();

	long	size = image->size();
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "image size: %d x %d, size %ld, (%f seconds)",
		image->width(), image->height(), size, endtime - starttime);

	// write the image data to 
	unlink("test.fits");
	fitsfile	*fits = NULL;
	int	status = 0;
	fits_create_file(&fits, "test.fits", &status);
	long	naxes[2] = { image->width(), image->height() };
	fits_create_img(fits, SHORT_IMG, 2, naxes, &status);
	long	fpixel[2] = { 1, 1 };
	fits_write_pix(fits, TUSHORT, fpixel, image->npixels(), image->pixelbuffer(), &status);
	fits_close_file(fits, &status);

	return EXIT_SUCCESS;
}

} // namespace qhy

int	main(int argc, char *argv[]) {
	try {
		return qhy::qhycamera_main(argc, argv);
	} catch (const std::runtime_error& x) {
		std::cerr << "error in qhycamera: " << x.what() << std::endl;
	}
}
