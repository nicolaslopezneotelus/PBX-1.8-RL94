#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
//#include <unistd.h>

#define LOG_DEBUG 0
#define LOG_INFO  1
#define LOG_WARN  2
#define LOG_ERROR 3

#define LOG_LEVEL_CHARS "DIWEF"
#define LOG_MAX_MSG_LEN 1024

typedef struct logger_t {
	int level;
	char *datetime_format;
	FILE *fp;
} logger_t;

logger_t* logger_create(char *logpath);
void logger_free(logger_t *l);
void log_add(logger_t *l, int level, const char *msg);
void log_debug(logger_t *l, const char *fmt, ...);
void log_info(logger_t *l, const char *fmt, ...);
void log_warn(logger_t *l, const char *fmt, ...);
void log_error(logger_t *l, const char *fmt, ...);

#endif