/*
 * utils.cpp -- auxiliary function implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <utils.h>
#include <qhydebug.h>
#include <sstream>
#include <iomanip>
#include <sys/time.h>

namespace qhy {

/**
 * \brief Write a data buffer to the log
 *
 * This function is used for debugging. It writes a buffer of data
 * in hex format to the debug log
 */
void	logbuffer(const unsigned char *data, int length) {
	for (int offset = 0; offset < length; offset += 16) {
		std::stringstream	out;
		out << std::setw(4) << std::right << std::setfill('0');
		out << std::hex << offset << " ";
		int	m = length - offset;
		if (m > 16) { m = 16; }
		for (int i = 0; i < m; i++) {
			out << " ";
			out << std::setw(2) << std::right << std::setfill('0');
			out << std::hex << (int)data[offset + i];
		}
		qhydebug(LOG_DEBUG, DEBUG_LOG, 0, "%s", out.str().c_str());
	}
}

/**
 * \brief compute double time
 */
double	gettime() {
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	double	result = tv.tv_sec;
	result += 0.0000001 * tv.tv_usec;
	return result;
}

unsigned char	LSB(unsigned short x) {
	unsigned char	result = 0xff & x;
	return result;
}

unsigned char	MSB(unsigned short x) {
	unsigned char	result = (0xff00 & x) >> 8;
	return result;
}

} // namespace qhy
