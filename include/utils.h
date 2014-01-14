/*
 * utils.h -- some utility functions, mainly for development
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _qhy_util_h
#define _qhy_util_h

namespace qhy {

void	logbuffer(const unsigned char *data, int length);
double	gettime();
unsigned char	MSB(unsigned short x);
unsigned char	LSB(unsigned short x);

} // namespace qhy

#endif /* _qhy_util_h */
