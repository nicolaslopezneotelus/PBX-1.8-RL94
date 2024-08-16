#include <dirent.h>
#include <pthread.h>
#include <string.h>
#include "logger.h"

char pathname[PATH_MAX];
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

FILE* file_create()
{
	char filename[PATH_MAX];
	time_t meow;
	char buf[64];

	strcpy(filename, pathname);

	meow = time(NULL);

	strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&meow));

	strcat(filename, "/");
	strcat(filename, buf);
	strcat(filename, ".log");

	return fopen(filename, "a+");
}

logger* logger_create(char *logpath)
{
	strcpy(pathname, logpath);

	logger *l = (logger *)malloc(sizeof(logger));
	if (l == NULL) {
		fprintf(stderr, "logger_create(): Could not allocate memory for logger\n");
		return NULL;
	}

	l->datetime_format = (char *)"%Y-%m-%d %H:%M:%S";
	l->level = LOG_INFO;
	l->fp = file_create();

	if (l->fp == NULL) {
		fprintf(stderr, "logger_create(): Could not create file\n");
		return NULL;
	}

	return l;
}

void logger_free(logger *l)
{
	if (l != NULL) {
		fclose(l->fp);
		free(l);
	}
}

void log_add(logger *l, int level, const char *msg)
{
	long size;
	time_t meow;
	char buf[64];

	if (level < l->level) return;

	if (l->fp == NULL) return;

	pthread_mutex_lock(&mut);

	fseek(l->fp, 0, SEEK_END);
	size = ftell(l->fp);
	fseek(l->fp, 0, SEEK_SET);

	if (size > 10 * 1024 * 1024) {
		fclose(l->fp);

		l->fp = file_create();

		if (l->fp == NULL) {
			pthread_mutex_unlock(&mut);
			fprintf(stderr, "logger_create(): Could not create file\n");
			return;
		}
	}

	meow = time(NULL);

	strftime(buf, sizeof(buf), l->datetime_format, localtime(&meow));
	fprintf(l->fp, "[%s][%c][%s]\n",
		buf,
		LOG_LEVEL_CHARS[level],
		msg);
	fflush(l->fp);

	pthread_mutex_unlock(&mut);
}

void log_debug(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_DEBUG, msg);
	va_end(ap);
}

void log_info(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_INFO, msg);
	va_end(ap);
}

void log_warn(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_WARN, msg);
	va_end(ap);
}

void log_error(logger *l, const char *fmt, ...)
{
	va_list ap;
	char msg[LOG_MAX_MSG_LEN];

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	log_add(l, LOG_ERROR, msg);
	va_end(ap);
}
