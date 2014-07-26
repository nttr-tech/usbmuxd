/*
	usbmuxd - iPhone/iPod Touch USB multiplex server daemon

Copyright (C) 2009	Hector Martin "marcan" <hector@marcansoft.com>
Copyright (C) 2009	Nikias Bassen <nikias@gmx.li>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 or version 3.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>

#include "log.h"

unsigned int log_level = LL_WARNING;

int log_syslog = 0;

void log_enable_syslog()
{
	if (!log_syslog) {
		openlog("usbmuxd", LOG_PID, 0);
		log_syslog = 1;
	}
}

void log_disable_syslog()
{
	if (log_syslog) {
		closelog();
	}
}

static int level_to_syslog_level(int level)
{
	int result = level + LOG_CRIT;
	if (result > LOG_DEBUG) {
		result = LOG_DEBUG;
	}
	return result;
}

static void usbmuxd_vlog_raw(enum loglevel level, const char *fmt, va_list ap)
{
	if (log_syslog) {
		vsyslog(level_to_syslog_level(level), fmt, ap);
	} else {
		vfprintf(stderr, fmt, ap);
	}
}

static void usbmuxd_log_raw(enum loglevel level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	usbmuxd_vlog_raw(level, fmt, ap);
	va_end(ap);
}

void usbmuxd_log(enum loglevel level, const char *fmt, ...)
{
	va_list ap;
	char *fs;
	struct timeval ts;
	struct tm *tp;

	if(level > log_level)
		return;

	gettimeofday(&ts, NULL);
	tp = localtime(&ts.tv_sec);

	fs = malloc(20 + strlen(fmt));

	if(log_syslog) {
		sprintf(fs, "[%d] %s\n", level, fmt);
	} else {
		strftime(fs, 10, "[%H:%M:%S", tp);
		sprintf(fs+9, ".%03d][%d] %s\n", (int)(ts.tv_usec / 1000), level, fmt);
	}

	va_start(ap, fmt);
	usbmuxd_vlog_raw(level, fs, ap);
	va_end(ap);

	free(fs);
}

void usbmuxd_log_buffer(enum loglevel level, const unsigned char *data, const size_t length)
{
	if(level > log_level)
		return;

	size_t i;
	int j;
	unsigned char c;

	for (i = 0; i < length; i += 16) {
		usbmuxd_log_raw(level, "%04x: ", i);
		for (j = 0; j < 16; j++) {
			if (i + j >= length) {
				usbmuxd_log_raw(level, "   ");
				continue;
			}
			usbmuxd_log_raw(level, "%02x ", *(data + i + j) & 0xff);
		}
		usbmuxd_log_raw(level, "  | ");
		for (j = 0; j < 16; j++) {
			if (i + j >= length)
				break;
			c = *(data + i + j);
			if ((c < 32) || (c > 127)) {
				usbmuxd_log_raw(level, ".");
				continue;
			}
			usbmuxd_log_raw(level, "%c", c);
		}
		usbmuxd_log_raw(level, "\n");
	}
}