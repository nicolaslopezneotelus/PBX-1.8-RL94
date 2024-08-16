#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_INOTIFY
#include <sys/inotify.h>
#endif
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <curl/curl.h>
#if HAVE_INOTIFY
#include "inotifytools.h"
#include "inotify.h"
#endif
#include "logger.h"
#include "thpool.h"

#define PROC_DIRECTORY "/proc/"
#define PROCESS_FILENAME "/usr/sbin/asterisk"
#if HAVE_INOTIFY
#define MOVE_FILES_DELAY 300
#else
#define MOVE_FILES_DELAY 30
#endif
#define PROCESS_FILES_DELAY 60
#define DUMMY_FILENAME "dummy"
#define BUFFER_SIZE 4096
#define UPLOAD_DIRECTORY "upload"

char conffilename[PATH_MAX];
char logfilename[PATH_MAX];
char ramdiskpath[PATH_MAX];
char harddiskpath[PATH_MAX];
char ramdiskpath[PATH_MAX];
char ftphost[128];
char ftpport[128];
char ftpusername[128];
char ftppassword[128];
int threads;
char loglevel[128];
int keep_running;
logger *l;
thpool_t *threadpool;

struct list_element {
	char value[PATH_MAX];
	struct list_element *next;
};

typedef struct list_element item;

int main(int argc, char **argv);

int read_config(void);

#if HAVE_INOTIFY
void thread_start(void *arg);
#endif

#if HAVE_INOTIFY
void process_files();
#endif

void move_files();

void move_files2(char *dirname, item *current, item *head);

void process_file(char *filename);

int transfer_file(char *filename, char *ftpurl, char *ftpusername, char *ftppassword);

void mkdir_recursive(const char *dirname);

int move_file(const char *to, const char *from);

int is_numeric(const char* string);

int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stdout, "Usage: %s conffilename logfilename\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	strcpy(conffilename, argv[1]);
	strcpy(logfilename, argv[2]);
	
	l = logger_create();
	if (l == NULL)
		exit(EXIT_FAILURE);
	l->fp = fopen(logfilename, "a+");
	if (l->fp == NULL)
		exit(EXIT_FAILURE);
	l->level = LOG_DEBUG;

	if (read_config() != 0) {
		logger_free(l);
		exit(EXIT_FAILURE);
	}

	if (daemon(0, 0) < 0) {
		log_error(l, "Couldn't create daemon: %s.", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
		log_error(l, "Couldn't init curl: %s.", strerror(errno));
		logger_free(l);
		exit(EXIT_FAILURE);
	}

	#if HAVE_INOTIFY
	if (inotifytools_initialize() == 0) {
		log_error(l, "Couldn't init inotifytools: %s.", strerror(inotifytools_error()));
		curl_global_cleanup();
		logger_free(l);
		exit(EXIT_FAILURE);
	}
	#endif

	threadpool = thpool_init(threads);
	if (threadpool == NULL) {
		log_error(l, "Couldn't init threadpool.");
		curl_global_cleanup();
		logger_free(l);
		exit(EXIT_FAILURE);
	}
	
	#if HAVE_INOTIFY
	pthread_t thread;
	if (pthread_create(&thread, NULL, (void *) &thread_start, NULL) != 0) {
		log_error(l, "Couldn't init process_files() thread: %s.", strerror(errno));
		thpool_destroy(threadpool);
		curl_global_cleanup();
		logger_free(l);
		exit(EXIT_FAILURE);
	}
	#endif

	keep_running = 1;
	log_info(l, "Service started.");

	while (keep_running) {
		move_files();

		if (threadpool->jobqueue != NULL && threadpool->jobqueue->jobsN != 0) {
			while (threadpool->jobqueue->jobsN != 0)
				sleep(1);
		}

		sleep(MOVE_FILES_DELAY);

		log_debug(l, "Done waiting %d seconds: move_files().", MOVE_FILES_DELAY);
	}

	thpool_destroy(threadpool);
	curl_global_cleanup();
	logger_free(l);

	exit(EXIT_SUCCESS);
}

int read_config(void)
{
	FILE *file;

	file = fopen(conffilename, "r");

	if (file != NULL) {
		char line[128];
		while (fgets (line, sizeof(line), file) != NULL) {
			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';
			if (line[strlen(line) - 1] == '\r')
				line[strlen(line) - 1] = '\0';
			if (strncmp(line, "ramdiskpath=", strlen("ramdiskpath=")) == 0) {
				strcpy(ramdiskpath, line + strlen("ramdiskpath="));
				if (strlen(ramdiskpath) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "ramdiskpath tag is empty");
					return -1;
				}
				if (ramdiskpath[strlen(ramdiskpath) - 1] == '/')
					ramdiskpath[strlen(ramdiskpath) - 1] = '\0';
			} else if (strncmp(line, "harddiskpath=", strlen("harddiskpath=")) == 0) {
				strcpy(harddiskpath, line + strlen("harddiskpath="));
				if (strlen(harddiskpath) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "harddiskpath tag is empty");
					return -1;
				}
				if (harddiskpath[strlen(harddiskpath) - 1] == '/')
					harddiskpath[strlen(harddiskpath) - 1] = '\0';
			} else if (strncmp(line, "ftphost=", strlen("ftphost=")) == 0) {
				strcpy(ftphost, line + strlen("ftphost="));
				if (strlen(ftphost) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftphost tag is empty");
					return -1;
				}
			} else if (strncmp(line, "ftpport=", strlen("ftpport=")) == 0) {
				strcpy(ftpport, line + strlen("ftpport="));
				if (strlen(ftpport) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftpport tag is empty");
					return -1;
				}
				if (is_numeric(ftpport) != 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftpport tag is not an integer");
					return -1;
				}
				if (atoi(ftpport) < 1 || atoi(ftpport) > 65536) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "threads tag is less than 1 or greater than 65536");
					return -1;
				}
			} else if (strncmp(line, "ftpusername=", strlen("ftpusername=")) == 0) {
				strcpy(ftpusername, line + strlen("ftpusername="));
				if (strlen(ftpusername) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftpusername tag is empty");
					return -1;
				}
			} else if (strncmp(line, "ftppassword=", strlen("ftppassword=")) == 0) {
				strcpy(ftppassword, line + strlen("ftppassword="));
				if (strlen(ftppassword) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftppassword tag is empty");
					return -1;
				}
			} else if (strncmp(line, "threads=", strlen("threads=")) == 0) {
				strcpy(line, line + strlen("threads="));
				if (strlen(line) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "threads tag is empty");
					return -1;
				}
				if (is_numeric(line) != 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "threads tag is not an integer");
					return -1;
				}
				threads = atoi(line);
				if (threads < 1 || threads > 100) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "threads tag is less than 1 or greater than 100");
					return -1;
				}
			} else if (strncmp(line, "loglevel=", strlen("loglevel=")) == 0) {
				strcpy(loglevel, line + strlen("loglevel="));
				if (strlen(loglevel) == 0) {
					log_error(l, "Couldn't parse file %s: %s.", conffilename, "loglevel tag is empty");
					return -1;
				}
				if (l != NULL) {
					if (strcmp(loglevel, "debug") == 0)
						l->level = LOG_DEBUG;
					else if (strcmp(loglevel, "info") == 0)
						l->level = LOG_INFO;
					else if (strcmp(loglevel, "warn") == 0)
						l->level = LOG_WARN;
					else if (strcmp(loglevel, "error") == 0)
						l->level = LOG_ERROR;
				}
			}
		}
		fclose(file);
		if (strlen(ramdiskpath) == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "ramdiskpath tag not found");
			return -1;
		} else if (strlen(harddiskpath) == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "harddiskpath tag not found");
			return -1;
		} else if (strlen(ftphost) == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftphost tag not found");
			return -1;
		} else if (strlen(ftpport) == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftpport tag not found");
			return -1;
		} else if (strlen(ftpusername) == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftpusername tag not found");
			return -1;
		} else if (strlen(ftppassword) == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "ftppassword tag not found");
			return -1;
		} else if (threads == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "threads tag not found");
			return -1;
		} else if (strlen(loglevel) == 0) {
			log_error(l, "Couldn't parse file %s: %s.", conffilename, "threads loglevel not found");
			return -1;
		}
	} else {
		log_error(l, "Couldn't find file %s.", conffilename);
		return -1;
	}
	return 0;
}

#if HAVE_INOTIFY
void thread_start(void *arg)
{
	process_files();

	pthread_exit(0);
}
#endif

#if HAVE_INOTIFY
void process_files()
{
	inotifytools_set_printf_timefmt("%F %T");

	// Now watch ramdiskpath
	if (inotifytools_watch_recursively(ramdiskpath, IN_ALL_EVENTS) == 0) {
		log_error(l, "Couldn't watch directory %s: %s.", ramdiskpath, strerror(inotifytools_error()));
		return;
	}

	// Now watch harddiskpath
	if (inotifytools_watch_recursively(harddiskpath, IN_ALL_EVENTS) == 0) {
		log_error(l, "Couldn't watch directory %s: %s.", harddiskpath, strerror(inotifytools_error()));
		return;
	}

	// Now wait till we get event
	struct inotify_event *event;
	char *moved_from = 0;
	char *filename;

	do {
		event = inotifytools_next_event(PROCESS_FILES_DELAY);

		if (event == NULL) {
			log_debug(l, "Done waiting %d seconds: process_files().", PROCESS_FILES_DELAY);
			continue;
		}

		//inotifytools_fprintf(stdout, event, "%T %w %e %f \n");

		// if we last had MOVED_FROM and don't currently have MOVED_TO,
		// moved_from file must have been moved outside of tree - so unwatch it.
		if (moved_from && !(event->mask & IN_MOVED_TO)) {
			if (!inotifytools_remove_watch_by_filename(moved_from)) {
				log_error(l, "Couldn't remove watch on %s: %s.", moved_from, strerror(inotifytools_error()));
			}
			free(moved_from);
			moved_from = 0;
		}

		if ((event->mask & IN_CREATE) || (!moved_from && (event->mask & IN_MOVED_TO))) {
			// New file - if it is a directory, watch it
			static char *newfilename;
			asprintf(&newfilename, "%s%s", inotifytools_filename_from_wd(event->wd), event->name);
			if (is_regular_file(newfilename) && !inotifytools_watch_recursively(newfilename, IN_ALL_EVENTS)) {
				log_error(l, "Couldn't watch new directory %s: %s.", newfilename, strerror(inotifytools_error()));
			}
			free(newfilename);
		}
		else if (event->mask & IN_MOVED_FROM) {
			asprintf(&moved_from, "%s%s/", inotifytools_filename_from_wd(event->wd), event->name);
			// if not watched...
			if (inotifytools_wd_from_filename(moved_from) == -1) {
				free(moved_from);
				moved_from = 0;
			}
		}
		else if (event->mask & IN_MOVED_TO) {
			if (moved_from) {
				static char *newfilename;
				asprintf(&newfilename, "%s%s/", inotifytools_filename_from_wd(event->wd), event->name);
				inotifytools_replace_filename(moved_from, newfilename);
				free(moved_from);
				moved_from = 0;
			}
		}

		if (event->mask & IN_CLOSE_WRITE) {
			if (strcmp(event->name, DUMMY_FILENAME) == 0)
				continue;
			filename = (char*) malloc(1024);
			strcpy(filename, inotifytools_filename_from_wd(event->wd));
			strcat(filename, event->name);
			if (!is_regular_file(filename))
				thpool_add_work(threadpool, (void*)process_file, (void*)filename);
			else
				free(filename);
		}

		fflush(NULL);

	}  while (keep_running);

	return 0;
}
#endif

void move_files()
{
	DIR *procdirname;
	struct dirent *procentry;
	char cmdpath[PATH_MAX];
	FILE *file;
	char cmdline[1024];
	char fdpath[PATH_MAX];
	DIR *fddirname;
	struct dirent *fdentry;
	char fdfilename[PATH_MAX];
	char filename[PATH_MAX];

	item *current, *head, *tmp;
	head = NULL;

	if ((procdirname = opendir(PROC_DIRECTORY)) == NULL) {
		log_error(l, "Couldn't open directory %s: %s.", PROC_DIRECTORY, strerror(errno));
		return;
	}

	while ((procentry = readdir(procdirname)) != NULL) {
		if (procentry->d_type != DT_DIR)
			continue;
		if (is_numeric(procentry->d_name) != 0)
			continue;

		strcpy(cmdpath, PROC_DIRECTORY);
		strcat(cmdpath, procentry->d_name);
		strcat(cmdpath, "/cmdline");

		file = fopen(cmdpath, "rt");

		if (file == NULL)
			continue;

		fscanf(file, "%s", cmdline);
		fclose(file);

		if (strncmp(cmdline, PROCESS_FILENAME, strlen(PROCESS_FILENAME)) != 0)
			continue;

		strcpy(fdpath, PROC_DIRECTORY);
		strcat(fdpath, procentry->d_name);
		strcat(fdpath, "/fd");

		if ((fddirname = opendir(fdpath)) == NULL) {
			log_error(l, "Couldn't open directory %s: %s.", fdpath, strerror(errno));
			continue;
		}

		while ((fdentry = readdir(fddirname)) != NULL) {
			if (fdentry->d_type != DT_LNK)
				continue;

			strcpy(fdfilename, fdpath);
			strcat(fdfilename, "/");
			strcat(fdfilename, fdentry->d_name);

			int len = readlink(fdfilename, filename, sizeof(filename) - 1);
			if (len == -1)
				continue;

			filename[len] = '\0';
			current = (item *) malloc(sizeof(item));
			strcpy(current->value, filename);
			current->next = head;
			head = current;
		}

		closedir(fddirname);
	}

	closedir(procdirname);

	move_files2(ramdiskpath, current, head);

	move_files2(harddiskpath, current, head);

	current = head;
	while (current) {
		tmp = current->next;
		free(current);
		current = tmp;
	}
}

void move_files2(char *dirname, item *current, item *head)
{
	DIR *dir;
	struct dirent *entry;
	char newdirname[PATH_MAX];
	char *filename;
	int blocked;

	if ((dir = opendir(dirname)) == NULL) {
		log_error(l, "Couldn't open directory %s: %s.", dirname, strerror(errno));
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_DIR) {
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;

			strcpy(newdirname, dirname);
			strcat(newdirname, "/");
			strcat(newdirname, entry->d_name);

			move_files2(newdirname, current, head);
		}
		else if (entry->d_type == DT_REG) {
			filename = (char*) malloc(PATH_MAX);
			strcpy(filename, dirname);
			strcat(filename, "/");
			strcat(filename, entry->d_name);

			blocked = 0;
			current = head;
			while (current) {
				if (strcmp(current->value, filename) == 0) {
					blocked = 1;
					break;
				}
				current = current->next;
			}

			if (blocked == 0)
				thpool_add_work(threadpool, (void*)process_file, (void*)filename);
			else {
				log_warn(l, "Couldn't access blocked file %s.", filename);
				free(filename);
			}
		}
	}

	closedir(dir);
}

void process_file(char *filename)
{
	char _filename[PATH_MAX];
	char ftpfilename[PATH_MAX];
	char backupfilename[PATH_MAX];
	char temp_filename1[PATH_MAX];
	char temp_filename2[PATH_MAX];
	char backuppathname[PATH_MAX];
	char ftpurl[1024];
	int len = 0;

	strcpy(_filename, filename);
	free(filename);

	if (strncmp(_filename, ramdiskpath, strlen(ramdiskpath)) == 0) {
		len = strlen(_filename) - strlen(ramdiskpath) - 1;
		strncpy(ftpfilename, _filename + strlen(ramdiskpath) + 1, len);
	} else if (strncmp(_filename, harddiskpath, strlen(harddiskpath)) == 0) {
		len = strlen(_filename) - strlen(harddiskpath) - 1;
		strncpy(ftpfilename, _filename + strlen(harddiskpath) + 1, len);
	}
	ftpfilename[len] = '\0';

	strcpy(backupfilename, harddiskpath);
	strcat(backupfilename, "/");
	strcat(backupfilename, ftpfilename);

	if (strrchr(ftpfilename, '/') != NULL) {
		strcpy(temp_filename2, strrchr(ftpfilename, '/'));
		len = strlen(ftpfilename) - strlen(temp_filename2);
		strncpy(temp_filename1, ftpfilename, len);
		temp_filename1[len] = '\0';
		strcpy(ftpfilename, temp_filename1);
		strcat(ftpfilename, "/");
		strcat(ftpfilename, UPLOAD_DIRECTORY);
		strcat(ftpfilename, temp_filename2);
	} else {
		strcpy(temp_filename2, ftpfilename);
		strcpy(ftpfilename, UPLOAD_DIRECTORY);
		strcat(ftpfilename, "/");
		strcat(ftpfilename, temp_filename2);
	}

	len = strlen(backupfilename) - strlen(strrchr(backupfilename, '/'));
	strncpy(backuppathname, backupfilename, len);
	backuppathname[len] = '\0';

	strcpy(ftpurl, "ftp://");
	strcat(ftpurl, ftphost);
	strcat(ftpurl, ":");
	strcat(ftpurl, ftpport);
	strcat(ftpurl, "/");
	strcat(ftpurl, ftpfilename);

	if (transfer_file(_filename, ftpurl, ftpusername, ftppassword) == 0) {
		log_info(l, "Done transferring file: %s.", _filename);
		if (remove(_filename) != 0)
			log_error(l, "Couldn't remove file %s: %s.", _filename, strerror(errno));
	}
	else {
		if (strncmp(_filename, ramdiskpath, strlen(ramdiskpath)) == 0) {
			struct stat st;
			if (!(stat(backuppathname, &st) == 0 && (((st.st_mode) & S_IFMT) == S_IFDIR)))
				mkdir_recursive(backuppathname);
			if (move_file(_filename, backupfilename) == 0)
				log_info(l, "Done moving file: %s.", _filename);
			else
				log_error(l, "Couldn't move file %s: %s.", _filename, strerror(errno));
		}
	}
}

int transfer_file(char *filename, char *ftpurl, char *ftpusername, char *ftppassword)
{
	FILE *file;
	CURL *curl;
	CURLcode res;

	file = fopen(filename, "r");
	if (file == NULL) {
		log_error(l, "Couldn't open file %s: %s.", filename, strerror(errno));
		return -1;
	}

	curl = curl_easy_init();
	if (curl == NULL) {
		log_error(l, "Couldn't init easy curl.");
		fclose(file);
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_USERNAME, ftpusername);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, ftppassword);
	curl_easy_setopt(curl, CURLOPT_URL, ftpurl);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
	curl_easy_setopt(curl, CURLOPT_READDATA, file);
	curl_easy_setopt(curl, CURLOPT_FTPPORT, "-");
	curl_easy_setopt(curl, CURLOPT_FTP_USE_EPRT, 0L);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	//curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	//curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 10L);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		log_error(l, "Couldn't transfer file %s: %s %d.", filename, curl_easy_strerror(res), (unsigned int) pthread_self());
		fclose(file);
		curl_easy_cleanup(curl);
		return -1;
	}

	fclose(file);
	curl_easy_cleanup(curl);

	return 0;
}

void mkdir_recursive(const char *path)
{
	char tmp[1024];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);
	if (tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for (p = tmp + 1; *p; p++)
		if (*p == '/') {
			*p = 0;
			mkdir(tmp, S_IRWXU);
			*p = '/';
		}
	mkdir(tmp, S_IRWXU);
}

int move_file(const char *sourcefilename, const char *destfilename)
{
	FILE *sourcefile, *destfile;

	sourcefile = fopen(sourcefilename, "rb");
	if (sourcefile == NULL) {
		log_error(l, "Couldn't open file %s: %s.", sourcefilename, strerror(errno));
		return -1;
	}

	destfile = fopen(destfilename, "wb");
	if (destfile == NULL) {
		log_error(l, "Couldn't open file %s: %s.", destfilename, strerror(errno));
		fclose(sourcefile);
		return -1;
	}
	
	char tmp[BUFFER_SIZE];
	size_t size;

	while (size = fread(tmp, 1, BUFFER_SIZE, sourcefile)) {
		fwrite(tmp, 1, size, destfile);
    }

	fclose(sourcefile);
	fclose(destfile);
	
	if (remove(sourcefilename) != 0)
		log_error(l, "Couldn't remove file %s: %s.", sourcefilename, strerror(errno));

	return 0;
}

int is_numeric(const char* string)
{
	int i;
	for (i = 0; i < strlen(string); i++)
		if (string[i] < '0' || string[i] > '9')
			return -1;
	return 0;
}
