/*
 * qhydebug.h -- debug header
 *
 * This header is used in slightly modified form in many different projects
 *
 * (c) 2007-2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _qhydebug_h
#define _qhydebug_h

#include <syslog.h>
#include <stdio.h>
#include <stdarg.h>

#define	DEBUG_NOFILELINE	1
#define DEBUG_ERRNO		2
#define DEBUG_LOG		__FILE__, __LINE__

#ifdef __cplusplus
extern "C" {
#endif

extern int	qhydebuglevel;
extern int	qhydebugtimeprecision;
extern int	qhydebugthreads;
extern void	qhydebug(int loglevel, const char *filename, int line,
			int flags, const char *format, ...);
extern void	qhyvdebug(int loglevel, const char *filename, int line,
			int flags, const char *format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif /* _qhydebug_h */
