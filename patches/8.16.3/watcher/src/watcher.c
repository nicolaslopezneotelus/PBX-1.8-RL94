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
#include "list.h"
#include "logger.h"
#include "thpool.h"

#define PROC_DIRECTORY "/proc/"
#if HAVE_INOTIFY
#define LIST_FILES_TIMEOUT 300
#else
#define LIST_FILES_TIMEOUT 30
#endif
#define WATCH_FILES_TIMEOUT 60
#define DUMMY_FILENAME "dummy"
#define BUFFER_SIZE 4096

char conffilename[PATH_MAX];
char ramdiskpath[PATH_MAX];
char harddiskpath[PATH_MAX];
char ftphost[128];
int ftpport = 0;
char ftpusername[128];
char ftppassword[128];
char ftphomepath[PATH_MAX];
char ftpuploadpath[PATH_MAX];
int threads = 0;
char loglevel[128];
int keep_running;
list_t *list;
logger_t *logger;
thpool_t *threadpool;

int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int is_numeric(const char* string)
{
	int i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] < '0' || string[i] > '9')
			return -1;

	return 0;
}

int read_config(void)
{
	FILE *file;
	char line[128];

	file = fopen(conffilename, "r");

	if (file != NULL) {
		while (fgets (line, sizeof(line), file) != NULL) {
			if (line[strlen(line) - 1] == '\n' || line[strlen(line) - 1] == '\r')
				line[strlen(line) - 1] = '\0';
			if (strncmp(line, "ramdiskpath=", strlen("ramdiskpath=")) == 0) {
				strcpy(ramdiskpath, line + strlen("ramdiskpath="));
				if (strlen(ramdiskpath) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: ramdiskpath, Error: setting is empty.");
					return -1;
				}
				if (ramdiskpath[strlen(ramdiskpath) - 1] == '/')
					ramdiskpath[strlen(ramdiskpath) - 1] = '\0';
                log_info(logger, "Applying configuration setting succeeded. Setting: ramdiskpath, Value: %s.", ramdiskpath);
			} else if (strncmp(line, "harddiskpath=", strlen("harddiskpath=")) == 0) {
				strcpy(harddiskpath, line + strlen("harddiskpath="));
				if (strlen(harddiskpath) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: harddiskpath, Error: setting is empty.");
					return -1;
				}
				if (harddiskpath[strlen(harddiskpath) - 1] == '/')
					harddiskpath[strlen(harddiskpath) - 1] = '\0';
                log_info(logger, "Applying configuration setting succeeded. Setting: harddiskpath, Value: %s.", harddiskpath);
			} else if (strncmp(line, "ftphost=", strlen("ftphost=")) == 0) {
				strcpy(ftphost, line + strlen("ftphost="));
				if (strlen(ftphost) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: ftphost, Error: setting is empty.");
					return -1;
				}
                log_info(logger, "Applying configuration setting succeeded. Setting: ftphost, Value: %s.", ftphost);
			} else if (strncmp(line, "ftpport=", strlen("ftpport=")) == 0) {
				strcpy(line, line + strlen("ftpport="));
				if (strlen(line) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: ftpport, Error: setting is empty.");
					return -1;
				}
				if (is_numeric(line) != 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: ftpport, Error: setting not valid.");
					return -1;
				}
				ftpport = atoi(line);
				if (ftpport < 1 || ftpport > 65536) {
					log_error(logger, "Parsing configuration setting failed. Setting: ftpport, Error: setting not valid.");
					return -1;
				}
                log_info(logger, "Applying configuration setting succeeded. Setting: ftpport, Value: %d.", ftpport);
			} else if (strncmp(line, "ftpusername=", strlen("ftpusername=")) == 0) {
				strcpy(ftpusername, line + strlen("ftpusername="));
				if (strlen(ftpusername) == 0) {
					log_error(logger, "Couldn't parse file %s: %s.", conffilename, "ftpusername tag is empty");
					return -1;
				}
                log_info(logger, "Applying configuration setting succeeded. Setting: ftpusername, Value: %s.", ftpusername);
			} else if (strncmp(line, "ftppassword=", strlen("ftppassword=")) == 0) {
				strcpy(ftppassword, line + strlen("ftppassword="));
				if (strlen(ftppassword) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: ftppassword, Error: setting is empty.");
					return -1;
				}
                log_info(logger, "Applying configuration setting succeeded. Setting: ftppassword, Value: %s.", ftppassword);
			} else if (strncmp(line, "ftphomepath=", strlen("ftphomepath=")) == 0) {
				strcpy(ftphomepath, line + strlen("ftphomepath="));
				if (strlen(ftphomepath) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: ftphomepath, Error: setting is empty.");
					return -1;
				}
				if (ftphomepath[strlen(ftphomepath) - 1] == '/')
					ftphomepath[strlen(ftphomepath) - 1] = '\0';
                log_info(logger, "Applying configuration setting succeeded. Setting: ftphomepath, Value: %s.", ftphomepath);
			} else if (strncmp(line, "ftpuploadpath=", strlen("ftpuploadpath=")) == 0) {
				strcpy(ftpuploadpath, line + strlen("ftpuploadpath="));
				if (strlen(ftpuploadpath) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: ftpuploadpath, Error: setting is empty.");
					return -1;
				}
				if (ftpuploadpath[strlen(ftpuploadpath) - 1] == '/')
					ftpuploadpath[strlen(ftpuploadpath) - 1] = '\0';
                log_info(logger, "Applying configuration setting succeeded. Setting: ftpuploadpath, Value: %s.", ftpuploadpath);
			} else if (strncmp(line, "threads=", strlen("threads=")) == 0) {
				strcpy(line, line + strlen("threads="));
				if (strlen(line) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: threads, Error: setting is empty.");
					return -1;
				}
				if (is_numeric(line) != 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: threads, Error: setting not valid.");
					return -1;
				}
				threads = atoi(line);
				if (threads < 1 || threads > 100) {
					log_error(logger, "Parsing configuration setting failed. Setting: threads, Error: setting not valid.");
					return -1;
				}
                log_info(logger, "Applying configuration setting succeeded. Setting: threads, Value: %d.", threads);
			} else if (strncmp(line, "loglevel=", strlen("loglevel=")) == 0) {
				strcpy(loglevel, line + strlen("loglevel="));
				if (strlen(loglevel) == 0) {
					log_error(logger, "Parsing configuration setting failed. Setting: loglevel, Error: setting is empty.");
					return -1;
				}
				if (logger != NULL) {
					if (strcmp(loglevel, "debug") == 0)
						logger->level = LOG_DEBUG;
					else if (strcmp(loglevel, "info") == 0)
						logger->level = LOG_INFO;
					else if (strcmp(loglevel, "warn") == 0)
						logger->level = LOG_WARN;
					else if (strcmp(loglevel, "error") == 0)
						logger->level = LOG_ERROR;
				}
                log_info(logger, "Applying configuration setting succeeded. Setting: loglevel, Value: %s.", loglevel);
			}
		}

		fclose(file);

		if (strlen(ramdiskpath) == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: ramdiskpath, Error: setting not found.");
			return -1;
		} else if (strlen(harddiskpath) == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: harddiskpath, Error: setting not found.");
			return -1;
		} else if (strlen(ftphost) == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: ftphost, Error: setting not found.");
			return -1;
		} else if (ftpport == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: ftpport, Error: setting not found.");
			return -1;
		} else if (strlen(ftpusername) == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: ftpusername, Error: setting not found.");
			return -1;
		} else if (strlen(ftppassword) == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: ftppassword, Error: setting not found.");
			return -1;
		} else if (strlen(ftpuploadpath) == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: ftpuploadpath, Error: setting not found.");
			return -1;
		} else if (threads == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: threads, Error: setting not found.");
			return -1;
		} else if (strlen(loglevel) == 0) {
			log_error(logger, "Parsing configuration setting failed. Setting: loglevel, Error: setting not found.");
			return -1;
		}
	} else {
		log_error(logger, "File not found. Filename: %s.", conffilename);
		return -1;
	}

	return 0;
}

int file_is_blocked(char *filename)
{
	DIR *procdirname;
	struct dirent *procentry;
	char fdpath[PATH_MAX];
	DIR *fddirname;
	struct dirent *fdentry;
	char fdfilename[PATH_MAX];
	char _filename[PATH_MAX];

	if ((procdirname = opendir(PROC_DIRECTORY)) == NULL) {
		log_error(logger, "Open directory failed. Directoryname: %s, Error: %s.", PROC_DIRECTORY, strerror(errno));
		return 1;
	}

	while ((procentry = readdir(procdirname)) != NULL) {
		if (procentry->d_type != DT_DIR)
			continue;
		if (is_numeric(procentry->d_name) != 0)
			continue;

		snprintf(fdpath, sizeof fdpath, "%s%s/fd", PROC_DIRECTORY, procentry->d_name);

		if ((fddirname = opendir(fdpath)) == NULL)
			continue;

		while ((fdentry = readdir(fddirname)) != NULL) {
			if (fdentry->d_type != DT_LNK)
				continue;

			snprintf(fdfilename, sizeof fdfilename, "%s/%s", fdpath, fdentry->d_name);

			int len = readlink(fdfilename, _filename, sizeof(_filename) - 1);
			if (len == -1)
				continue;

			_filename[len] = '\0';

			if (strcmp(filename, _filename) == 0) {
				log_debug(logger, "File is blocked. Filename: %s, ProcessId: %s.", filename, procentry->d_name);
				closedir(fddirname);
				closedir(procdirname);
				return 1;
			}
		}

		closedir(fddirname);
	}

	closedir(procdirname);
	
	return 0;
}

int transfer_file(char *filename, char *ftpurl, char *ftpusername, char *ftppassword)
{
	FILE *file;
	CURL *curl;
	CURLcode res;

	file = fopen(filename, "r");

	if (file == NULL) {
		log_error(logger, "Transfer file failed. Filename: %s, Error: %s.", filename, strerror(errno));
		return -1;
	}

	curl = curl_easy_init();

	if (curl == NULL) {
		log_error(logger, "Transfer file failed. Filename: %s, Error: Can not init easy curl.", filename);
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
	//curl_easy_setopt(curl, CURLOPT_LOCALPORTRANGE, 1000L);
	//curl_easy_setopt(curl, CURLOPT_LOCALPORT, 40000L);
	//curl_easy_setopt(curl, CURLOPT_FTPPORT, ":40000-41000");
	curl_easy_setopt(curl, CURLOPT_FTP_USE_EPRT, 0L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	//curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	//curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 10L);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		log_error(logger, "Transfer file failed. Filename: %s, Error: %s.", filename, curl_easy_strerror(res));
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
		log_error(logger, "Move file failed. SourceFilename: %s, DestFilename: %s, Error: %s.", sourcefilename, destfilename, strerror(errno));
		return -1;
	}

	destfile = fopen(destfilename, "wb");

	if (destfile == NULL) {
		log_error(logger, "Move file failed. SourceFilename: %s, DestFilename: %s, Error: %s.", sourcefilename, destfilename, strerror(errno));
		fclose(sourcefile);
		return -1;
	}
	
	char tmp[BUFFER_SIZE];
	size_t size;

	while ((size = fread(tmp, 1, BUFFER_SIZE, sourcefile))) {
		fwrite(tmp, 1, size, destfile);
    }

	fclose(sourcefile);
	fclose(destfile);
	
	if (remove(sourcefilename) != 0)
		log_error(logger, "Delete file failed. Filename: %s, Error: %s.", sourcefilename, strerror(errno));

	return 0;
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

	log_debug(logger, "Process file started. Filename: %s.", _filename);

	// if (list_contains_element(list, _filename) != 0)
	//	return;

	if (file_is_blocked(_filename) != 0) {
		list_remove_element(list, _filename);
		return;
	}

	// list_add_element(list, _filename);

	if (strncmp(_filename, ramdiskpath, strlen(ramdiskpath)) == 0) {
		len = strlen(_filename) - strlen(ramdiskpath) - 1;
		strncpy(ftpfilename, _filename + strlen(ramdiskpath) + 1, len);
	} else if (strncmp(_filename, harddiskpath, strlen(harddiskpath)) == 0) {
		len = strlen(_filename) - strlen(harddiskpath) - 1;
		strncpy(ftpfilename, _filename + strlen(harddiskpath) + 1, len);
	}
	ftpfilename[len] = '\0';

	snprintf(backupfilename, sizeof backupfilename, "%s/%s", harddiskpath, ftpfilename);

	if (strrchr(ftpfilename, '/') != NULL) {
		strcpy(temp_filename2, strrchr(ftpfilename, '/'));
		len = strlen(ftpfilename) - strlen(temp_filename2);
		strncpy(temp_filename1, ftpfilename, len);
		temp_filename1[len] = '\0';
		snprintf(ftpfilename, sizeof ftpfilename, "%s/%s%s", temp_filename1, ftpuploadpath, temp_filename2);
	} else {
		strcpy(temp_filename2, ftpfilename);
		snprintf(ftpfilename, sizeof ftpfilename, "%s/%s", ftpuploadpath, temp_filename2);
	}

	len = strlen(backupfilename) - strlen(strrchr(backupfilename, '/'));
	strncpy(backuppathname, backupfilename, len);
	backuppathname[len] = '\0';

	snprintf(ftpurl, sizeof ftpurl, "ftp://%s:%d/%s/%s", ftphost, ftpport, strlen(ftphomepath) > 0 ? ftphomepath : "", ftpfilename);

	if (transfer_file(_filename, ftpurl, ftpusername, ftppassword) == 0) {
		log_debug(logger, "Transfer file succeeded. Filename: %s.", _filename);
		if (remove(_filename) == 0)
			log_debug(logger, "Delete file succeeded. Filename: %s.", _filename);
		else
			log_error(logger, "Delete file failed. Filename: %s, Error: %s.", _filename, strerror(errno));
	}
	else {
		if (strncmp(_filename, ramdiskpath, strlen(ramdiskpath)) == 0) {
			struct stat st;
			if (!(stat(backuppathname, &st) == 0 && (((st.st_mode) & S_IFMT) == S_IFDIR)))
				mkdir_recursive(backuppathname);
			if (move_file(_filename, backupfilename) == 0)
				log_debug(logger, "Move file succeeded. SourceFilename: %s, DestFilename: %s.", _filename, backupfilename);
		}
	}
	
	list_remove_element(list, _filename);
}

#if HAVE_INOTIFY
void watch_files()
{
	inotifytools_set_printf_timefmt("%F %T");

	// Now watch ramdiskpath
	if (inotifytools_watch_recursively(ramdiskpath, IN_ALL_EVENTS) == 0) {
		log_error(logger, "Watch directory failed. Directoryname: %s, Error: %s.", ramdiskpath, strerror(inotifytools_error()));
		return;
	}

	// Now watch harddiskpath
	if (inotifytools_watch_recursively(harddiskpath, IN_ALL_EVENTS) == 0) {
		log_error(logger, "Watch directory failed. Directoryname: %s, Error: %s.", harddiskpath, strerror(inotifytools_error()));
		return;
	}

	// Now wait till we get event
	struct inotify_event *event;
	char *moved_from = 0;
	char _filename[PATH_MAX];
	char *filename;

	do {
		event = inotifytools_next_event(WATCH_FILES_TIMEOUT);

		if (event == NULL)
			continue;

		//inotifytools_fprintf(stdout, event, "%T %w %e %f \n");

		// if we last had MOVED_FROM and don't currently have MOVED_TO, moved_from file must have been moved outside of tree - so unwatch it.
		if (moved_from && !(event->mask & IN_MOVED_TO)) {
			if (!inotifytools_remove_watch_by_filename(moved_from)) {
				log_error(logger, "Remove watch failed. Directoryname: %s, Error: %s.", moved_from, strerror(inotifytools_error()));
			}
			free(moved_from);
			moved_from = 0;
		}

		if ((event->mask & IN_CREATE) || (!moved_from && (event->mask & IN_MOVED_TO))) {
			// New file - if it is a directory, watch it
			static char *newfilename;
			asprintf(&newfilename, "%s%s", inotifytools_filename_from_wd(event->wd), event->name);
			if (is_regular_file(newfilename) && !inotifytools_watch_recursively(newfilename, IN_ALL_EVENTS)) {
				log_error(logger, "Watch new directory failed. Directoryname: %s, Error %s.", newfilename, strerror(inotifytools_error()));
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

			snprintf(_filename, sizeof _filename, "%s%s", inotifytools_filename_from_wd(event->wd), event->name);

			if (is_regular_file(_filename))
				continue;

			log_debug(logger, "New file watched. Filename: %s.", _filename);

			if (list_contains_element(list, _filename) == 0) {
				filename = (char*) malloc(PATH_MAX);
				strcpy(filename, _filename);
				list_add_element(list, filename);
				thpool_add_work(threadpool, (void*)process_file, (void*)filename);
			}
		}

		fflush(NULL);

	}  while (keep_running);
}

void thread_start(void *arg)
{
	watch_files();

	pthread_exit(0);
}
#endif

void list_files(char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	char newdirname[PATH_MAX];
	char _filename[PATH_MAX];
	char *filename;

	if ((dir = opendir(dirname)) == NULL) {
		log_error(logger, "Open directory failed. Directoryname %s, Error: %s.", dirname, strerror(errno));
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_DIR) {
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;

			snprintf(newdirname, sizeof newdirname, "%s/%s", dirname, entry->d_name);

			list_files(newdirname);
		}
		else if (entry->d_type == DT_REG) {
			if (strcmp(entry->d_name, DUMMY_FILENAME) == 0)
				continue;

			snprintf(_filename, sizeof _filename, "%s/%s", dirname, entry->d_name);

			log_debug(logger, "New file listed. Filename: %s.", _filename);

			if (list_contains_element(list, _filename) == 0) {
				filename = (char*) malloc(PATH_MAX);
				strcpy(filename, _filename);
				list_add_element(list, filename);
				thpool_add_work(threadpool, (void*)process_file, (void*)filename);
			}
		}
	}

	closedir(dir);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stdout, "Usage: %s conffilename logpath\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	strcpy(conffilename, argv[1]);

	list = list_create();

	if (list == NULL)
		exit(EXIT_FAILURE);

	logger = logger_create(argv[2]);

	if (logger == NULL) {
		list_free(list);
		exit(EXIT_FAILURE);
	}

	if (read_config() != 0) {
		logger_free(logger);
		list_free(list);
		exit(EXIT_FAILURE);
	}

	if (daemon(0, 0) < 0) {
		log_error(logger, "Create daemon failed. Error: %s.", strerror(errno));
		logger_free(logger);
		list_free(list);
		exit(EXIT_FAILURE);
	}

	if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
		log_error(logger, "Init curl failed. Error: %s.", strerror(errno));
		logger_free(logger);
		list_free(list);
		exit(EXIT_FAILURE);
	}

	#if HAVE_INOTIFY
	if (inotifytools_initialize() == 0) {
		log_error(logger, "Init inotifytools failed. Error: %s.", strerror(inotifytools_error()));
		curl_global_cleanup();
		logger_free(logger);
		list_free(list);
		exit(EXIT_FAILURE);
	}
	#endif

	threadpool = thpool_init(threads);

	if (threadpool == NULL) {
		log_error(logger, "Init threadpool failed.");
		curl_global_cleanup();
		logger_free(logger);
		list_free(list);
		exit(EXIT_FAILURE);
	}
	
	#if HAVE_INOTIFY
	pthread_t thread;

	if (pthread_create(&thread, NULL, (void *) &thread_start, NULL) != 0) {
		log_error(logger, "Init thread failed. Error: %s.", strerror(errno));
		thpool_destroy(threadpool);
		curl_global_cleanup();
		logger_free(logger);
		list_free(list);
		exit(EXIT_FAILURE);
	}
	#endif

	keep_running = 1;
	log_info(logger, "Service started.");

	while (keep_running) {
		list_files(ramdiskpath);
		list_files(harddiskpath);
		sleep(LIST_FILES_TIMEOUT);
	}

	thpool_destroy(threadpool);
	curl_global_cleanup();
	logger_free(logger);
	list_free(list);

	exit(EXIT_SUCCESS);
}