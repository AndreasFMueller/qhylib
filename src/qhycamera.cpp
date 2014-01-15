/*
 * qhycamera.cpp -- the qhycamera program
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <qhylib.h>
#include <qhydebug.h>
#include <utils.h>

// unchecked headers
#include <fitsio.h>
#include <fitsio2.h>

namespace qhy {

DevicePtr	device;

static void	usage(const char *progname) {
	std::cout << "usage: " << progname;
	std::cout << "%s [ -d ] [ -p cameraid ] [ -b bin ] [ -e seconds ] "
		"fitsfile" << std::endl;
	std::cout << "retrieve an image from a QHYCCD camera and save it "
			"in <fitsfile>" << std::endl;
	std::cout << "options:" << std::endl;
	std::cout << "  -d           increase the debug level" << std::endl;
	std::cout << "  -b bin       binning mode, take a bin x bin "
		"binned image" << std::endl;
	std::cout << "  -e seconds   exposure time in seconds" << std::endl;
	std::cout << "  -f           fast download speed" << std::endl;
	std::cout << "  -p cameraid  set the USB product id of the camera";
	std::cout << std::endl;
	std::cout << "               known cameras:" << std::endl;
	std::cout << "                  QHY8PRO     0x6003" << std::endl;
}

unsigned short	getProduct(const char *s) {
	unsigned short 	result = 0;
	if (strncmp("0x", s, 2) == 0) {
		if (1 != sscanf(s, "%hx", &result)) {
			throw std::runtime_error("cannot parse hex number");
		}
	} else {
		result = atoi(s);
	}
	return result;
}

/**
 * \brief Main function for the qhyccd program
 */
int	qhycamera_main(int argc, char *argv[]) {
	qhydebugthreads = 1;
	qhydebugtimeprecision = 3;
	int	c;
	unsigned short	idProduct = 0x6003; // default QHY8PRO
	int	binning = 1;
	BinningMode	binningmode(binning, binning);
	double	exposuretime = 1;
	enum Camera::DownloadSpeed	speed = Camera::Low;
	while (EOF != (c = getopt(argc, argv, "de:b:p:h?f")))
		switch (c) {
		case 'd':
			qhydebuglevel = LOG_DEBUG;
			break;
		case 'e':
			exposuretime = atof(optarg);
			break;
		case 'b':
			binning = atoi(optarg);
			binningmode = BinningMode(binning, binning);
			break;
		case 'p':
			idProduct = getProduct(optarg);
			break;
		case 'f':
			speed = Camera::High;
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			return EXIT_SUCCESS;
		}

	// next argument must be the file name
	if (optind >= argc) {
		throw std::runtime_error("file name argument missing");
	}
	char	*filename = argv[optind];

	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "qhycamera started");

	// open a device, just for testing purposes
	device = getDevice(0x1618, idProduct);

	// turn of the cooler
	device->dc201().pwm(0);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "temperature: %f",
		device->dc201().temperature());

	double	starttime = gettime();

	// create the camera
	Camera&	camera = device->camera();
	camera.mode(binningmode);
	camera.exposuretime(exposuretime);
	camera.downloadSpeed(speed);
	camera.startExposure();
	ImageBufferPtr	image = camera.getImage();

	double	endtime = gettime();

	long	size = image->size();
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0,
		"image size: %d x %d, size %ld, (%f seconds)",
		image->width(), image->height(), size, endtime - starttime);

	// write the image data to 
	unlink(filename);
	fitsfile	*fits = NULL;
	int	status = 0;
	fits_create_file(&fits, filename, &status);
	qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "fits file %s created\n", filename);
	long	naxes[2] = { image->width(), image->height() };
	fits_create_img(fits, SHORT_IMG, 2, naxes, &status);
	long	fpixel[2] = { 1, 1 };
	fits_write_pix(fits, TUSHORT, fpixel, image->npixels(),
		image->pixelbuffer(), &status);
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
