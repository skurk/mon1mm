#define _POSIX_C_SOURCE 200809L
#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

void log_msg(const char *level, const char *fmt, ...)
{
	char tsbuf[32];
	struct tm tmv;
	time_t now = time(NULL);
	FILE *out;
	va_list ap;

	localtime_r(&now, &tmv);
	strftime(tsbuf, sizeof(tsbuf), "%Y-%m-%d %H:%M:%S", &tmv);

	out = (strcmp(level, "INFO") == 0) ? stdout : stderr;

	fprintf(out, "%s [%s] ", tsbuf, level);
	va_start(ap, fmt);
	vfprintf(out, fmt, ap);
	va_end(ap);
	fputc('\n', out);
	fflush(out);
}
