#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <errno.h>
#ifdef _WIN32
#include <stdarg.h>

void libcperciva_warnx(const char* fmt, ...) {
	// Stub: do nothing
}

void syslog(int priority, const char* format, ...) {
	// Stub: do nothing
}
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define LOG_EMERG   0       /* system is unusable */
#define LOG_ALERT   1       /* action must be taken immediately */
#define LOG_CRIT    2       /* critical conditions */
#define LOG_ERR     3       /* error conditions */
#define LOG_WARNING 4       /* warning conditions */
#define LOG_NOTICE  5       /* normal but significant condition */
#define LOG_INFO    6       /* informational */
#define LOG_DEBUG   7       /* debug-level messages */
#endif

#include "warnp.h"

#ifdef _WIN32
#include <stdarg.h>


void closelog(void) {
	// no-op
}
#endif

#ifdef _WIN32
#include <stdio.h>

// Stub implementations, do nothing on Windows
void flockfile(FILE* file) {
	(void)file;  // silence unused parameter warning
}

void funlockfile(FILE* file) {
	(void)file;
}
#endif

static int initialized = 0;
static char * name = NULL;
static int use_syslog = 0;
static int syslog_priority = LOG_WARNING;

/* Free the name string and clean up writing to the syslog (if applicable). */
static void
warnp_atexit(void)
{

	/* Clean up writing to the syslog (if applicable). */
	if (use_syslog)
		closelog();

	free(name);
	name = NULL;
}

/**
 * warnp_setprogname(progname):
 * Set the program name to be used by warn() and warnx() to ${progname}.
 */
void
warnp_setprogname(const char * progname)
{
	const char * p;

	/* Free the name if we already have one. */
	free(name);

	/* Find the last segment of the program name. */
	for (p = progname; progname[0] != '\0'; progname++)
		if (progname[0] == '/')
			p = progname + 1;

	/* Copy the name string. */
	name = strdup(p);

	/* If we haven't already done so, register our exit handler. */
	if (initialized == 0) {
		atexit(warnp_atexit);
		initialized = 1;
	}
}

/* This function will preserve errno. */
void
warn(const char * fmt, ...)
{
	va_list ap;
	char msgbuf[WARNP_SYSLOG_MAX_LINE + 1];
	int saved_errno;

	/* Save errno in case it gets clobbered. */
	saved_errno = errno;

	va_start(ap, fmt);
	if (use_syslog == 0) {
		/* Stop other threads writing to stderr. */
		flockfile(stderr);

		/* Print to stderr. */
		fprintf(stderr, "%s", (name != NULL) ? name : "(unknown)");
		if (fmt != NULL) {
			fprintf(stderr, ": ");
			vfprintf(stderr, fmt, ap);
		}
		fprintf(stderr, ": %s\n", strerror(saved_errno));

		/* Allow other threads to write to stderr. */
		funlockfile(stderr);
	} else {
		/* Print to syslog. */
		if (fmt != NULL) {
			/* No need to print "${name}: "; syslog does it. */
			vsnprintf(msgbuf, WARNP_SYSLOG_MAX_LINE + 1, fmt, ap);
			syslog(syslog_priority, "%s: %s\n", msgbuf,
			    strerror(saved_errno));
		} else
			syslog(syslog_priority, "%s\n", strerror(saved_errno));
	}
	va_end(ap);

	/* Restore saved errno. */
	errno = saved_errno;
}

/**
 * warnp_syslog(enable):
 * Send future messages to syslog if ${enable} is non-zero.  Messages to
 * syslog will be truncated at WARNP_SYSLOG_MAX_LINE characters.
 */
void
warnp_syslog(int enable)
{

	/* Clean up writing to the syslog (if applicable). */
	if (use_syslog && !enable)
		closelog();

	use_syslog = enable;
}

/**
 * warnp_syslog_priority(priority):
 * Tag future syslog messages with priority ${priority}.  Do not enable
 * syslog messages; for that, use warnp_syslog().
 */
void
warnp_syslog_priority(int priority)
{

	syslog_priority = priority;
}
