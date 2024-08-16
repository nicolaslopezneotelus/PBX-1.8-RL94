/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) <2011>, <Verbio Technologies>
 *
 * Verbio Technologies <support@verbio.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief Verbio Technologies Speech Applications
 *
 * \author Verbio Technologies S.L <support@verbio.com>
 *
 * \ingroup applications
 */

#include "asterisk.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h> /*toupper*/
#include <errno.h>
#include "sys/stat.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 38 $")

#include "asterisk/file.h"
#include "asterisk/logger.h"
#include "asterisk/channel.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"
#include "asterisk/lock.h"
#include "asterisk/app.h"
#include "asterisk/version.h"

/* Verbio voxlib */
#include <voxlib.h>

static const int VERBIO_BUFFER_SIZE = 8000;
static const int VERBIO_MAXINDEX = 320;
static const int VERBIO_MAX_RES_SIZE = 100;
static const int VERBIO_MAX_CVAR_SIZE = 120;
static const int VERBIO_MAX_INT_SIZE = 10;
static const int VERBIO_MAX_TSTR_SIZE = 10;
static const char *VERBIO_SERVER = "127.0.0.1";
//ADDED: 2017-03-20
static const int VERBIO_PORT = 8765;
//ADDED: 2017-03-20
static const int VERBIO_NET_TIMEOUT = 5;
static const char *VERBIO_GRAMMAR_PATH = "/var/spool/asterisk/tmp";
static const int VERBIO_INIT_SIL = 300;
static const int VERBIO_MAX_SIL = 200;
static const int VERBIO_ABS_TIMEOUT = 30;
static const float VERBIO_MAX_REF = 200.0;
static const float VERBIO_MIN_REF = 5.0;
static const char *VERBIO_TEXT_PROMPT_PATH = "/var/spool/asterisk/tmp";
static const float VERBIO_INIT_DELAY = 650;
static const float VERBIO_END_DELAY = 80;
static const float VERBIO_LOW_FACTOR = 5.0;
static const float VERBIO_HIGH_FACTOR = 9.0;
static const float VERBIO_FINAL_FACTOR = 1.6;
static const float VERBIO_FINAL_HIGH_FACTOR = 5.0;
static const float VERBIO_MIN_HIGH_THRESH = 500.0;
static const float VERBIO_AAM_MIN = 100.0;
static const float VERBIO_AAM_MAX = 400.0;
static const char *VERBIO_CFG = "verbio.conf";
static const int MAX_WAIT_TIME = 1000; /*ms*/
static const char *VERBIO_GRM_CACHE_DIR	= "/cache/";

#define	VERBIO_DIR_MODE 0777
#define	VERBIO_FILE_MODE 0644

/* MC_ALAW, MC_MULAW, MC_LIN16 */
/* AST_FORMAT_ALAW, AST_FORMAT_ULAW, AST_FORMAT_SLINEAR */
#define	VERBIO_CODEC MC_ALAW
#define AUDIO_FORMAT AST_FORMAT_ALAW

static const char *EXT = "alaw";

#define VERBIO_DEF_REC_PATH	"/var/spool/asterisk/tmp"
#define args_sep ','
#define str_sep ","

typedef const char* DATA_TYPE;	

/* If we detect a hangup, close current verbio device*/
#define VERBIO_CLOSE_ON_HANGUP

//ADDED: 2019-04-02
static unsigned int channel_handle_idx;
//ADDED: 2019-04-02

/* 
* Application info
*/
/* VerbioPrompt */
static char *verbio_prompt_descrip =
"VerbioPrompt(text_or_file[|language][|speaker][|options])\n"
"Synthetise a text using Verbio TTS engine.\n"
"- text_or_file : text or file (see options) to synth\n"
"- language     : TTS language\n"
"- speaker      : TTS speaker\n"
"- options      : a (answer channel)\n"
"                 b (beep before prompt)\n"
"                 d (enable dtmf detection)\n"
"                 f (treat text_or_file parameter as a text file to read from)\n"
"                 n (do not hangup on Verbio error)\n\n"
"If dtmf detection ('d' option) is enabled, the following channel vars will be set:\n"
" - VDTMF_DETECTED (TRUE or FALSE)\n"
" - VDTMF_RESULT (if VDTMF_DETECTED = TRUE, will contain the pressed key -0,1,2,3,4,5,6,7,8,9,*,#...-)\n"
"\n You are also allowed to set DTMF max length and DTMF terminator via the following dialplan variables:\n"
" - VERBIO_DTMF_MAXLEN\n"
" - VERBIO_DTMF_TERMINATOR\n\n";
static char *verbio_prompt_app = "VerbioPrompt";

/* VerbioRec */
static char *verbio_rec_descrip =
"VerbioRec([|config][|language][|init_sil][|max_sil][|abs_timeout][|options])\n"
"Launch a recognition.\n"
"- config      : ASR configuration\n"
"- language    : ASR language\n"
"- init_sil    : Initial silence (max. duration of initial silence in 10ms units)\n"
"- max_sil     : Max. silence (max. duration of final silence in 10ms units)\n"
"- abs_timeout : Absolute timeout (seconds)\n"
"- options     : a (answer channel)\n"
"                b (beep before recognition)\n"
"                d (enable dtmf detection)\n"
"                n (do not hangup on Verbio error)\n\n"
"You /must/ execute VerbioLoadVcb app prior to launch any recognition.\n\n"
"In order to check the recognition result, you need to check (once VerbioRec has finished) \n"
"the following channel vars:\n"
" - VASR_WORDS      : Number of recognized words (n).\n"
" - VASR_INDEXn     : Index (in grammar) of the n-word recognized.\n"
" - VASR_RESULTn    : n-Result of recognition.\n"
" - VASR_SCOREn     : n-score of recognition.\n"
" - VASR_UTTERANCEn : n-utterance.\n"
" - VASR_WEIGHTn    : n-weight\n"
" - VASR_RULEn      : n-rule \n\n"
"For backward compatibility:\n"
" - VASR_INDEX = VASR_INDEX0\n"
" - VASR_RESULT = VASR_RESULT0\n"
" - VASR_SCORE = VASR_SCORE0\n"
" - VASR_UTTERANCE = VASR_UTTERANCE0\n"
" - VASR_WEIGHT = VASR_WEIGHT0\n"
" - VASR_RULE = VASR_RULE0\n\n"
"If dtmf detection ('d' option) is enabled, the following channel vars will be set:\n"
" - VDTMF_DETECTED (TRUE or FALSE)\n"
" - VDTMF_RESULT (if VDTMF_DETECTED = TRUE, will contain the pressed key -0,1,2,3,4,5,6,7,8,9,*,#...-)\n"
"\n You are also allowed to set DTMF max length and DTMF terminator via the following dialplan variables:\n"
" - VERBIO_DTMF_MAXLEN\n"
" - VERBIO_DTMF_TERMINATOR\n\n"
"Note: If 'keep_recorded_files' is enabled in 'verbio.conf', you can check the recorded file name via\n"
"      VASR_REC_FILE dialplan variable (once the recognition application has finished).\n";
static char *verbio_rec_app = "VerbioRec";

/* VerbioStreamAndRec */
static char *verbio_stream_and_rec_descrip =
"VerbioStreamAndRec(audio_file[|init_sil][|max_sil][|config][|language][|abs_timeout][|options])\n"
"VerbioStreamAndRec.\n"
"- audio_file  : Audio file to stream.\n"
"- init_sil    : Initial silence (max. duration of initial silence in 10ms units)\n"
"- max_sil     : Max. silence (max. duration of final silence in 10ms units)\n"
"- config      : ASR configuration\n"
"- language    : ASR language\n"
"- abs_timeout : Absolute timeout (seconds)\n"
"- options     : a (answer channel)\n"
"                b (beep before rec -no sense when using bargein-)\n"
"                d (enable dtmf detection)\n"
"                g (bargein activated, user can interrupt the system -will disable beep-)\n"
"                i (immediately stop stream on voice detection -will enable bargein-)\n"
"                n (do not hangup on Verbio error)\n\n"
"Please, when using bargein, be very carefull with initsil and maxsil parameters. If you do not\n"
"properly set them you may run into problems like recognition stopping\n"
"(prior to finish) our audio file (if user does not speak). \n"
"initsil must be high enough to allow the user to listen our (full) audio file.\n\n"
"Also remember that you /must/ execute VerbioLoadVcb app prior to launch a recognition.\n\n"
"In order to check the recognition result, you need to check (once VerbioPromptAndRec has finished) \n"
"the following channel vars:\n"
" - VASR_WORDS      : Number of recognized words (n).\n"
" - VASR_INDEXn     : Index (in grammar) of the n-word recognized.\n"
" - VASR_RESULTn    : n-Result of recognition.\n"
" - VASR_SCOREn     : n-score of recognition.\n"
" - VASR_UTTERANCEn : n-utterance.\n"
" - VASR_WEIGHTn    : n-weight\n"
" - VASR_RULEn      : n-rule \n\n"
"For backward compatibility:\n"
" - VASR_INDEX = VASR_INDEX0\n"
" - VASR_RESULT = VASR_RESULT0\n"
" - VASR_SCORE = VASR_SCORE0\n"
" - VASR_UTTERANCE = VASR_UTTERANCE0\n"
" - VASR_WEIGHT = VASR_WEIGHT0\n"
" - VASR_RULE = VASR_RULE0\n\n"
"If dtmf detection ('d' option) is enabled, the following channel vars will be set:\n"
" - VDTMF_DETECTED (TRUE or FALSE)\n"
" - VDTMF_RESULT (if VDTMF_DETECTED = TRUE, will contain the pressed key -0,1,2,3,4,5,6,7,8,9,*,#...-)\n"
"\n You are also allowed to set DTMF max length and DTMF terminator via the following dialplan variables:\n"
" - VERBIO_DTMF_MAXLEN\n"
" - VERBIO_DTMF_TERMINATOR\n\n"
"Note: If 'keep_recorded_files' is enabled in 'verbio.conf', you can check the recorded file name via\n"
"      VASR_REC_FILE dialplan variable (once the recognition application has finished).\n";
static char *verbio_stream_and_rec_app = "VerbioStreamAndRec";

/* VerbioLoadVcb */
static char *verbio_load_vcb_descrip =
"VerbioLoadVcb(gram_file|gram_type[|config][|language][|options])\n"
"Load vocabulary.\n"
"- gram_file : grammar file\n"
"- gram_type : grammar type (ISOLATED, CONNECTED, ABNF, BUILTIN or VVI)\n"
"- config    : ASR config\n"
"- language  : ASR language\n"
"- options   : n (do not hangup on Verbio error)\n\n"
"This function will set a channel var (VERBIO_VOCABULARY_HANDLE) containing the id (vcb_handle)\n"
"of the loaded vocabulary (see VerbioUnloadVcb to see why we need this handle).\n";
static char *verbio_load_vcb_app = "VerbioLoadVcb";

/* VerbioUnloadVcb */
static char *verbio_unload_vcb_descrip =
"VerbioUnloadVcb(vocabulary_handle[|config][|language][|options])\n"
"Unload vocabulary.\n"
"- vocabulary_handle : vocabulary id to unload (-1 to unload all, and free licences)\n"
"- config            : ASR config \n"
"- language          : ASR language\n"
"- options           : n (do not hangup on Verbio error)\n\n"
"Please do not forget to execute this app on hangup...otherwise you may run out of licences.\n";
static char *verbio_unload_vcb_app = "VerbioUnloadVcb";

/* VerbioLastErr */
static char *verbio_last_err_descrip =
"VerbioLastErr(var)\n"
"Get Verbio last error.\n"
" - var : channel var where to store last error message.\n\n"
"You need to call Verbio apps using the 'n' option (in order to\n"
"disable hangup on app error).\n\n"
"Error codes:\n"
"EVX_NOERROR   : NO ERROR \n"
"EVX_INVSETUP  : Vox ERROR (Files may be corrupted. Check disk and repeat Vox Setup) \n"
"EVX_NOMEM     : OUT OF MEMORY. (Check memory leakages) \n"
"EVX_VCBFILE   : THE VOCABULARY FILE NAME IS NOT VALID. (Check the vocabulary file name and path writing permission) \n"
"EVX_INVWORD   : THE VOCABULARY CONTAINS AN INVALID WORD. (Check and correct invalid words) \n"
"EVX_NOLICFILE : NO LICENSE FILE WAS FOUND. (Use Setup and CheckOut to obtain the Vox directory structure and the license file) \n"
"EVX_INVLIC    : THE LICENSE FILE IS NOT VALID. (Use CheckOut to obtain a valid license file) \n"
"EVX_SYSTEM	   : SYSTEM ERROR (Check errno) \n"
"EVX_NOLIBINIT : VOXLIB WAS NOT SUCCESSFULLY LOADED. (Call vox_libinit() before using any Vox function) \n"
"EVX_NOLIC     : NO LICENSE \n"
"EVX_NOSETVCB  : NO ACTIVE VOCABULARY. (Use vox_setvcb() to set the active vocabulary) \n"
"EVX_NORECSTR  : NO RECOGNITION. (Use vox_recstr() to init recognition) \n"
"EVX_NOLINE    : NO MORE LINES ARE AVAILABLE FOR THE SPECIFIED CHANNEL DEVICE \n"
"EVX_BADPARM   : INVALID PARAMETER IN FUNCTION CALL \n"
"EVX_NOTIMP    : NOT IMPLEMENTED \n"
"EVX_NORECIND  : NO RECIND OR NBEST. (Call vox_recind() before calling ATVOX_NIND()) \n"
"EVX_INVFILE   : INVALID FILENAME \n"
"EVX_NETWORK   : NETWORK ERROR \n"
"EVX_DICFILE   : THE DICTIONARY FILE NAME IS NOT VALID \n"
"EVX_PARSER    : ABNF PARSER ERROR \n"
"EVX_INVVER    : THE VOXSERVER VERSION DOES NOT MATCH THE CLIENT VERSION \n"
"EVX_UNKNOWN   : Unknown error\n";
static char *verbio_last_err_app = "VerbioLastErr";

/* VerbioFreeChannel */
static char *verbio_free_channel_descrip =
"VerbioFreeChannel()\n"
"Free Verbio resources (memory and licences).\n"
" Please do not forget to run this application at the end of every call.\n";
static char *verbio_free_channel_app = "VerbioFreeChannel";

static int voxserver_is_down = 0;
static int trying_to_reconnect = 0;
static int unloading_module = 0;

ast_mutex_t voxdownlock;
ast_mutex_t reconnectlock;
ast_mutex_t unloadingmodlock;

/* Helpler functions */
static void close_verbio(void);
static int init_verbio(void);

/*! \brief Helper function. Copy file */
static int verbio_copy_file(char *infile, char *outfile)
{
	int ifd;
	int ofd;
	int res;
	int len;
	char buf[4096];

	if ((ifd = open(infile, O_RDONLY)) < 0) {
		ast_log(LOG_WARNING, "Unable to open %s in read-only mode: %s\n", infile, strerror(errno));
		return -1;
	}
	if ((ofd = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, VERBIO_FILE_MODE)) < 0) {
		ast_log(LOG_WARNING, "Unable to open %s in write-only mode: %s\n", outfile, strerror(errno));
		close(ifd);
		return -1;
	}
	do {
		len = read(ifd, buf, sizeof(buf));
		if (len < 0) {
			ast_log(LOG_WARNING, "Read failed on %s: %s\n", infile, strerror(errno));
			close(ifd);
			close(ofd);
			unlink(outfile);
		}
		if (len) {
			res = write(ofd, buf, len);
			if (errno == ENOMEM || errno == ENOSPC || res != len) {
				ast_log(LOG_WARNING, "Write failed on %s (%d of %d): %s\n", outfile, res, len, strerror(errno));
				close(ifd);
				close(ofd);
				unlink(outfile);
			}
		}
	} while (len);
	close(ifd);
	close(ofd);
	return 0;
}

/*! \brief Load config */
static struct ast_config* verbio_load_asterisk_config(void)
{
	struct ast_flags config_flags = { CONFIG_FLAG_WITHCOMMENTS };
	return ast_config_load(VERBIO_CFG, config_flags);
}

/*! \brief Helper function. Check grammar existance in cache */
static int verbio_md5_grammar_exists(char *gram_full_path, char *md5_gram_full_path, const char *lang, int verbosity)
{
	struct ast_config *cfg = NULL;	
	const char *cfg_grammar_path = NULL;

	cfg = verbio_load_asterisk_config();
	
	if (!cfg) {
		ast_log(LOG_ERROR, "Error opening configuration file %s\n", VERBIO_CFG);
		return -1;
	}

	if (!(cfg_grammar_path = ast_variable_retrieve(cfg, "asr", "grammar_path"))) {
		ast_log( LOG_ERROR, "Could not read grammar_path option. Please set it up\n");
		ast_config_destroy(cfg);
		return -1;
	}

	char md5gram[33] = "";
	FILE *fp;
	long len;
	char *buf;
	size_t rres;
	char md5gram_path[MAXPATHLEN];

	strcpy(md5_gram_full_path, "");

	fp = fopen(gram_full_path, "rb");
	if (!fp) {
		ast_config_destroy(cfg);
		return -1;
	}
	
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);
	buf = (char*) malloc(sizeof(char) * (len + 1));
	if (buf == NULL) {
		ast_log(LOG_ERROR, "Error allocating buffer\n");
		ast_config_destroy(cfg);
		return -1;
	}

	rres = fread(buf, 1, len, fp);
	if (rres != len) {
		ast_log(LOG_ERROR, "Error reading grammar file to buffer\n");
		if (buf)
			free(buf);

		ast_config_destroy(cfg);
		return -1;
	}

	if (fp)
		fclose(fp);

	buf[len] = '\0';
	ast_md5_hash(md5gram, buf);
	md5gram[32] = '\0';

	if (buf)
		free(buf);

	strcpy(md5gram_path, cfg_grammar_path);
	strcat(md5gram_path, VERBIO_GRM_CACHE_DIR);
	struct stat st;
	if (stat(md5gram_path, &st) != 0) {
		/* Cache dir does not exists */
		if (mkdir(md5gram_path, VERBIO_DIR_MODE) && errno != EEXIST) {
			ast_log(LOG_WARNING, "mkdir %s failed: %s\n", md5gram_path, strerror(errno));
			ast_config_destroy(cfg);
			return -1;
		}
	}

	strcat(md5gram_path, md5gram);
	strcat(md5gram_path, "_");
	strcat(md5gram_path, lang);
	
	/* Check if file exists in cfg_grammar_path */
	if (access(md5gram_path, F_OK) != -1) {
   		/* file exists */
		strcpy(md5_gram_full_path, md5gram_path);
		if (verbosity > 0)
			ast_log(LOG_NOTICE, "Grammar %s exists in cache %s\n", gram_full_path, md5_gram_full_path);
		ast_config_destroy(cfg);
		return 1; 
	} else {
		/* Create cache file */
		verbio_copy_file(gram_full_path, md5gram_path);
		strcpy(md5_gram_full_path, md5gram_path);
	}

	ast_config_destroy(cfg);
	return 0;
}

/*! \brief Helper function called on connection lost */
static void lost_conn_callback(const char *server)
{
	struct ast_config *cfg;
	int auto_reconnect = 0;
	
	ast_mutex_lock(&voxdownlock);
	voxserver_is_down = 1;
	ast_mutex_unlock(&voxdownlock);
	
	ast_log(LOG_ERROR, "Connection with server %s lost!\n", server);
	
	cfg = verbio_load_asterisk_config();
	
	if (cfg) {
		if (ast_variable_retrieve(cfg, "general", "auto_reconnect"))
			auto_reconnect = 1;
		
		ast_config_destroy(cfg);
	}
	
	if (!auto_reconnect)
		return;
	
	ast_log(LOG_NOTICE, "auto_reconnect enabled in config file. Trying to reestablish connection...\n");
	
	ast_mutex_lock(&reconnectlock);
	trying_to_reconnect = 1;
	ast_mutex_unlock(&reconnectlock);
	
	int i = 1;
	int j = 0;
	while (1) {
		ast_mutex_lock(&unloadingmodlock);
		if (unloading_module) {
			ast_mutex_unlock(&unloadingmodlock);
			break;
		}
		ast_mutex_unlock(&unloadingmodlock);
		
		if (init_verbio() < 0) {
			/*ast_log( LOG_NOTICE, "it does not connect\n");*/
		} else { 
			close_verbio();
			if (init_verbio() >= 0) {
				ast_log(LOG_NOTICE, "Connection with server %s reestablished\n", server);
				ast_mutex_lock(&voxdownlock);
				voxserver_is_down = 0;
				ast_mutex_unlock(&voxdownlock);
				break;
			}
		}

		j = 0;
		while (j < (30 * i)) {
			sleep(1);
			ast_mutex_lock(&unloadingmodlock);
			if (unloading_module) {
				ast_log(LOG_NOTICE, "unload_module called...aborting reconnection\n");
				ast_mutex_unlock(&unloadingmodlock);
				break;
			}
			ast_mutex_unlock(&unloadingmodlock);
			++j;
		}
	
		if (i < 121)
			++i;
	}
	ast_mutex_lock(&reconnectlock);
	trying_to_reconnect = 0;
	ast_mutex_unlock(&reconnectlock);
}

/*! \brief Close connection with server */
static void close_verbio(void)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) { 
		ast_mutex_unlock(&voxdownlock); 
		return;
	} else {
		voxserver_is_down = 1;
	}
	ast_mutex_unlock(&voxdownlock);
	
	vox_libclose();
	ast_log(LOG_NOTICE, "vox_libclose()\n");
}

//ADDED: 2017-03-20
static int check_verbio(const char *host, int port, int timeout)
{
	int s, flags, res;
	struct pollfd pfds[1];
	struct sockaddr_in addr_in;
	struct hostent *hp;
	struct ast_hostent ahp;

	if (!(hp = ast_gethostbyname(host, &ahp))) {
		ast_log(LOG_WARNING, "Unable to locate host '%s'\n", host);
		return -1;
	}
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		ast_log(LOG_WARNING, "Unable to create socket: %s\n", strerror(errno));
		return -1;
	}
	if ((flags = fcntl(s, F_GETFL)) < 0) {
		ast_log(LOG_WARNING, "Fcntl(F_GETFL) failed: %s\n", strerror(errno));
		close(s);
		return -1;
	}
	if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
		ast_log(LOG_WARNING, "Fnctl(F_SETFL) failed: %s\n", strerror(errno));
		close(s);
		return -1;
	}
	memset(&addr_in, 0, sizeof(addr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(port);
	memcpy(&addr_in.sin_addr, hp->h_addr, sizeof(addr_in.sin_addr));
	if (connect(s, (struct sockaddr *)&addr_in, sizeof(addr_in)) && (errno != EINPROGRESS)) {
		ast_log(LOG_WARNING, "Connect failed with unexpected error: %s\n", strerror(errno));
		close(s);
		return -1;
	}

	pfds[0].fd = s;
	pfds[0].events = POLLOUT;
	while ((res = ast_poll(pfds, 1, 1000 * timeout)) != 1) {
		if (errno != EINTR) {
			if (!res) {
				ast_log(LOG_WARNING, "Connection to %s:%d timed out after timeout %d milliseconds\n", host, port, 1000 * timeout);
			} else
				ast_log(LOG_WARNING, "Connection to %s:%d failed: %s\n", host, port, strerror(errno));
			close(s);
			return -1;
		}
	}

	close(s);
	return 0;
}
//ADDED: 2017-03-20

/*! \brief Connect to verbio engine */
static int init_verbio(void)
{
	/*
	Return values:
		-1 - Something went wrong
		 0 - ASR and TTS
		 1 - ASR
		 2 - TTS
	*/

	/* Vars to store cfg file options*/
	struct ast_config *cfg;	
	const char *cfg_primary_server = NULL;
	const char *cfg_backup_server = NULL;
	int cfg_net_timeout = VERBIO_NET_TIMEOUT;
	const char *cfg_asr_config = NULL;
	const char *cfg_asr_language = NULL;
	const char *cfg_tts_language = NULL;
	const char *cfg_tts_spkr = NULL;
	const char *tmp = NULL;
	/* ASR, TTS availability */
	int asr = 0;
	int tts = 0;
	unsigned int retries = 10;
	unsigned int done = 0;
	unsigned int count = 0;
	
	/* -Get options from Verbio config file */
	cfg = verbio_load_asterisk_config();
	
	if (!cfg) {
		ast_log(LOG_ERROR, "Error opening configuration file %s\n", VERBIO_CFG);
		return -1;
	}
	if (!(cfg_primary_server = ast_variable_retrieve(cfg, "general", "primary_server"))) 
		cfg_primary_server = VERBIO_SERVER;
	if (!(cfg_backup_server = ast_variable_retrieve(cfg, "general", "backup_server"))) 	
		cfg_backup_server = VERBIO_SERVER;
	if ((tmp = ast_variable_retrieve(cfg, "general", "net_timeout")))
		cfg_net_timeout = atoi(tmp);
	if (!(cfg_asr_config = ast_variable_retrieve(cfg, "asr", "default_config")))
		ast_log(LOG_WARNING, "ASR 'default_config' option missing...ASR will not be available\n");
	if (!(cfg_asr_language = ast_variable_retrieve(cfg, "asr", "default_language")))
		ast_log(LOG_WARNING, "ASR 'default_language' option missing...ASR will not be available\n");
	if (!(cfg_tts_language = ast_variable_retrieve(cfg, "tts", "default_language"))) 
		ast_log(LOG_WARNING, "TTS 'default_language' option missing...TTS will not be available\n");
	if (!(cfg_tts_spkr = ast_variable_retrieve(cfg, "tts", "default_speaker"))) 
		ast_log(LOG_WARNING, "TTS default_speaker missing\n");

	sleep(1);

	/* Verbio Init stuff */
	/* Set timeout */
	vox_setparm(-1, VXGB_NETTIMEOUT, &cfg_net_timeout);
	
 	/* Init VOX library */
	/* Try to init ASR */
	if (cfg_asr_config && cfg_asr_language) {
		asr = done = count = 0;
		ast_log(LOG_NOTICE, "Starting ASR engine [config:%s, language:%s]\n", cfg_asr_config, cfg_asr_language);
		while ((count < retries) && (done == 0)) {
			ast_mutex_lock(&unloadingmodlock);
			if (unloading_module) {
				ast_mutex_unlock(&unloadingmodlock);
				break;
			}
			ast_mutex_unlock(&unloadingmodlock);

			sleep(1);
//ADDED: 2017-03-20
			if (check_verbio(cfg_primary_server, VERBIO_PORT, cfg_net_timeout) == 0) {
				/* Set primary server */
				vox_setparm(-1, VXGB_DEFSERVER, cfg_primary_server);
				if (vox_asr_init(cfg_asr_config, cfg_asr_language) == -1) {
					ast_log(LOG_ERROR, "vox_asr_init(%s, %s) failed on %s: %s\n", cfg_asr_config, cfg_asr_language, cfg_primary_server, ATVOX_ERRMSGP(-1));
				} else {
					asr = done = 1;
					ast_log(LOG_NOTICE, "vox_asr_init(%s, %s) succeeded on %s\n", cfg_asr_config, cfg_asr_language, cfg_primary_server);
					break;
				}
			}
			if (check_verbio(cfg_backup_server, VERBIO_PORT, cfg_net_timeout) == 0) {
				/* Set backup server */
				vox_setparm(-1, VXGB_DEFSERVER, cfg_backup_server);
				if (vox_asr_init(cfg_asr_config, cfg_asr_language) == -1) {
					asr = 0;
					ast_log(LOG_ERROR, "vox_asr_init(%s, %s) failed on %s: %s\n", cfg_asr_config, cfg_asr_language, cfg_backup_server, ATVOX_ERRMSGP(-1));
				} else {
					asr = done = 1;
					ast_log(LOG_NOTICE, "vox_asr_init(%s, %s) succeeded on %s\n", cfg_asr_config, cfg_asr_language, cfg_backup_server);
					break;
				}
			}
//ADDED: 2017-03-20
			++count;
		}
		if (done == 1)
			ast_log(LOG_NOTICE, "Starting ASR engine succeeded\n");
		else
			ast_log(LOG_ERROR, "Starting ASR engine failed, tried primary %s and backup %s servers: %s\n", cfg_primary_server, cfg_backup_server, ATVOX_ERRMSGP(-1));
	}
	
	/* Init VOX library */
	/* Try to init TTS */
	if (cfg_tts_language) {
		tts = done = count = 0;
		ast_log(LOG_NOTICE, "Starting TTS engine [language:%s]\n", cfg_tts_language);
		while ((count < retries) && (done == 0)) {
			ast_mutex_lock(&unloadingmodlock);
			if (unloading_module) {
				ast_mutex_unlock(&unloadingmodlock);
				break;
			}
			ast_mutex_unlock(&unloadingmodlock);
			
			sleep(1);
//ADDED: 2017-03-20
			if (check_verbio(cfg_primary_server, VERBIO_PORT, cfg_net_timeout) == 0) {
				/* Set primary server */
				vox_setparm(-1, VXGB_DEFSERVER, cfg_primary_server);
				if (vox_tts_init(NULL, cfg_tts_language) == -1) {
					ast_log(LOG_ERROR, "vox_tts_init(%s) failed on %s: %s\n", cfg_tts_language, cfg_primary_server, ATVOX_ERRMSGP(-1));
				} else {
					tts = done = 1;
					ast_log(LOG_NOTICE, "vox_tts_init(%s) succeeded on %s\n", cfg_tts_language, cfg_primary_server);
					break;
				}
			}
			if (check_verbio(cfg_backup_server, VERBIO_PORT, cfg_net_timeout) == 0) {
				/* Set backup server */
				vox_setparm(-1, VXGB_DEFSERVER, cfg_backup_server);
				if (vox_tts_init(NULL, cfg_tts_language) == -1) {
					tts = 0;
					ast_log(LOG_ERROR, "vox_tts_init(%s) failed on %s: %s\n", cfg_tts_language, cfg_backup_server, ATVOX_ERRMSGP(-1));
				} else {
					tts = done = 1;
					ast_log(LOG_NOTICE, "vox_tts_init(%s) succeeded on %s\n", cfg_tts_language, cfg_backup_server);
					break;
				}
			}
//ADDED: 2017-03-20
			++count;
		}
		if (done == 1)
			ast_log(LOG_NOTICE, "Starting TTS engine succeeded\n");
		else
			ast_log(LOG_ERROR, "Starting TTS engine failed, tried primary %s and backup %s servers: %s\n", cfg_primary_server, cfg_backup_server, ATVOX_ERRMSGP(-1));
	}
	if (asr || tts) {
		ast_mutex_lock(&voxdownlock);
		voxserver_is_down = 0;
		ast_mutex_unlock(&voxdownlock);
		vox_regsrvclose(lost_conn_callback);
	}

	ast_config_destroy(cfg);
	
	if (asr && tts)
		return 0;
	else if (asr)
		return 1;
	else if (tts)
		return 2;
	else
		return -1;
}

//ADDED: 2019-04-02
/*! \brief Helper function. Gets a valid id to be used as Verbio channel handle*/
static int verbio_get_channel_handle(struct ast_channel *chan)
{
	const char *tmp = NULL;
	int channel_handle = 0;

	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_CHANNEL_HANDLE")) && !ast_strlen_zero(tmp)) {
		channel_handle = atoi(tmp);
	} else {
		channel_handle = ast_atomic_fetchadd_int((int *)&channel_handle_idx, +1);
		char tmpbuff[VERBIO_MAX_INT_SIZE];
		if (snprintf(tmpbuff, VERBIO_MAX_INT_SIZE, "%d", channel_handle) > VERBIO_MAX_INT_SIZE)
			ast_log( LOG_WARNING, "VERBIO_CHANNEL_HANDLE may be truncated\n");
		pbx_builtin_setvar_helper(chan, "VERBIO_CHANNEL_HANDLE", tmpbuff);
	}
	
	return channel_handle;
}
//ADDED: 2019-04-02

/*!\brief Helper function. Set last verbio error as a channel variable 'var'.*/
static void verbio_set_err(struct ast_channel *chan, int channel_handle, const char *var)
{
	char *err_msg;
	long err_code;
	
	/* Get last verbio error */
	err_code = ATVOX_LASTERR(channel_handle);
	
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log(LOG_ERROR, "Voxserver is down\n"); 
		err_code = EVX_NETWORK;
	}
	ast_mutex_unlock(&voxdownlock);
	
	switch( err_code) {
		case 0:
			err_msg = "EVX_NOERROR";
			break;
		case -4:
			err_msg = "EVX_INVSETUP";
			break;
		case -5:
			err_msg = "EVX_NOMEM";
			break;
		case -6: 
			err_msg = "EVX_VCBFILE";
			break;
		case -7: 
			err_msg = "EVX_INVWORD";
			break;
		case -8: 
			err_msg = "EVX_NOLICFILE";
			break;
		case -9: 
			err_msg = "EVX_INVLIC";
			break;
		case -10: 
			err_msg = "EVX_SYSTEM";
			break;
		case -13: 
			err_msg = "EVX_NOLIBINIT";
			break;
		case -14: 
			err_msg = "EVX_NOLIC";
			break;
		case -15: 
			err_msg = "EVX_NOSETVCB";
			break;
		case -16: 
			err_msg = "EVX_NORECSTR";
			break;
		case -17: 
			err_msg = "EVX_NOLINE";
			break;
		case -18: 
			err_msg = "EVX_BADPARM";
			break;
		case -19: 
			err_msg = "EVX_NOTIMP";
			break;
		case -20: 
			err_msg = "EVX_NORECIND";
			break;
		case -21: 
			err_msg = "EVX_INVFILE";
			break;
		case -22: 
			err_msg = "EVX_NETWORK";
			break;
		case -23: 
			err_msg = "EVX_DICFILE";
			break;
		case -24: 
			err_msg = "EVX_PARSER";
			break;
		case -25: 
			err_msg = "EVX_INVVER";
			break;
		default: err_msg = "EVX_UNKNOWN";
	}
	
	pbx_builtin_setvar_helper(chan, var, err_msg);
}

static void verbio_remove_h(char *c)
{
	unsigned int x;
	if (c[0] == '[')
		for (x = 0; x < strlen( c)-1; x++) /* Remove first 3 chars '[H]'*/
			*(c+x) = *(c+x+3);
}

/*! \brief Helper function. Sets recognition result on Asterisk's dialplan (as channel var) */
static int verbio_set_asr_result(struct ast_channel *chan, int channel_handle, int verbose)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log(LOG_ERROR, "Voxserver is down\n"); 
		ast_mutex_unlock(&voxdownlock); 
		return 0;
	}
	ast_mutex_unlock(&voxdownlock);
	
	int nind = 0;
	ssize_t index[VERBIO_MAXINDEX+1];
	float score[VERBIO_MAXINDEX+1];
	char tmpbuff[VERBIO_MAX_RES_SIZE];
	char vtmpbuff[VERBIO_MAX_CVAR_SIZE];
	char itmpbuff[VERBIO_MAX_INT_SIZE];
	int i = 0;
	int scr = 0;
	
	/* Get result from recognizer */
	if ((nind = vox_recind(channel_handle, VERBIO_MAXINDEX, index, score, 0)) == -1) {
		ast_log(LOG_ERROR, "vox_recind(%d, %d) failed on %s: %s\n", channel_handle, VERBIO_MAXINDEX, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (verbose)
			ast_log(LOG_NOTICE, "vox_recind(%d, %d) succeeded on %s\n", channel_handle, VERBIO_MAXINDEX, chan->name);
	}

	/* Set Asterisk Channel vars */
	/* Number of recognized words */
	if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%d", nind) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "Number of recognized words may be truncated\n");
	pbx_builtin_setvar_helper(chan, "VASR_WORDS", tmpbuff);

	/* Backward compatibility */
	/* Index */
	if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%zd", index[0]) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "Index may be truncated\n");
	pbx_builtin_setvar_helper(chan, "VASR_INDEX", tmpbuff);

	/* Result */
	if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_word(channel_handle, index[0])) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "Result may be truncated\n");
	pbx_builtin_setvar_helper(chan, "VASR_RESULT", tmpbuff);
	
	/* Utterance */
	if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 1)) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "Utterance may be truncated\n");
	verbio_remove_h(tmpbuff);
	if (strlen( tmpbuff) > 0)
		pbx_builtin_setvar_helper(chan, "VASR_UTTERANCE", tmpbuff);
	
	/* Weight */
	if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 2)) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "Weight may be truncated\n");
	pbx_builtin_setvar_helper(chan, "VASR_WEIGHT", tmpbuff);

	/* Rule */
	if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 3)) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "Rule may be truncated\n");
	pbx_builtin_setvar_helper(chan, "VASR_RULE", tmpbuff);

	/* Score */
	scr = (int)score[0];
	if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%d",  scr) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "Score may be truncated\n");
	pbx_builtin_setvar_helper(chan, "VASR_SCORE", tmpbuff);

	if (nind == 0) {
		/* No words recognized */
		
		/* Index */
		if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%zd",  index[0]) > VERBIO_MAX_RES_SIZE)
			ast_log(LOG_WARNING, "Index may be truncated\n");
		pbx_builtin_setvar_helper(chan, "VASR_INDEX0", tmpbuff);
	
		/* Result */
		if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_word(channel_handle, index[0])) > VERBIO_MAX_RES_SIZE)
			ast_log(LOG_WARNING, "Result may be truncated\n");
		pbx_builtin_setvar_helper(chan, "VASR_RESULT0", tmpbuff);
		
		/* Utterance */
		if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 1)) > VERBIO_MAX_RES_SIZE)
			ast_log(LOG_WARNING, "Utterance may be truncated\n");
		verbio_remove_h(tmpbuff);
		if (strlen( tmpbuff) > 0)
			pbx_builtin_setvar_helper(chan, "VASR_UTTERANCE0", tmpbuff);
		
		/* Weight */
		if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 2)) > VERBIO_MAX_RES_SIZE)
			ast_log(LOG_WARNING, "Weight may be truncated\n");
		pbx_builtin_setvar_helper(chan, "VASR_WEIGHT0", tmpbuff);

		/* Rule */
		if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 3)) > VERBIO_MAX_RES_SIZE)
			ast_log(LOG_WARNING, "Rule may be truncated\n"); 
		pbx_builtin_setvar_helper(chan, "VASR_RULE0", tmpbuff);

		/* Score */
		pbx_builtin_setvar_helper(chan, "VASR_SCORE0", "0");
	} else {
		/* For each recognized word... */
		for (i = 0; i < nind; i++) {
			/* Convert our index to a char */
			if (snprintf(itmpbuff, VERBIO_MAX_INT_SIZE, "%d",  i) > VERBIO_MAX_INT_SIZE)
				ast_log(LOG_WARNING, "Number of recognized words may be truncated\n");

			/* Create our var names (to be set in Asterisk Dialplan)*/
			/* Index  */
			strcpy(vtmpbuff, "VASR_INDEX");
			strcat(vtmpbuff, itmpbuff);
			if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%zd",  index[i]) > VERBIO_MAX_RES_SIZE)
				ast_log(LOG_WARNING, "Index may be truncated\n");
			pbx_builtin_setvar_helper(chan, vtmpbuff, tmpbuff);
			
			/* Result */
			strcpy(vtmpbuff, "VASR_RESULT");
			strcat(vtmpbuff, itmpbuff);
			if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_word(channel_handle, index[i])) > VERBIO_MAX_RES_SIZE)
				ast_log(LOG_WARNING, "Result may be truncated\n"); 
			pbx_builtin_setvar_helper(chan, vtmpbuff, tmpbuff);
			
			/* Utterance */
			strcpy(vtmpbuff, "VASR_UTTERANCE");
			strcat(vtmpbuff, itmpbuff);
			if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[i], 1)) > VERBIO_MAX_RES_SIZE)
				ast_log(LOG_WARNING, "Utterance may be truncated\n"); 
			verbio_remove_h(tmpbuff);
			if (strlen(tmpbuff) > 0)
				pbx_builtin_setvar_helper(chan, vtmpbuff, tmpbuff);

			/* Weight */
			strcpy(vtmpbuff, "VASR_WEIGHT");
			strcat(vtmpbuff, itmpbuff);
			if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 2)) > VERBIO_MAX_RES_SIZE)
				ast_log(LOG_WARNING, "Weight may be truncated\n"); 
			pbx_builtin_setvar_helper(chan, vtmpbuff, tmpbuff);
	
			/* Rule */
			strcpy(vtmpbuff, "VASR_RULE");
			strcat(vtmpbuff, itmpbuff);
			if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%s", vox_wordex(channel_handle, index[0], 3)) > VERBIO_MAX_RES_SIZE)
				ast_log(LOG_WARNING, "Rule may be truncated\n"); 
			pbx_builtin_setvar_helper(chan, vtmpbuff, tmpbuff);
			
			/* Score */
			strcpy(vtmpbuff, "VASR_SCORE");
			strcat(vtmpbuff, itmpbuff);
			scr = (int)score[i];
			if (snprintf(tmpbuff, VERBIO_MAX_RES_SIZE, "%d",  scr) > VERBIO_MAX_RES_SIZE)
				ast_log(LOG_WARNING, "Score may be truncated\n");
			pbx_builtin_setvar_helper(chan, vtmpbuff, tmpbuff);
		}
	}
	return 0;
}

/*! \brief Helper function. Set dtmf detection result (as channel var) */
static void verbio_set_dtmf_result(struct ast_channel *chan, int subclass)
{
	char tmpbuff[VERBIO_MAX_RES_SIZE];
	char tmp[VERBIO_MAX_RES_SIZE];
	
	ast_log(LOG_NOTICE, "Pressed %c\n", subclass);

	/* Set result */
	pbx_builtin_setvar_helper(chan, "VDTMF_DETECTED", "TRUE");
	if (snprintf(tmp, VERBIO_MAX_RES_SIZE, "%c", subclass) > VERBIO_MAX_RES_SIZE)
		ast_log(LOG_WARNING, "DTMF may be truncated\n");

	/* Check if user already pressed a dtmf */
	const char *d = pbx_builtin_getvar_helper(chan, "VDTMF_RESULT");
	if (!ast_strlen_zero(d)) {
		strcpy(tmpbuff, d);
		pbx_builtin_setvar_helper(chan, "VDTMF_RESULT", strcat(tmpbuff, tmp));
	} else {
		pbx_builtin_setvar_helper(chan, "VDTMF_RESULT", tmp);
	}
}

/*! \brief Helper function. Reset dtmf detection channel var */
static void verbio_reset_dtmf_result(struct ast_channel *chan)
{
	pbx_builtin_setvar_helper(chan, "VDTMF_DETECTED", "FALSE");
	pbx_builtin_setvar_helper(chan, "VDTMF_RESULT", NULL);
}

/* Verbio Apps */
/*! \brief Text to speech application. */
static int verbio_prompt(struct ast_channel *chan, DATA_TYPE data)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log(LOG_ERROR, "Voxserver is down\n");
		ast_mutex_unlock(&voxdownlock);
		return 0;
	}
	ast_mutex_unlock(&voxdownlock);

	int res = 0;
	struct ast_module_user *u;
	char *args = NULL;
	char *argv[2];
	int argc = 0;
	char *options = NULL;
	char *opt = NULL;
	/* Input options */
	char *text_or_file = NULL;
	char *option_language = NULL;
	char *option_speaker = NULL;
	int	option_answer = 0;
	int option_beep = 0;
	int option_dtmf = 0;
	int option_no_hangup_on_verbio_err = 0;
	int option_asfile = 0;
	/* Config file options */
	struct ast_config *cfg;
	const char *cfg_primary_server = NULL;
	const char *cfg_backup_server = NULL;
	const char *cfg_language = NULL;
	const char *cfg_speaker = NULL;
	const char *cfg_tts_txt_path = NULL;
	int cfg_init_delay = VERBIO_INIT_DELAY;
	int cfg_end_delay = VERBIO_END_DELAY;
	int cfg_verbose = 0;
	int cfg_keep_synthesized_files = 0;
	const char *cfg_recorded_files_path = NULL;
	const char *tmp = NULL;
	/* Vars to be used when dealing with voxlib */
	const char *language = NULL;
	const char *speaker = NULL;
	const char *text_to_synth = NULL;
	int speed = 120;
	int volume = 32768;
	int channel_handle;
	ssize_t read_handle = 0;
	/* Keep recorded files related vars */
	int fflags = O_CREAT|O_TRUNC|O_WRONLY;
	struct ast_filestream *s = '\0';
	char filename[MAXPATHLEN] = "verbio-tts-";
	char rec_file_path[MAXPATHLEN];
	/* Misc vars */
	char text_full_path[MAXPATHLEN];
	int max_dtmf_len = 1;
	char dtmf_terminator = '#';
	int waitres = -1;
	struct ast_frame *f;
	char *samples[VERBIO_BUFFER_SIZE];

	verbio_reset_dtmf_result(chan);

	if (ast_strlen_zero(data)) {
		ast_log(LOG_ERROR, "%s requires an argument(text_or_file[|language][|speaker][|options])\n", verbio_prompt_app);
		return -1;
	}

	u = ast_module_user_add(chan);

	/* We need to make a copy of the input string if we are going to modify it! */
	args = ast_strdupa(data);
	if (!args) {
		ast_log(LOG_ERROR, "Out of memory!\n");
		ast_module_user_remove(u);
		return -1;
	}
	
	if ((argc = ast_app_separate_args(args, args_sep, argv, sizeof(argv) / sizeof(argv[0])))) {
		text_or_file = argv[0];
		options = argv[1];
	}

	/* Get options from user */
	/* language */
	option_language = strsep(&options, str_sep);
	if (option_language) {
		if (!strcmp(option_language, ""))
			option_language = NULL;
	}
	/* speaker */
	option_speaker = strsep(&options, str_sep);
	if (option_speaker) {
		if (!strcmp(option_speaker, ""))
			option_speaker = NULL;
	}
	/* options */
	opt = strsep(&options, str_sep);
	if (opt) {
		if (strchr(opt, 'a'))
			option_answer = 1;
		if (strchr(opt, 'b'))
			option_beep = 1;
		if (strchr(opt, 'd'))
			option_dtmf = 1;
		if (strchr(opt, 'n'))
			option_no_hangup_on_verbio_err = 1;
		if (strchr(opt, 'f'))
			option_asfile = 1;
	}
	
	/* Get options from Verbio config file */
	cfg = verbio_load_asterisk_config();
	if (!cfg) {
		ast_log(LOG_WARNING, "Error opening configuration file %s\n", VERBIO_CFG);
		ast_module_user_remove(u);
		return -1;
	}

	if (!(cfg_primary_server = ast_variable_retrieve(cfg, "general", "primary_server")))
		cfg_primary_server = VERBIO_SERVER;
	if (!(cfg_backup_server = ast_variable_retrieve(cfg, "general", "backup_server")))
		cfg_backup_server = VERBIO_SERVER;
	if (!(cfg_language = ast_variable_retrieve(cfg, "tts", "default_language")))
		ast_log(LOG_WARNING, "Error reading default_language option\n");
	if (!(cfg_speaker = ast_variable_retrieve(cfg, "tts", "default_speaker")))
		ast_log(LOG_WARNING, "Error reading default_speaker option\n");
	if (!(cfg_tts_txt_path = ast_variable_retrieve(cfg, "tts", "text_prompts_path"))) 
		cfg_tts_txt_path = VERBIO_TEXT_PROMPT_PATH;
	if ((tmp = ast_variable_retrieve(cfg, "tts", "init_delay")))
		cfg_init_delay = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "tts", "end_delay")))
		cfg_end_delay = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "debug", "verbose")))
		cfg_verbose = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "debug", "keep_synthesized_files")))
		cfg_keep_synthesized_files = atoi(tmp);
	cfg_recorded_files_path = ast_variable_retrieve(cfg, "debug", "recorded_files_path");

	/* Set options (to be used on voxlib calls) */
	/* language */
	if (!option_language)
		language = cfg_language;
	else
		language = option_language;
	/* speaker */
	if (!option_speaker)
		speaker = cfg_speaker;
	else
		speaker = option_speaker;
	if (option_asfile) {
		if (text_or_file[0] != '/') {
			strcpy(text_full_path, cfg_tts_txt_path);
			strcat(text_full_path, "/");
			strcat(text_full_path, text_or_file);
		} else {
			strcpy(text_full_path, text_or_file);
		}
		text_to_synth = ast_read_textfile(text_full_path);
	} else {
		text_to_synth = text_or_file;
	}

	channel_handle = verbio_get_channel_handle(chan);

	/* Save synthesized audio options */
	if (cfg_keep_synthesized_files) {
		char t_str[VERBIO_MAX_TSTR_SIZE];
		time_t t = time(NULL);
		if (snprintf(t_str, VERBIO_MAX_TSTR_SIZE, "%d", (int)t) > VERBIO_MAX_TSTR_SIZE)
			ast_log(LOG_WARNING, "Filename may be truncated\n");
		strcat(filename, t_str);
		strcat(filename, "-");
		strcat(filename, chan->uniqueid);
		if (cfg_recorded_files_path) {
			strcpy(rec_file_path, cfg_recorded_files_path);
			strcat(rec_file_path, "/");
			strcat(rec_file_path, filename);
		} else {
			strcpy(rec_file_path, VERBIO_DEF_REC_PATH);
			strcat(rec_file_path, filename);
		}
	} 
	
	/* Options set via chan vars */
	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_DTMF_MAXLEN")) && !ast_strlen_zero(tmp))
		max_dtmf_len = atoi(tmp);

	/* See if a terminator is specified */
	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_DTMF_TERMINATOR"))) {
		if (ast_strlen_zero(tmp))
			dtmf_terminator = '#';
		else
			dtmf_terminator = tmp[0];
	}
	
	if (cfg_verbose) {
		ast_log(LOG_NOTICE, "---------------------------\n");
		ast_log(LOG_NOTICE, "VerbioPrompt param summary:\n");
		ast_log(LOG_NOTICE, " Channel        : %s\n", chan->name);
		ast_log(LOG_NOTICE, " Primary server : %s\n", cfg_primary_server);
		ast_log(LOG_NOTICE, " Backup server  : %s\n", cfg_backup_server);
		ast_log(LOG_NOTICE, " Language       : %s\n", language);
		ast_log(LOG_NOTICE, " Speaker        : %s\n", speaker);
		ast_log(LOG_NOTICE, " Text to synth  : %s\n", text_to_synth);
		ast_log(LOG_NOTICE, " Init. delay    : %d\n", cfg_init_delay);
		ast_log(LOG_NOTICE, " End. delay     : %d\n", cfg_end_delay);
		if (option_dtmf) {
			ast_log(LOG_NOTICE, " Max dtmf len.  : %d\n", max_dtmf_len);
			ast_log(LOG_NOTICE, " Dtmf term.     : %c\n", dtmf_terminator);
		}
		if (cfg_keep_synthesized_files)
			ast_log(LOG_NOTICE, " Recording file : %s.%s\n", rec_file_path, EXT);
		ast_log(LOG_NOTICE, " Channel handle : %d\n", channel_handle);
		ast_log(LOG_NOTICE, "------------------------\n");
		ast_log(LOG_NOTICE, "\n");
	}

	/* Answer the channel (if not up) */
	if (chan->_state != AST_STATE_UP) {
		if (option_answer) {
			res = ast_answer(chan);
		} else {
			/* At the user's option, skip if the line is not up */
			ast_log(LOG_WARNING, "Not answering the channel\n");
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
			return -1;
		}
	}

	if (res) {
		ast_log(LOG_WARNING, "Could not answer channel '%s'\n", chan->name);
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}

	/* Set channel format */
	/* Verbio only accepts alaw ulaw and slin, we are going to use alaw */
	if (ast_set_write_format(chan, AUDIO_FORMAT) <  0) {
		ast_log(LOG_NOTICE, "AUDIO_FORMAT (write) failed\n");
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}
	if (ast_set_read_format(chan, AUDIO_FORMAT) <  0) {
		ast_log(LOG_NOTICE, "AUDIO_FORMAT (read) failed\n");
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}

	/* If we can not set asr language, exit */
	if (vox_setparm(channel_handle, VXCH_DEFTTSLANG, language) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_DEFTTSLANG, %s) failed on %s: %s\n", channel_handle, language, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_DEFTTSLANG, %s) succeeded on %s\n", channel_handle, language, chan->name);
	}
	/* Speaker is not critical */
	if (vox_setparm(channel_handle, VXCH_TTSSPKNAME, speaker) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_TTSSPKNAME, %s) failed on %s: %s\n", channel_handle, speaker, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_TTSSPKNAME, %s) succeeded on %s\n", channel_handle, speaker, chan->name);
	}
	if (vox_setparm(channel_handle, VXCH_TTSSPEED, &speed) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_TTSSPEED, %d) failed on %s: %s\n", channel_handle, speed, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_TTSSPEED, %d) succeeded on %s\n", channel_handle, speed, chan->name);
	}
	if (vox_setparm(channel_handle, VXCH_TTSVOLUME, &volume) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_TTSVOLUME, %d) failed on %s: %s\n", channel_handle, volume, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_TTSVOLUME, %d) succeeded on %s\n", channel_handle, volume, chan->name);
	}

	/* Open recognition stream */
	if ((read_handle = vox_playstr_open(channel_handle, text_to_synth, VERBIO_CODEC)) == -1) {
		ast_log(LOG_ERROR, "vox_playstr_open(%d, %s) failed on %s: %s\n", channel_handle, text_to_synth, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_playstr_open(%d, %s) succeeded on %s\n", channel_handle, text_to_synth, chan->name);
	}
	
	/* Keep a copy of what speaker hears and let the user know (via a chan var)
	   which file corresponds to actual recording */
	if (cfg_keep_synthesized_files) {
		s = ast_writefile(rec_file_path, EXT, NULL, fflags, 0, 0644);
		pbx_builtin_setvar_helper(chan, "VASR_REC_FILE", rec_file_path);
	}

	if (option_beep) {
		res = ast_streamfile(chan, "beep", chan->language);
		if (!res)
			res = ast_waitstream(chan, "");
		else
			ast_log(LOG_WARNING, "ast_streamfile failed on %s\n", chan->name);
		ast_stopstream(chan);
	}

	/* Ensure no streams are currently running */
	ast_stopstream(chan);

	ast_safe_sleep(chan, cfg_init_delay);

	/* Synthesisys loop */
	while (1) {
		waitres = ast_waitfor(chan, MAX_WAIT_TIME);
		if (waitres < 0) {
			ast_log(LOG_NOTICE, "Wait failed\n");
			if (vox_playstr_close(channel_handle, read_handle) == -1) {
				ast_log(LOG_ERROR, "vox_playstr_close(%d, %zd) failed on %s: %s\n", channel_handle, read_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_playstr_close(%d, %zd) succeeded on %s\n", channel_handle, read_handle, chan->name);
			}
			if (cfg_keep_synthesized_files)
				ast_closestream(s);
			ast_stopstream(chan);
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
			return -1;
		} else if (waitres == 0) {
			ast_log(LOG_NOTICE, "Wait timeout!\n");
			if (vox_playstr_read(read_handle, samples, 8000) < 8000)
				break;
			continue;
		}
		
		/* Read a frame */
		f = ast_read(chan);
		if (!f) {
			ast_log(LOG_NOTICE, "Hangup detected on %s\n", chan->name);
			if (vox_playstr_close(channel_handle, read_handle) == -1) {
				ast_log(LOG_ERROR, "vox_playstr_close(%d, %zd) failed on %s: %s\n", channel_handle, read_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_playstr_close(%d, %zd) succeeded on %s\n", channel_handle, read_handle, chan->name);
			}
			if (cfg_keep_synthesized_files)
				ast_closestream(s);
			ast_stopstream(chan);
			#ifdef VERBIO_CLOSE_ON_HANGUP
			if (vox_devclose(channel_handle) == -1) {
				ast_log(LOG_ERROR, "vox_devclose(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_devclose(%d) succeeded on %s\n", channel_handle, chan->name);
			}
			#endif
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
 			return -1;
		}
		
		if (f->frametype == AST_FRAME_VOICE) {
			/* Get frame from synthesizer */
			if (vox_playstr_read(read_handle, samples, f->samples) < f->samples) {
				ast_frfree(f);
				break;
			}

			/* Tell the frame which are it's new samples */
			f->data.ptr = samples;

			/* Write frame to file */
			if (cfg_keep_synthesized_files) {
				if (ast_writestream(s, f))
					ast_log(LOG_WARNING, "Error writing frame to %s on %s\n", rec_file_path, chan->name);
			}

			/* Write frame to chan */
			if (ast_write(chan, f))
				ast_log(LOG_ERROR, "Error writing frame to chan\n");
		} else if (f->frametype == AST_FRAME_DTMF) {
			if (option_dtmf) {
				/* Set detected dtmf to dialplan var */
				verbio_set_dtmf_result(chan, f->subclass.integer);
				int dtmflen = strlen(pbx_builtin_getvar_helper(chan, "VDTMF_RESULT"));
				if ((dtmflen >= max_dtmf_len) || f->subclass.integer == dtmf_terminator) {
					ast_log(LOG_NOTICE, "dtmf max len or terminator detected on %s\n", chan->name);
					ast_frfree(f);
					break;
				}
			}
		}
		ast_frfree(f);
	}

	if (cfg_keep_synthesized_files)
		ast_closestream(s);

	ast_safe_sleep(chan, cfg_end_delay);
	
	/* Cleanup */
	if (vox_playstr_close(channel_handle, read_handle) == -1) {
		ast_log(LOG_ERROR, "vox_playstr_close(%d, %zd) failed on %s: %s\n", channel_handle, read_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_playstr_close(%d, %zd) succeeded on %s\n", channel_handle, read_handle, chan->name);
	}
	
	ast_stopstream(chan);
	ast_module_user_remove(u);
	ast_config_destroy(cfg);
	return 0;
}

/*! \brief Recognition application. */
static int verbio_rec(struct ast_channel *chan, DATA_TYPE data)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log(LOG_ERROR, "Voxserver is down\n");
		ast_mutex_unlock(&voxdownlock);
		return 0;
	}
	ast_mutex_unlock(&voxdownlock);
	
	int res = 0;
	struct ast_module_user *u;
	char *args = NULL;
	char *argv[1];
	int argc = 0;
	char *options = NULL;
	char *opt = NULL;
	/* Input options */
	char *option_config = NULL;
	char *option_language = NULL;
	int option_init_sil = -1;
	int option_max_sil = -1;
	int option_abs_timeout = -1;
	int	option_answer = 0;
	int option_beep = 0;
	int option_dtmf = 0;
	int option_no_hangup_on_verbio_err = 0;
	/* Config file options */
	struct ast_config *cfg;
	const char *cfg_primary_server = NULL;
	const char *cfg_backup_server = NULL;
	const char *cfg_config = NULL;
	const char *cfg_language = NULL;
	int cfg_init_sil = VERBIO_INIT_SIL;
	int cfg_max_sil = VERBIO_MAX_SIL;
	int cfg_abs_timeout = VERBIO_ABS_TIMEOUT;
	float cfg_max_ref = VERBIO_MAX_REF;
	float cfg_min_ref = VERBIO_MIN_REF;
	int cfg_verbose = 0;
	int cfg_keep_recorded_files = 0;
	const char *cfg_recorded_files_path = NULL;
	const char *tmp = NULL;
	/* Vars to be used when dealing with voxlib */
	const char *config = NULL;
	const char *language = NULL;
	int init_sil;
	int max_sil;
	int	abs_timeout;
	int channel_handle;
	ssize_t write_handle;
	VX_RSP rsp;
	/* Keep recorded files related vars */
	int fflags = O_CREAT|O_TRUNC|O_WRONLY;
	struct ast_filestream *s = '\0';
	char filename[MAXPATHLEN] = "verbio-asr-";
	char rec_file_path[MAXPATHLEN];
	/* Misc vars */
	int max_dtmf_len = 1;
	char dtmf_terminator = '#';
	int waitres = -1;
	struct ast_frame *f;
	/* Timeout control */
	time_t start, current;

	verbio_reset_dtmf_result(chan);

	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "%s requires an argument ([|config][|language][|init_sil][|max_sil][|abs_timeout][|options])\n", verbio_rec_app);
		return -1;
	}

	u = ast_module_user_add(chan);

	/* We need to make a copy of the input string if we are going to modify it! */
	args = ast_strdupa(data);
	if (!args) {
		ast_log(LOG_ERROR, "Out of memory!\n");
		ast_module_user_remove(u);
		return -1;
	}
	
	if ((argc = ast_app_separate_args(args, args_sep, argv, sizeof(argv) / sizeof(argv[0]))))
		options = argv[0];

	/* Get options from user */
	/* config */
	option_config = strsep(&options, str_sep);
	if (option_config) {
		if (!strcmp(option_config, ""))
			option_config = NULL;
	}
	/* language */
	option_language = strsep(&options, str_sep);
	if (option_language) {
		if (!strcmp(option_language, ""))
			option_language = NULL;
	}
	/* init_sil */
	tmp = strsep(&options, str_sep);
	if (tmp)
		option_init_sil = atoi(tmp);
	/* max_sil */
	tmp = strsep(&options, str_sep);
	if (tmp)
		option_max_sil = atoi(tmp);
	/* abs_timeout */
	tmp = strsep(&options, str_sep);
	if (tmp)
		option_abs_timeout = atoi(tmp);
	/* options */
	opt = strsep(&options, str_sep);
	if (opt) {
		if (strchr(opt, 'a'))
			option_answer = 1;
		if (strchr(opt, 'b'))
			option_beep = 1;
		if (strchr(opt, 'd'))
			option_dtmf = 1;
		if (strchr(opt, 'n'))
			option_no_hangup_on_verbio_err = 1;
	}
	
	/* Get options from Verbio config file */
	cfg = verbio_load_asterisk_config();
	if (!cfg) {
		ast_log(LOG_WARNING, "Error opening configuration file %s\n", VERBIO_CFG);
		ast_module_user_remove(u);
		return -1;
	}

	if (!(cfg_primary_server = ast_variable_retrieve(cfg, "general", "primary_server")))
		cfg_primary_server = VERBIO_SERVER;
	if (!(cfg_backup_server = ast_variable_retrieve(cfg, "general", "backup_server")))
		cfg_backup_server = VERBIO_SERVER;
	if (!(cfg_config = ast_variable_retrieve(cfg, "asr", "default_config")))
		ast_log(LOG_WARNING, "Error reading default_config option\n");
	if (!(cfg_language = ast_variable_retrieve(cfg, "asr", "default_language")))
		ast_log(LOG_WARNING, "Error reading default_language option\n");
	if ((tmp = ast_variable_retrieve(cfg, "asr", "init_sil")))
		cfg_init_sil = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "max_sil")))
		cfg_max_sil = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "abs_timeout")))
		cfg_abs_timeout = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "max_ref")))
		cfg_max_ref = atof(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "min_ref")))
		cfg_min_ref = atof(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "debug", "verbose")))
		cfg_verbose = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "debug", "keep_recorded_files")))
		cfg_keep_recorded_files = atoi(tmp);
	cfg_recorded_files_path = ast_variable_retrieve(cfg, "debug", "recorded_files_path");

	/* Set options (to be used on voxlib calls) */
	/* config */
	if (!option_config)
		config = cfg_config;
	else
		config = option_config;
	/* language */
	if (!option_language)
		language = cfg_language;
	else
		language = option_language;
	/* init_sil */
	if (option_init_sil == -1)
		init_sil = cfg_init_sil;
	else
		init_sil = option_init_sil;
	/* max_sil */
	if (!option_max_sil)
		max_sil = cfg_max_sil;
	else
		max_sil = option_max_sil;
	/* abs_timeout */
	if (option_abs_timeout == -1)
		abs_timeout = cfg_abs_timeout;
	else
		abs_timeout = option_abs_timeout;

	channel_handle = verbio_get_channel_handle(chan);
	
	/* Save recorded audio options */
	if (cfg_keep_recorded_files) {
		char t_str[VERBIO_MAX_TSTR_SIZE];
		time_t t = time(NULL);
		if (snprintf(t_str, VERBIO_MAX_TSTR_SIZE, "%d", (int)t) > VERBIO_MAX_TSTR_SIZE)
			ast_log(LOG_WARNING, "Filename may be truncated\n");
		strcat(filename, t_str);
		strcat(filename, "-");
		strcat(filename, chan->uniqueid);
		if (cfg_recorded_files_path) {
			strcpy(rec_file_path, cfg_recorded_files_path);
			strcat(rec_file_path, "/");
			strcat(rec_file_path, filename);
		} else {
			strcpy(rec_file_path, VERBIO_DEF_REC_PATH);
			strcat(rec_file_path, filename);
		}
	}
	
	/* Options set via chan vars */
	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_DTMF_MAXLEN")) && !ast_strlen_zero(tmp))
		max_dtmf_len = atoi(tmp);

	/* See if a terminator is specified */
	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_DTMF_TERMINATOR"))) {
		if (ast_strlen_zero(tmp))
			dtmf_terminator = '#';
		else
			dtmf_terminator = tmp[0];
	}
	
	if (cfg_verbose) {
		ast_log(LOG_NOTICE, "------------------------\n");
		ast_log(LOG_NOTICE, "VerbioRec param summary:\n");
		ast_log(LOG_NOTICE, " Channel        : %s\n", chan->name);
		ast_log(LOG_NOTICE, " Primary server : %s\n", cfg_primary_server);
		ast_log(LOG_NOTICE, " Backup server  : %s\n", cfg_backup_server);
		ast_log(LOG_NOTICE, " Configuration  : %s\n", config);
		ast_log(LOG_NOTICE, " Language       : %s\n", language);
		ast_log(LOG_NOTICE, " Init. silence  : %d\n", init_sil);
		ast_log(LOG_NOTICE, " Max. silence   : %d\n", max_sil);
		ast_log(LOG_NOTICE, " Abs. timeout   : %d\n", abs_timeout);
		ast_log(LOG_NOTICE, " Max. ref.      : %f\n", cfg_max_ref);
		ast_log(LOG_NOTICE, " Min. ref.      : %f\n", cfg_min_ref);
		if (option_dtmf) {
			ast_log(LOG_NOTICE, " Max dtmf len.  : %d\n", max_dtmf_len);
			ast_log(LOG_NOTICE, " Dtmf term.     : %c\n", dtmf_terminator);
		}
		if (cfg_keep_recorded_files)
			ast_log(LOG_NOTICE, " Recording file : %s.%s\n", rec_file_path, EXT);
		ast_log(LOG_NOTICE, " Channel handle : %d\n", channel_handle);
		ast_log(LOG_NOTICE, "------------------------\n");
		ast_log(LOG_NOTICE, "\n");
	}

	/* Answer the channel (if not up) */
	if (chan->_state != AST_STATE_UP) {
		if (option_answer) {
			res = ast_answer(chan);
		} else {
			/* At the user's option, skip if the line is not up */
			ast_log(LOG_WARNING, "Not answering the channel\n");
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
			return -1;
		}	
	}
	
	if (res) {
		ast_log(LOG_WARNING, "Could not answer channel '%s'\n", chan->name);
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}

	/* Set channel format */
	/* Verbio only accepts alaw ulaw and slin, we are going to use alaw */
	if (ast_set_write_format(chan, AUDIO_FORMAT) <  0) {
		ast_log(LOG_NOTICE, "AUDIO_FORMAT (write) failed\n");
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}
	if (ast_set_read_format(chan, AUDIO_FORMAT) <  0) {
		ast_log(LOG_NOTICE, "AUDIO_FORMAT (read) failed\n");
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}

	/* Verbio rec stuff */
	vox_clrrsp(&rsp);
	rsp.maxsil = max_sil;
	rsp.initsil = init_sil;

	/* If we can not set asr config, exit */
	if (vox_setparm(channel_handle, VXCH_DEFASRCFG, config) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_DEFASRCFG, %s) failed on %s: %s\n", channel_handle, config, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_DEFASRCFG, %s) succeeded on %s\n", channel_handle, config, chan->name);
	}
	/* If we can not set asr language, exit */
	if (vox_setparm(channel_handle, VXCH_DEFASRLANG, language) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_DEFASRLANG, %s) failed on %s: %s\n", channel_handle, language, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_DEFASRLANG, %s) succeeded on %s\n", channel_handle, language, chan->name);
	}

	/* Open recognition stream */
	if ((write_handle = vox_recstr_open(channel_handle, &rsp, VERBIO_CODEC | MC_INITSIL)) == -1) {
		ast_log(LOG_ERROR, "vox_recstr_open(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_recstr_open(%d) succeeded on %s\n", channel_handle, chan->name);
	}

	/* max_ref */
	if (cfg_max_ref > VERBIO_MAX_REF)
		cfg_max_ref = VERBIO_MAX_REF;
	
	if (vox_setparm(channel_handle, VXGB_VSDMAXREF, &cfg_max_ref) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXGB_VSDMAXREF, %f) failed on %s: %s\n", channel_handle, cfg_max_ref, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXGB_VSDMAXREF, %f) succeeded on %s\n", channel_handle, cfg_max_ref, chan->name);
	}

	/* min_ref */
	if (cfg_min_ref >= VERBIO_MAX_REF) {
		cfg_min_ref = VERBIO_MAX_REF * 0.8;
		ast_log(LOG_WARNING, "(min_ref >= VERBIO_MAX_REF). min_ref set to: %f\n", cfg_min_ref);
	}

	if (vox_setparm(channel_handle, VXGB_VSDMINREF, &cfg_min_ref) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXGB_VSDMINREF, %f) failed on %s: %s\n", channel_handle, cfg_min_ref, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXGB_VSDMINREF, %f) succeeded on %s\n", channel_handle, cfg_min_ref, chan->name);
	}

	/* Keep a copy of what speaker says and let the user know (via a chan var)
	   which file corresponds to actual recording */
	if (cfg_keep_recorded_files) {
		s = ast_writefile(rec_file_path, EXT, NULL, fflags, 0, 0644);
		pbx_builtin_setvar_helper(chan, "VASR_REC_FILE", rec_file_path);
	}
	
	if (option_beep) {
		res = ast_streamfile(chan, "beep", chan->language);
		if (!res)
			res = ast_waitstream(chan, "");
		else
			ast_log(LOG_WARNING, "ast_streamfile failed on %s\n", chan->name);
		ast_stopstream(chan);
	}

	/* Ensure no streams are currently running */
	ast_stopstream(chan);

	/* Recognition loop */
	time(&start);
	while (1) {
		time(&current);
		if ((current - start) >= abs_timeout) {
			ast_log(LOG_NOTICE, "Absolute timeout reached: %d seconds\n", (int)(current - start));
			break;
		}
		waitres = ast_waitfor(chan, MAX_WAIT_TIME);
		if (waitres < 0) {
			ast_log(LOG_NOTICE, "Wait failed\n");
			if (vox_recstr_close(channel_handle, write_handle) == -1) {
				ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
			}
			if (cfg_keep_recorded_files)
				ast_closestream(s);
			ast_stopstream(chan);
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
			return -1;
		} else if (waitres == 0) {
			ast_log(LOG_NOTICE, "Wait timeout!\n");
			continue;
		}

		/* Read a frame */
		f = ast_read(chan);
		if (!f) {
			ast_log(LOG_NOTICE, "Hangup detected on %s\n", chan->name);
			if (vox_recstr_close(channel_handle, write_handle) == -1) {
				ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
			}
			if (cfg_keep_recorded_files)
				ast_closestream(s);
			ast_stopstream(chan);
			#ifdef VERBIO_CLOSE_ON_HANGUP
			if (vox_devclose(channel_handle) == -1) {
				ast_log(LOG_ERROR, "vox_devclose(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_devclose(%d) succeeded on %s\n", channel_handle, chan->name);
			}
			#endif
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
 			return -1;
		}
		
		if (f->frametype == AST_FRAME_VOICE) {
			/* Write frame to file */
			if (cfg_keep_recorded_files) {
				if (ast_writestream(s, f))
					ast_log(LOG_WARNING, "Error writing frame to %s on %s\n", rec_file_path, chan->name);
			}
			/* Pass our frame to recognizer */
			if (vox_recstr_write(write_handle, f->data.ptr, f->samples) < f->samples) {
				ast_frfree(f);
				break;
			}
		} else if (f->frametype == AST_FRAME_DTMF) {
			if (option_dtmf) {
				/* Set detected dtmf to dialplan var */
				verbio_set_dtmf_result(chan, f->subclass.integer);
				int dtmflen = strlen(pbx_builtin_getvar_helper(chan, "VDTMF_RESULT"));
				if ((dtmflen >= max_dtmf_len) || f->subclass.integer == dtmf_terminator) {
					ast_log(LOG_NOTICE, "dtmf max len or terminator detected on %s\n", chan->name);
					ast_frfree(f);
					break;
				}
			}
		}
		ast_frfree(f);
	}

	if (cfg_keep_recorded_files)
		ast_closestream(s);
	
	/* Get and set (as chan vars) asr result */
	verbio_set_asr_result(chan, channel_handle, cfg_verbose);

	/* Cleanup */
	if (vox_recstr_close(channel_handle, write_handle) == -1) {
		ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
	}
	
	ast_stopstream(chan);
	ast_module_user_remove(u);
	ast_config_destroy(cfg);
	return 0;
}

/*! \brief Stream and recognition application (implements bargein). */
static int verbio_stream_and_rec( struct ast_channel *chan, DATA_TYPE data)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log(LOG_ERROR, "Voxserver is down\n");
		ast_mutex_unlock(&voxdownlock);
		return 0;
	}
	ast_mutex_unlock(&voxdownlock);
	
	int res = 0;
	struct ast_module_user *u;
	char *args = NULL;
	char *argv[2];
	int argc = 0;
	char *options = NULL;
	char *opt = NULL;
	/* Input options */
	char *audio_file = NULL;
	char *option_config = NULL;
	char *option_language = NULL;
	int option_init_sil = -1;
	int option_max_sil = -1;
	int option_abs_timeout = -1;
	int	option_answer = 0;
	int option_bargein = 0;
	int option_beep = 0;
	int option_dtmf = 0;
	int option_interrupt = 0;
	int option_no_hangup_on_verbio_err = 0;
	int option_stop_on_dtmf	= 0;
	/* Config file opts */
	struct ast_config *cfg;
	const char *cfg_primary_server = NULL;
	const char *cfg_backup_server = NULL;
	const char *cfg_config = NULL;
	const char *cfg_language = NULL;
	int cfg_init_sil = VERBIO_INIT_SIL;
	int cfg_max_sil = VERBIO_MAX_SIL;
	int cfg_abs_timeout = VERBIO_ABS_TIMEOUT;
	float cfg_max_ref = VERBIO_MAX_REF;
	float cfg_min_ref = VERBIO_MIN_REF;
	float cfg_low_factor = VERBIO_LOW_FACTOR;
	float cfg_high_factor = VERBIO_HIGH_FACTOR;
	float cfg_final_factor = VERBIO_FINAL_FACTOR;
	float cfg_final_high_factor = VERBIO_FINAL_HIGH_FACTOR;
	float cfg_min_high_thresh = VERBIO_MIN_HIGH_THRESH;
	float cfg_aam_min = VERBIO_AAM_MIN;
	float cfg_aam_max = VERBIO_AAM_MAX;
	int cfg_verbose = 0;
	int cfg_keep_recorded_files = 0;
	const char *cfg_recorded_files_path = NULL;
	const char *tmp = NULL;
	/* Vars to be used when dealing with voxlib */
	const char *config = NULL;
	const char *language = NULL;
	int init_sil;
	int max_sil;
	int	abs_timeout;
	int channel_handle;
	VAD_PARAM *vad_handle = 0;
	ssize_t write_handle;
	VX_RSP rsp;
	/* Keep recorded files related vars */
	int fflags = O_CREAT|O_TRUNC|O_WRONLY;
	struct ast_filestream *s = '\0';
	char filename[MAXPATHLEN] = "verbio-asr-";
	char rec_file_path[MAXPATHLEN];
	/* Misc vars */
	int max_dtmf_len = 1;
	char dtmf_terminator = '#';
	int waitres = -1;
	struct ast_frame *f;
	char *front = NULL;
	/* Timeout control */
	time_t start, current;

	verbio_reset_dtmf_result(chan);

	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "%s requires an argument (audio_file[|init_sil][|max_sil][|config][|language][|abs_timeout][|options])\n", verbio_stream_and_rec_app);
		return -1;
	}

	u = ast_module_user_add(chan);

	/* We need to make a copy of the input string if we are going to modify it! */
	args = ast_strdupa(data);	
	if (!args) {
		ast_log(LOG_ERROR, "Out of memory!\n");
		ast_module_user_remove(u);
		return -1;
	}

	if ((argc = ast_app_separate_args(args, args_sep, argv, sizeof(argv) / sizeof(argv[0])))) {
		audio_file = argv[0];
		options = argv[1];
	}

	/* Get options from user */
	/* init_sil */
	tmp = strsep(&options, str_sep);
	if (tmp)
		option_init_sil = atoi(tmp);
	/* max_sil */
	tmp = strsep(&options, str_sep);
	if (tmp)
		option_max_sil = atoi(tmp);
	/* config */
	option_config = strsep(&options, str_sep);
	if (option_config) {
		if (!strcmp(option_config, ""))
			option_config = NULL;
	}
	/* language */
	option_language = strsep(&options, str_sep);
	if (option_language) {
		if (!strcmp(option_language, ""))
			option_language = NULL;
	}
	/* abs_timeout */
	tmp = strsep(&options, str_sep);
	if (tmp)
		option_abs_timeout = atoi(tmp);
	/* options */
	opt = strsep(&options, str_sep);
	if (opt) {
		if (strchr(opt, 'a'))
			option_answer = 1;
		if (strchr(opt, 'g')) {
			option_bargein = 1;
			if (option_beep == 1) {
				ast_log(LOG_WARNING, "Bargein is active...I'm going to disable beep\n");
				/* No sense if bargein active */
				option_beep = 0;
			}
		}
		if (strchr(opt, 'b'))
			option_beep = 1;
		if (strchr(opt, 'd'))
			option_dtmf = 1;
		if (strchr(opt, 'i')) {
			option_interrupt = 1;
			if (option_bargein == 0) {
				ast_log(LOG_WARNING, "Immediate interrupt is active...I'm going to enable bargein\n");
				/* Bargein needs to be active */
				option_bargein = 1;
			}
			if (option_beep == 1) {
				ast_log(LOG_WARNING, "Bargein is active...I'm going to disable beep\n");
				/* No sense if bargein active */
				option_beep = 0;
			}
		}
		if (strchr(opt, 'n'))
			option_no_hangup_on_verbio_err = 1;
		if (strchr(opt, 's'))
			option_stop_on_dtmf = 1;
	}

	/* Get options from Verbio config file */
	cfg = verbio_load_asterisk_config();
	if (!cfg) {
		ast_log(LOG_WARNING, "Error opening configuration file %s\n", VERBIO_CFG);
		ast_module_user_remove(u);
		return -1;
	}

	if (!(cfg_primary_server = ast_variable_retrieve(cfg, "general", "primary_server")))
		cfg_primary_server = VERBIO_SERVER;
	if (!(cfg_backup_server = ast_variable_retrieve(cfg, "general", "backup_server")))
		cfg_backup_server = VERBIO_SERVER;
	if (!(cfg_config = ast_variable_retrieve(cfg, "asr", "default_config")))
		ast_log(LOG_WARNING, "Error reading default_config option\n");
	if (!(cfg_language = ast_variable_retrieve(cfg, "asr", "default_language")))
		ast_log(LOG_WARNING, "Error reading default_language option\n");
	if ((tmp = ast_variable_retrieve(cfg, "asr", "init_sil")))
		cfg_init_sil = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "max_sil")))
		cfg_max_sil = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "abs_timeout")))
		cfg_abs_timeout = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "max_ref")))
		cfg_max_ref = atof(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "asr", "min_ref")))
		cfg_min_ref = atof(tmp);
	if (option_interrupt) {
		if ((tmp = ast_variable_retrieve(cfg, "vad", "low_factor")))
			cfg_low_factor = atof(tmp);
		if ((tmp = ast_variable_retrieve(cfg, "vad", "high_factor")))
			cfg_high_factor = atof(tmp);
		if ((tmp = ast_variable_retrieve(cfg, "vad", "final_factor")))
			cfg_final_factor = atof(tmp);
		if ((tmp = ast_variable_retrieve(cfg, "vad", "final_high_factor")))
			cfg_final_high_factor = atof(tmp);
		if ((tmp = ast_variable_retrieve(cfg, "vad", "min_high_thresh")))
			cfg_min_high_thresh = atof(tmp);
		if ((tmp = ast_variable_retrieve(cfg, "vad", "aam_min")))
			cfg_aam_min = atof(tmp);
		if ((tmp = ast_variable_retrieve(cfg, "vad", "aam_max")))
			cfg_aam_max = atof(tmp);
	}
	if ((tmp = ast_variable_retrieve(cfg, "debug", "verbose")))
		cfg_verbose = atoi(tmp);
	if ((tmp = ast_variable_retrieve(cfg, "debug", "keep_recorded_files")))
		cfg_keep_recorded_files = atoi(tmp);
	cfg_recorded_files_path = ast_variable_retrieve(cfg, "debug", "recorded_files_path");

	/* Set options (to be used on voxlib calls) */
	/* config */
	if (!option_config)
		config = cfg_config;
	else
		config = option_config;
	/* language */
	if (!option_language)
		language = cfg_language;
	else
		language = option_language;
	/* init_sil */
	if (option_init_sil == -1)
		init_sil = cfg_init_sil;
	else
		init_sil = option_init_sil;
	/* max_sil */
	if (!option_max_sil)
		max_sil = cfg_max_sil;
	else
		max_sil = option_max_sil;
	/* abs_timeout */
	if (option_abs_timeout == -1)
		abs_timeout = cfg_abs_timeout;
	else
		abs_timeout = option_abs_timeout;

	channel_handle = verbio_get_channel_handle(chan);	

	/* Save recorded audio options */
	if (cfg_keep_recorded_files) {
		char t_str[VERBIO_MAX_TSTR_SIZE];
		time_t t = time(NULL);
		if (snprintf(t_str, VERBIO_MAX_TSTR_SIZE, "%d", (int)t) > VERBIO_MAX_TSTR_SIZE)
			ast_log(LOG_WARNING, "Filename may be truncated\n");
		strcat(filename, t_str);
		strcat(filename, "-");
		strcat(filename, chan->uniqueid);
		if (cfg_recorded_files_path) {
			strcpy(rec_file_path, cfg_recorded_files_path);
			strcat(rec_file_path, "/");
			strcat(rec_file_path, filename);
		} else {
			strcpy(rec_file_path, VERBIO_DEF_REC_PATH);
			strcat(rec_file_path, filename);
		}
	}

	/* Options set via chan vars */
	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_DTMF_MAXLEN")) && !ast_strlen_zero(tmp))
		max_dtmf_len = atoi(tmp);

	/* See if a terminator is specified */
	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_DTMF_TERMINATOR"))) {
		if (ast_strlen_zero(tmp))
			dtmf_terminator = '#';
		else
			dtmf_terminator = tmp[0];
	}

	if (cfg_verbose) {
		ast_log(LOG_NOTICE, "---------------------------------\n");
		ast_log(LOG_NOTICE, "VerbioStreamAndRec param summary:\n");
		ast_log(LOG_NOTICE, " Channel        : %s\n", chan->name);
		ast_log(LOG_NOTICE, " Primary server : %s\n", cfg_primary_server);
		ast_log(LOG_NOTICE, " Backup server  : %s\n", cfg_backup_server);
		ast_log(LOG_NOTICE, " Configuration  : %s\n", config);
		ast_log(LOG_NOTICE, " Language       : %s\n", language);
		ast_log(LOG_NOTICE, " Init. silence  : %d\n", init_sil);
		ast_log(LOG_NOTICE, " Max.  silence  : %d\n", max_sil);
		ast_log(LOG_NOTICE, " Abs. timeout   : %d\n", abs_timeout);
		ast_log(LOG_NOTICE, " Max. ref.      : %f\n", cfg_max_ref);
		ast_log(LOG_NOTICE, " Min. ref.      : %f\n", cfg_min_ref);
		ast_log(LOG_NOTICE, " File to play   : %s\n", audio_file);
		if (option_dtmf) {
			ast_log(LOG_NOTICE, " Max dtmf len.  : %d\n", max_dtmf_len);
			ast_log(LOG_NOTICE, " Dtmf term.     : %c\n", dtmf_terminator);
		}
		if (cfg_keep_recorded_files)
			ast_log(LOG_NOTICE, " Recording file : %s.%s\n", rec_file_path, EXT);
		if (option_interrupt) {
			ast_log(LOG_NOTICE, "  Low factor    : %f\n", cfg_low_factor);
			ast_log(LOG_NOTICE, "  High factor   : %f\n", cfg_high_factor);
			ast_log(LOG_NOTICE, "  Final factor  : %f\n", cfg_final_factor);
			ast_log(LOG_NOTICE, "  Final high fc.: %f\n", cfg_final_high_factor);
			ast_log(LOG_NOTICE, "  Min high th.  : %f\n", cfg_min_high_thresh);
			ast_log(LOG_NOTICE, "  Aam min.      : %f\n", cfg_aam_min);
			ast_log(LOG_NOTICE, "  Aam max.      : %f\n", cfg_aam_max);
		}
		ast_log(LOG_NOTICE, " Channel handle : %d\n", channel_handle);
		ast_log(LOG_NOTICE, "---------------------------------\n");
		ast_log(LOG_NOTICE, "\n");
	}

	/* Answer the channel (if not up) */
	if (chan->_state != AST_STATE_UP) {
		if (option_answer) {
			res = ast_answer(chan);
		} else {
			/* At the user's option, skip if the line is not up */
			ast_log(LOG_WARNING, "Not answering the channel\n");
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
			return -1;
		}
	}

	if (res) {
		ast_log(LOG_WARNING, "Could not answer channel '%s'\n", chan->name);
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}

	/* Set channel format */
	/* Verbio only accepts alaw ulaw and slin, we are going to use alaw */
	if (ast_set_write_format(chan, AUDIO_FORMAT) <  0) {
		ast_log(LOG_NOTICE, "AUDIO_FORMAT (write) failed\n");
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}
	if (ast_set_read_format(chan, AUDIO_FORMAT) <  0) {
		ast_log(LOG_NOTICE, "AUDIO_FORMAT (read) failed\n");
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		return -1;
	}

	/* Verbio rec stuff */
	vox_clrrsp(&rsp);
	rsp.maxsil = max_sil;
	rsp.initsil = init_sil;

	/* If we can not set asr config, exit */
	if (vox_setparm(channel_handle, VXCH_DEFASRCFG, config) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_DEFASRCFG, %s) failed on %s: %s\n", channel_handle, config, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_DEFASRCFG, %s) succeeded on %s\n", channel_handle, config, chan->name);
	}
	/* If we can not set asr language, exit */
	if (vox_setparm(channel_handle, VXCH_DEFASRLANG, language) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_DEFASRLANG, %s) failed on %s: %s\n", channel_handle, language, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_DEFASRLANG, %s) succeeded on %s\n", channel_handle, language, chan->name);
	}

	/* Open recognition stream */
	if ((write_handle = vox_recstr_open(channel_handle, &rsp, VERBIO_CODEC | MC_INITSIL)) == -1) {
		ast_log(LOG_ERROR, "vox_recstr_open(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;	
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_recstr_open(%d) succeeded on %s\n", channel_handle, chan->name);
	}

	/* max_ref */
	if (cfg_max_ref > VERBIO_MAX_REF)
		cfg_max_ref = VERBIO_MAX_REF;
	
	if (vox_setparm(channel_handle, VXGB_VSDMAXREF, &cfg_max_ref) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXGB_VSDMAXREF, %f) failed on %s: %s\n", channel_handle, cfg_max_ref, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXGB_VSDMAXREF, %f) succeeded on %s\n", channel_handle, cfg_max_ref, chan->name);
	}

	/* min_ref */
	if (cfg_min_ref >= VERBIO_MAX_REF) {
		cfg_min_ref = VERBIO_MAX_REF * 0.8;
		ast_log(LOG_WARNING, "(min_ref >= VERBIO_MAX_REF). min_ref set to: %f\n", cfg_min_ref);
	}

	if (vox_setparm(channel_handle, VXGB_VSDMINREF, &cfg_min_ref) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXGB_VSDMINREF, %f) failed on %s: %s\n", channel_handle, cfg_min_ref, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXGB_VSDMINREF, %f) succeeded on %s\n", channel_handle, cfg_min_ref, chan->name);
	}

	/* Keep a copy of what speaker says and let the user know (via a chan var)
	   which file corresponds to actual recording */
	if (cfg_keep_recorded_files) {
		s = ast_writefile(rec_file_path, EXT, NULL, fflags, 0, 0644);
		pbx_builtin_setvar_helper(chan, "VASR_REC_FILE", rec_file_path);
	}

	/* Verbio VAD stuff */
	if (option_interrupt) {
		VAD_PRM prm;
		vox_clrvad(&prm);
		prm.low_factor = cfg_low_factor;
		prm.high_factor = cfg_high_factor;
		prm.final_factor = cfg_final_factor;
		prm.final_high_factor = cfg_final_high_factor;
		prm.min_high_thresh = cfg_min_high_thresh;
		prm.aam_min = cfg_aam_min;
		prm.aam_max = cfg_aam_max;

		vad_handle = vox_vsd_open(channel_handle, write_handle, &prm, VERBIO_CODEC);
		if (vad_handle == (VAD_PARAM *)-1) {
			ast_log(LOG_ERROR, "vox_vsd_open(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			if (vox_recstr_close(channel_handle, write_handle) == -1) {
				ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
			}
			if (cfg_keep_recorded_files)
				ast_closestream(s);
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
			if (option_no_hangup_on_verbio_err)
				return 0;
			else
				return -1;
		} else {
			if (cfg_verbose)
				ast_log(LOG_NOTICE, "vox_vsd_open(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
		}
	}

	/* Ensure no streams are currently running */
	ast_stopstream(chan);

	/* Stream file */
	front = audio_file;
	res = ast_streamfile(chan, front, chan->language);
	if (res) {
		ast_log(LOG_WARNING, "ast_streamfile failed on %s for %s\n", chan->name, (char *)front);
		if (vox_recstr_close(channel_handle, write_handle) == -1) {
			ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
		} else {
			if (cfg_verbose)
				ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
		}
		if (cfg_keep_recorded_files)
			ast_closestream(s);
		ast_config_destroy(cfg);
		return -1;
	}

	if (!option_bargein) {
		if (option_dtmf) {
			ast_log(LOG_NOTICE, "Waiting for digit...\n");
			
			int rtimeout = 0;
			res = ast_waitstream(chan, AST_DIGIT_ANY);

			while (res && !rtimeout) {
				verbio_set_dtmf_result(chan, res);
				int dtmflen = strlen(pbx_builtin_getvar_helper(chan, "VDTMF_RESULT"));
				
				if ((dtmflen >= max_dtmf_len) || ( res == dtmf_terminator)) {
					ast_log(LOG_NOTICE, "dtmf max len or terminator detected\n");
					if (vox_recstr_close(channel_handle, write_handle) == -1) {
						ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
					} else {
						if (cfg_verbose)
							ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
					}
					if (option_interrupt) {
						if (vox_vsd_close(channel_handle, vad_handle) == -1) {
							ast_log(LOG_ERROR, "vox_vsd_close(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
						} else {
							if (cfg_verbose)
								ast_log(LOG_NOTICE, "vox_vsd_close(%d) succeeded on %s\n", channel_handle, chan->name);
						}
					}
					if (cfg_keep_recorded_files)
						ast_closestream(s);
					ast_stopstream(chan);
					ast_module_user_remove(u);
					ast_config_destroy(cfg);
					return 0;
				}
				if (option_stop_on_dtmf) {
					/*ast_seekstream(chan->stream, 0, SEEK_SET);*/
					ast_stopstream(chan);
					time(&start);
					for (;;) {
						/*res = ast_waitstream(chan, AST_DIGIT_ANY);*/
						res = ast_waitfordigit(chan, 1000);
						if (res)
							break;

						time(&current);
						if ((current - start) >= abs_timeout) {
							ast_log(LOG_NOTICE, "Absolute timeout reached: %d seconds\n", (int)(current - start));
							rtimeout = 1;
							break; 
						}
					}
				} else
					res = ast_waitstream(chan, AST_DIGIT_ANY);
			}
		}

		/* Wait for audio stream to finish */
		res = ast_waitstream(chan, "");
		if (res) {
			ast_log(LOG_WARNING, "ast_waitstream failed on %s\n", chan->name);
			if (vox_recstr_close(channel_handle, write_handle) == -1) {
				ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
			}
			if (option_interrupt) {
				if (vox_vsd_close(channel_handle, vad_handle) == -1) {
					ast_log(LOG_ERROR, "vox_vsd_close(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
				} else {
					if (cfg_verbose)
						ast_log(LOG_NOTICE, "vox_vsd_close(%d) succeeded on %s\n", channel_handle, chan->name);
				}
			}
			if (cfg_keep_recorded_files)
				ast_closestream(s);
			ast_config_destroy(cfg);
			return -1;	
		}
		
		if (option_beep) {
			if (!ast_streamfile(chan, "beep", chan->language))
				ast_waitstream(chan, "");
			else 
				ast_log(LOG_WARNING, "ast_streamfile failed on %s\n", chan->name);
		}
		ast_stopstream(chan);
	}

	/* Recognition loop */
	int voice_detected = 0;
	time(&start);
	while (1) {
		time(&current);
		if ((current - start) >= abs_timeout) {
			ast_log(LOG_NOTICE, "Absolute timeout reached: %d seconds\n", (int)(current - start));
			break;
		}
		if (option_bargein)
			res = ast_sched_wait(chan->sched);
		else
			res = -1;
		waitres = ast_waitfor(chan, MAX_WAIT_TIME);
		if (waitres < 0) {
			ast_log(LOG_NOTICE, "Wait failed\n");
			if (vox_recstr_close(channel_handle, write_handle) == -1) {
				ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
			}
			if (option_interrupt) {
				if (vox_vsd_close(channel_handle, vad_handle) == -1) {
					ast_log(LOG_ERROR, "vox_vsd_close(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
				} else {
					if (cfg_verbose)
						ast_log(LOG_NOTICE, "vox_vsd_close(%d) succeeded on %s\n", channel_handle, chan->name);
				}
			}
			if (cfg_keep_recorded_files)
				ast_closestream(s);
			ast_stopstream(chan);
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
			return -1;
		} else if (waitres == 0) {
			ast_log(LOG_NOTICE, "Wait timeout!\n");
			continue;
		}

		/* Read a frame */
		f = ast_read(chan);
		if (!f) {
			ast_log(LOG_NOTICE, "Hangup detected on %s\n", chan->name);
			if (vox_recstr_close(channel_handle, write_handle) == -1) {
				ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
			}
			if (option_interrupt) {
				if (vox_vsd_close(channel_handle, vad_handle) == -1) {
					ast_log(LOG_ERROR, "vox_vsd_close(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
				} else {
					if (cfg_verbose)
						ast_log(LOG_NOTICE, "vox_vsd_close(%d) succeeded on %s\n", channel_handle, chan->name);
				}
			}
			if (cfg_keep_recorded_files)
				ast_closestream(s);
			ast_stopstream(chan);
			#ifdef VERBIO_CLOSE_ON_HANGUP
			if (vox_devclose(channel_handle) == -1) {
				ast_log(LOG_ERROR, "vox_devclose(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_devclose(%d) succeeded on %s\n", channel_handle, chan->name);
			}
			#endif
			ast_module_user_remove(u);
			ast_config_destroy(cfg);
 			return -1;
		}

		if (f->frametype == AST_FRAME_VOICE) {
			/* Write frame to file */
			if (cfg_keep_recorded_files) {
				if (ast_writestream(s, f))
					ast_log(LOG_WARNING, "Error writing frame to %s on %s\n", rec_file_path, chan->name);
			}
			/* Pass our frame to recognizer */
			if (vox_recstr_write(write_handle, f->data.ptr, f->samples) < f->samples) {
				ast_frfree(f);
				break;
			}
			if (option_interrupt && voice_detected != 1) {
				int status = 0;
				status = vox_vsd_write(vad_handle, f->data.ptr, f->samples);
				if ((status != VVX_INIT) && (status != VVX_SILENCE)) {
					voice_detected = 1;
					ast_stopstream(chan);
					if (cfg_verbose)
						ast_log(LOG_NOTICE, "Voice detected\n");
				}
			}
		} else if( f->frametype == AST_FRAME_DTMF) {
			if (option_dtmf) {
				/* Set detected dtmf to dialplan var */
				verbio_set_dtmf_result(chan, f->subclass.integer);
				int dtmflen = strlen(pbx_builtin_getvar_helper(chan, "VDTMF_RESULT"));
				if ((dtmflen >= max_dtmf_len) || f->subclass.integer == dtmf_terminator) {
					ast_log(LOG_NOTICE, "dtmf max len or terminator detected on %s\n", chan->name);
					ast_frfree(f);
					break; 
				}
			}
		}
		ast_frfree(f);
		
		if (option_bargein)
			ast_sched_runq(chan->sched);
	}

	if (cfg_keep_recorded_files)
		ast_closestream(s);
	
	/* Get and set (as chan vars) asr result */
	verbio_set_asr_result(chan, channel_handle, cfg_verbose);

	/* Cleanup */
	if (vox_recstr_close(channel_handle, write_handle) == -1) {
		ast_log(LOG_ERROR, "vox_recstr_close(%d, %zd) failed on %s: %s\n", channel_handle, write_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_recstr_close(%d, %zd) succeeded on %s\n", channel_handle, write_handle, chan->name);
	}
	if (option_interrupt) {
		if (vox_vsd_close(channel_handle, vad_handle) == -1) {
			ast_log(LOG_ERROR, "vox_vsd_close(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
		} else {
			if (cfg_verbose)
				ast_log(LOG_NOTICE, "vox_vsd_close(%d) succeeded on %s\n", channel_handle, chan->name);
		}
	}
	
	ast_stopstream(chan);
	ast_module_user_remove(u);
	ast_config_destroy(cfg);
	return 0;
}

/*! \brief Load vocabulary application. This app must be called prior to any recognition. */
static int verbio_load_vcb(struct ast_channel *chan, DATA_TYPE data)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log( LOG_ERROR, "Voxserver is down\n");
		ast_mutex_unlock(&voxdownlock);
		return 0;
	}
	ast_mutex_unlock(&voxdownlock);
	
	struct ast_module_user *u;
	char *args = NULL;
	char *argv[3];
	int argc = 0;
	char *options = NULL;
	char *opt = NULL;
	/* Input options */
	char *gram_file = NULL;
	char *gram_type = NULL;
	char *option_config = NULL;
	char *option_language = NULL;
	int option_no_hangup_on_verbio_err = 0;
	/* Config file options */
	struct ast_config *cfg;
	const char *cfg_primary_server = NULL;
	const char *cfg_backup_server = NULL;
	const char *cfg_config = NULL;
	const char *cfg_language = NULL;
	const char *cfg_grammar_path = NULL;
	int cfg_verbose = 0;
	const char *tmp = NULL;
	/* Vars to be used when dealing with voxlib */
	const char *config = NULL;
	const char *language = NULL;
	int channel_handle;
	int vocabulary_handle;
	char gram_full_path[MAXPATHLEN];
	int line;
	int setmode = GVX_ISOLATED;/*GVX_ABNF/GVX_ISOLATED/GVX_CONNECTED*/
	int builtin_grammar = 0;
	int vvi_grammar = 0;
	/* Misc vars */
	char gram_cache_full_path[MAXPATHLEN];
	int res;

	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "%s requires an argument (gram_file|gram_type[|config][|language][|options])\n", verbio_load_vcb_app);
		return -1;
	}

	u = ast_module_user_add(chan);

	/* We need to make a copy of the input string if we are going to modify it! */
	args = ast_strdupa(data);
	if (!args) {
		ast_log(LOG_ERROR, "Out of memory!\n");
		ast_module_user_remove(u);
		return -1;
	}
	
	if ((argc = ast_app_separate_args(args, args_sep, argv, sizeof(argv) / sizeof(argv[0])))) {
		gram_file = argv[0];
		gram_type = argv[1];
		options = argv[2];
	}

	if (ast_strlen_zero(gram_file) && ast_strlen_zero(gram_type)) {
		ast_log(LOG_ERROR, "Check input parameters\n");
		ast_module_user_remove(u);
		return -1;
	}

	/* Get options from user */
	/* config */
	option_config = strsep(&options, str_sep);
	if (option_config) {
		if (!strcmp(option_config, ""))
			option_config = NULL;
	}
	/* language */
	option_language = strsep(&options, str_sep);
	if (option_language) {
		if (!strcmp(option_language, ""))
			option_language = NULL;
	}
	/* options */
	opt = strsep(&options, str_sep);
	if (opt) {
		if (strchr(opt, 'n'))
			option_no_hangup_on_verbio_err = 1;
	}
	
	/* Get options from Verbio config file */
	cfg = verbio_load_asterisk_config();
	if (!cfg) {
		ast_log(LOG_WARNING, "Error opening configuration file %s\n", VERBIO_CFG);
		ast_module_user_remove(u);
		return -1;
	}

	if (!(cfg_primary_server = ast_variable_retrieve(cfg, "general", "primary_server")))
		cfg_primary_server = VERBIO_SERVER;
	if (!(cfg_backup_server = ast_variable_retrieve(cfg, "general", "backup_server")))
		cfg_backup_server = VERBIO_SERVER;
	if (!(cfg_config = ast_variable_retrieve(cfg, "asr", "default_config")))
		ast_log(LOG_WARNING, "Error reading default_config option\n");
	if (!(cfg_language = ast_variable_retrieve(cfg, "asr", "default_language")))
		ast_log(LOG_WARNING, "Error reading default_language option\n");
	if (!(cfg_grammar_path = ast_variable_retrieve(cfg, "asr", "grammar_path")))
		cfg_grammar_path = VERBIO_GRAMMAR_PATH;
	if ((tmp = ast_variable_retrieve(cfg, "debug", "verbose")))
		cfg_verbose = atoi(tmp);

	/* Set options (to be used on voxlib calls) */
	/* config */
	if (!option_config)
		config = cfg_config;
	else
		config = option_config;
	/* language */
	if (!option_language)
		language = cfg_language;
	else
		language = option_language;
	/* gram_type */
	if (toupper(gram_type[0]) == 'I')
		/* Isolated */
		setmode = GVX_ISOLATED;
	else if (toupper(gram_type[0]) == 'C')
		/* Connected */
		setmode = GVX_CONNECTED;
	else if (toupper(gram_type[0]) == 'A')
		/* ABNF */
		setmode = GVX_ABNF;
	else if (toupper(gram_type[0]) == 'B') {
		/* Builtin */
		setmode = GVX_ABNF;
		builtin_grammar = 1;
	} else if (toupper(gram_type[0]) == 'V') {
		/* vvi */
		setmode = GVX_VVI;
		vvi_grammar = 1;
	} else
		ast_log(LOG_WARNING, " Unknown grammar type : %s. Using Isolated mode\n", gram_type);
	/* gram_file */
	if ((gram_file[0] != '/') && (!builtin_grammar) && (!vvi_grammar)) {
		strcpy(gram_full_path, cfg_grammar_path);
		strcat(gram_full_path, "/");
		strcat(gram_full_path, gram_file);
	} else {
		strcpy(gram_full_path, gram_file);
	}

	channel_handle = verbio_get_channel_handle(chan);	

	if (cfg_verbose) {
		ast_log(LOG_NOTICE, "----------------------------\n");
		ast_log(LOG_NOTICE, "VerbioLoadVcb param summary:\n");
		ast_log(LOG_NOTICE, " Channel        : %s\n", chan->name);
		ast_log(LOG_NOTICE, " Primary server : %s\n", cfg_primary_server);
		ast_log(LOG_NOTICE, " Backup server  : %s\n", cfg_backup_server);
		ast_log(LOG_NOTICE, " Configuration  : %s\n", config);
		ast_log(LOG_NOTICE, " Language       : %s\n", language);
		ast_log(LOG_NOTICE, " Grammar file   : %s\n", gram_full_path);
		if (setmode == GVX_ISOLATED)
			ast_log(LOG_NOTICE, " Grammar type   : ISOLATED\n");
		else if (setmode == GVX_CONNECTED)
			ast_log(LOG_NOTICE, " Grammar type   : CONNECTED\n");
		else if (setmode == GVX_ABNF) {
			if (builtin_grammar)
				ast_log(LOG_NOTICE, " Grammar type   : ABNF/BUILT-IN\n");
			else
				ast_log(LOG_NOTICE, " Grammar type   : ABNF\n");
		} else if (setmode == GVX_VVI)
			ast_log(LOG_NOTICE, " Grammar type   : VVI\n");
		else
			ast_log(LOG_NOTICE, " Grammar type   : UNKNOWN\n");
		ast_log(LOG_NOTICE, " Channel handle : %d\n", channel_handle);
		ast_log(LOG_NOTICE, "----------------------------\n");
		ast_log(LOG_NOTICE, "\n");
	}

	/* If we can not set asr config, exit */
	if (vox_setparm(channel_handle, VXCH_DEFASRCFG, config) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_DEFASRCFG, %s) succeeded on %s: %s\n", channel_handle, config, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;	
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_DEFASRCFG, %s) succeeded on %s\n", channel_handle, config, chan->name);
	}
	/* If we can not set asr language, exit */
	if (vox_setparm(channel_handle, VXCH_DEFASRLANG, language) == -1) {
		ast_log(LOG_ERROR, "vox_setparm(%d, VXCH_DEFASRLANG, %s) succeeded on %s: %s\n", channel_handle, language, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_setparm(%d, VXCH_DEFASRLANG, %s) succeeded on %s\n", channel_handle, language, chan->name);
	}

	if (builtin_grammar || vvi_grammar) {
		strcpy(gram_cache_full_path, gram_full_path);
	} else {
		/* Check if grammar is available in cache */
		res = verbio_md5_grammar_exists(gram_full_path, gram_cache_full_path, language, cfg_verbose);
		if (res == -1) {
			return -1;
		} else if (res == 0) {
			if (cfg_verbose > 0)
				ast_log(LOG_NOTICE, "Preparing grammar %s. Creating cache file %s\n", gram_full_path, gram_cache_full_path);
			/* Prepare vocabulary */
			if (vox_prevcbdev(channel_handle, gram_cache_full_path, setmode, &line, language) == -1) {
				ast_log(LOG_ERROR, "vox_prevcbdev(%d, %s, %d, %s) failed on %s: %s, line: %d\n", channel_handle, gram_cache_full_path, setmode, language, chan->name, ATVOX_ERRMSGP(channel_handle), line);
				ast_module_user_remove(u);
				ast_config_destroy(cfg);
				if (option_no_hangup_on_verbio_err)
					return 0;
				else
					return -1;	
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_prevcbdev(%d, %s, %d, %s) succeeded on %s\n", channel_handle, gram_cache_full_path, setmode, language, chan->name);
			}
		}
	}
	
	/* Load vocabulary */
	if ((vocabulary_handle = vox_loadvcb(channel_handle, gram_cache_full_path, setmode)) == -1) {
		ast_log(LOG_ERROR, "vox_loadvcb(%d, %s, %d) failed on %s: %s\n", channel_handle, gram_cache_full_path, setmode, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;	
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_loadvcb(%d, %s, %d) succeeded on %s\n", channel_handle, gram_cache_full_path, setmode, chan->name);
	}
	
	/* Activate vocabulary */
	if (vox_activatevcb(channel_handle, vocabulary_handle, setmode) == -1) {
		ast_log(LOG_ERROR, "vox_activatevcb(%d, %d, %d) failed on %s: %s\n", channel_handle, vocabulary_handle, setmode, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_activatevcb(%d, %d, %d) succeeded on %s\n", channel_handle, vocabulary_handle, setmode, chan->name);
	}

	char tmpbuff[VERBIO_MAX_INT_SIZE];
	if (snprintf(tmpbuff, VERBIO_MAX_INT_SIZE, "%d", vocabulary_handle) > VERBIO_MAX_INT_SIZE)
		ast_log(LOG_WARNING, "VERBIO_VOCABULARY_HANDLE may be truncated\n");
	pbx_builtin_setvar_helper(chan, "VERBIO_VOCABULARY_HANDLE", tmpbuff);

	ast_module_user_remove(u);
	ast_config_destroy(cfg);
	return 0;
}

/*! \brief Unload vocabulary application. */
static int verbio_unload_vcb(struct ast_channel *chan, DATA_TYPE data)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log( LOG_ERROR, "Voxserver is down\n");
		ast_mutex_unlock(&voxdownlock);
		return 0;
	}
	ast_mutex_unlock(&voxdownlock);
	
	struct ast_module_user *u;
	char *args = NULL;
	char *argv[2];
	int argc = 0;
	char *options = NULL;
	char *opt = NULL;
	/* Input options */
	char *option_config = NULL;
	char *option_language = NULL;
	int option_no_hangup_on_verbio_err = 0;
	/* Config file options */
	struct ast_config *cfg;
	const char *cfg_primary_server = NULL;
	const char *cfg_backup_server = NULL;
	const char *cfg_config = NULL;
	const char *cfg_language = NULL;
	int cfg_verbose = 0;
	const char *tmp = NULL;
	/* Vars to be used when dealing with voxlib */
	const char *config = NULL;
	const char *language = NULL;
	int channel_handle;
	int vocabulary_handle;

	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "%s requires an argument (vocabulary_handle[|config][|language][|options])\n", verbio_unload_vcb_app);
		return -1;
	}

	u = ast_module_user_add(chan);

	/* We need to make a copy of the input string if we are going to modify it! */
	args = ast_strdupa(data);
	if (!args) {
		ast_log(LOG_ERROR, "Out of memory!\n");
		ast_module_user_remove(u);
		return -1;
	}
	
	if ((argc = ast_app_separate_args(args, args_sep, argv, sizeof(argv) / sizeof(argv[0])))) {
		tmp = argv[0];
		options = argv[1];
	}

 	if (!ast_strlen_zero(tmp)) {
		vocabulary_handle = atoi(tmp);
	} else {
		ast_log(LOG_ERROR, "Check input parameters\n");
		ast_module_user_remove(u);
		return -1;
	}

	/* Get options from user */
	/* config */
	option_config = strsep(&options, str_sep);
	if (option_config) {
		if (!strcmp(option_config, ""))
			option_config = NULL;
	}
	/* language */
	option_language = strsep(&options, str_sep);
	if (option_language) {
		if (!strcmp(option_language, ""))
			option_language = NULL;
	}
	/* options */
	opt = strsep(&options, str_sep);
	if (opt) {
		if (strchr(opt, 'n'))
			option_no_hangup_on_verbio_err = 1;
	}

	/* Get options from Verbio config file */
	cfg = verbio_load_asterisk_config();
	if (!cfg) {
		ast_log(LOG_WARNING, "Error opening configuration file %s\n", VERBIO_CFG);
		ast_module_user_remove(u);
		return -1;
	}

	if (!(cfg_primary_server = ast_variable_retrieve(cfg, "general", "primary_server")))
		cfg_primary_server = VERBIO_SERVER;
	if (!(cfg_backup_server = ast_variable_retrieve(cfg, "general", "backup_server")))
		cfg_backup_server = VERBIO_SERVER;
	if (!(cfg_config = ast_variable_retrieve(cfg, "asr", "default_config")))
		ast_log(LOG_WARNING, "Error reading default_config option\n");
	if (!(cfg_language = ast_variable_retrieve(cfg, "asr", "default_language")))
		ast_log(LOG_WARNING, "Error reading default_language option\n");
	if ((tmp = ast_variable_retrieve(cfg, "debug", "verbose")))
		cfg_verbose = atoi(tmp);

	/* Set options (to be used on voxlib calls) */
	/* config */
	if (!option_config)
		config = cfg_config;
	else
		config = option_config;
	/* language */
	if (!option_language)
		language = cfg_language;
	else
		language = option_language;

	channel_handle = verbio_get_channel_handle(chan);	

	if ((tmp = pbx_builtin_getvar_helper(chan, "VERBIO_VOCABULARY_HANDLE")) && !ast_strlen_zero(tmp)) {
		vocabulary_handle = atoi(tmp);
	}

	if (cfg_verbose) {
		ast_log(LOG_NOTICE, "------------------------------\n");
		ast_log(LOG_NOTICE, "VerbioUnloadVcb param summary:\n");
		ast_log(LOG_NOTICE, " Channel        : %s\n", chan->name);
		ast_log(LOG_NOTICE, " Primary server : %s\n", cfg_primary_server);
		ast_log(LOG_NOTICE, " Backup server  : %s\n", cfg_backup_server);
		ast_log(LOG_NOTICE, " Configuration  : %s\n", config);
		ast_log(LOG_NOTICE, " Language       : %s\n", language);
		ast_log(LOG_NOTICE, " Channel handle    : %d\n", channel_handle);
		ast_log(LOG_NOTICE, " Vocabulary handle : %d\n", vocabulary_handle);
		ast_log(LOG_NOTICE, "------------------------------\n");
		ast_log(LOG_NOTICE, "\n");
	}

	/* unload vocabulary */
	if (vox_unloadvcb(channel_handle, vocabulary_handle, 0) == -1) {
		ast_log(LOG_ERROR, "vox_unloadvcb(%d, %d) failed on %s: %s\n", channel_handle, vocabulary_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
		ast_module_user_remove(u);
		ast_config_destroy(cfg);
		if (option_no_hangup_on_verbio_err)
			return 0;
		else
			return -1;	
	} else {
		if (cfg_verbose)
			ast_log(LOG_NOTICE, "vox_unloadvcb(%d, %d) succeeded on %s\n", channel_handle, vocabulary_handle, chan->name);
	}

	ast_module_user_remove(u);
	ast_config_destroy(cfg);
	return 0;
}

/*! \brief Save last verbio error to channel var. */
static int verbio_last_err(struct ast_channel *chan, DATA_TYPE data)
{
	struct ast_module_user *u;
	char *args = NULL;
	char *argv[1];
	int argc = 0;
	/* Input options */
	char *var = NULL;
	
	if (ast_strlen_zero(data)) {
		ast_log(LOG_ERROR, "%s requires an argument (var)\n", verbio_last_err_app);
		return -1;
	}

	u = ast_module_user_add(chan);

	/* We need to make a copy of the input string if we are going to modify it! */
	args = ast_strdupa(data);
	if (!args) {
		ast_log(LOG_ERROR, "Out of memory!\n");
		ast_module_user_remove(u);
		return -1;
	}

	if ((argc = ast_app_separate_args(args, args_sep, argv, sizeof(argv) / sizeof(argv[0]))))
		var = argv[0];

	verbio_set_err(chan, verbio_get_channel_handle(chan), var);

	ast_module_user_remove(u);

	return 0;
}

/*! \brief Close verbio voxlib device. */
static int verbio_free_channel(struct ast_channel *chan, DATA_TYPE data)
{
	ast_mutex_lock(&voxdownlock);
	if (voxserver_is_down) {
		ast_log( LOG_ERROR, "Voxserver is down\n");
		ast_mutex_unlock(&voxdownlock);
		return 0;
	}
	ast_mutex_unlock(&voxdownlock);
	
	struct ast_module_user *u;
	char *args = NULL;
	char *argv[1];
	int argc = 0;
	/* Input options */
	int	option_channel_handle = 0;
	/* Config file options */
	struct ast_config *cfg;
	int cfg_verbose = 0;
	const char *tmp = NULL;
	/* Vars to be used when dealing with voxlib */
	int channel_handle;
	/* Misc vars */
	int i = 0;

	u = ast_module_user_add(chan);
	
	if (!ast_strlen_zero(data)) {
		/* We need to make a copy of the input string if we are going to modify it! */
		args = ast_strdupa(data);	
		if (!args) {
			ast_log(LOG_ERROR, "Out of memory!\n");
			ast_module_user_remove(u);
			return -1;
		}

		if ((argc = ast_app_separate_args(args, args_sep, argv, sizeof(argv) / sizeof(argv[0]))))
			tmp = argv[0];

		if (!ast_strlen_zero(tmp))
			option_channel_handle = atoi(tmp);
	}

	/* Get options from Verbio config file */
	cfg = verbio_load_asterisk_config();
	if (!cfg) {
		ast_log(LOG_WARNING, "Error opening configuration file %s\n", VERBIO_CFG);
		ast_module_user_remove(u);
		return -1;
	}

	if ((tmp = ast_variable_retrieve(cfg, "debug", "verbose")))
		cfg_verbose = atoi(tmp);
	
	channel_handle = verbio_get_channel_handle(chan);	

	if (cfg_verbose) {
		ast_log(LOG_NOTICE, "--------------------------------\n");
		ast_log(LOG_NOTICE, "VerbioFreeChannel param summary:\n");
		ast_log(LOG_NOTICE, " Channel        : %s\n", chan->name);
		ast_log(LOG_NOTICE, " Channel handle : %d\n", channel_handle);
		ast_log(LOG_NOTICE, "--------------------------------\n");
		ast_log(LOG_NOTICE, "\n");
	}

	if (option_channel_handle == -1) {
		/* Free all possible channel handles */
		ast_log(LOG_WARNING, "Closing all channel handles! (may be slow)\n");
		for (i = 0; i <= channel_handle; ++i) {
			if (vox_devclose(i) == -1) {
				ast_log(LOG_ERROR, "vox_devclose(%d) failed on %s: %s\n", i, chan->name, ATVOX_ERRMSGP(channel_handle));
			} else {
				if (cfg_verbose)
					ast_log(LOG_NOTICE, "vox_devclose(%d) succeeded on %s\n", i, chan->name);
			}
		}
	} else {
		if (vox_devclose(channel_handle) == -1) {
			ast_log(LOG_ERROR, "vox_devclose(%d) failed on %s: %s\n", channel_handle, chan->name, ATVOX_ERRMSGP(channel_handle));
		} else {
			if (cfg_verbose)
				ast_log(LOG_NOTICE, "vox_devclose(%d) succeeded on %s\n", channel_handle, chan->name);
		}
	}

	ast_module_user_remove(u);
	ast_config_destroy(cfg);
	return 0;
}

static int unload_module(void)
{
	int res = 0;
	
	ast_mutex_lock(&unloadingmodlock);
	unloading_module = 1;
	ast_mutex_unlock(&unloadingmodlock);
	
	while (1) {
		sleep(1);
		ast_mutex_lock(&reconnectlock);
		if (!trying_to_reconnect) {
			ast_mutex_unlock(&reconnectlock);
			break;
		}
		ast_mutex_unlock(&reconnectlock);
	}
	
	/* Close connection with server */
	close_verbio();
	
	res = ast_unregister_application(verbio_load_vcb_app);
	res |= ast_unregister_application(verbio_unload_vcb_app);
	res |= ast_unregister_application(verbio_rec_app);
	res |= ast_unregister_application(verbio_stream_and_rec_app);
	res |= ast_unregister_application(verbio_prompt_app);
	res |= ast_unregister_application(verbio_last_err_app);
	res |= ast_unregister_application(verbio_free_channel_app);

	ast_mutex_destroy(&voxdownlock);
	ast_mutex_destroy(&reconnectlock);
	ast_mutex_destroy(&unloadingmodlock);
	
	return res;	
}

static int load_module(void)
{
	int res = 0;

	ast_mutex_init(&voxdownlock);
	ast_mutex_init(&reconnectlock);
	ast_mutex_init(&unloadingmodlock);
	
/*
	init_verbio()
		-1 - Something went wrong
		0 - ASR and TTS
		1 - ASR
		2 - TTS
*/
	int avres = init_verbio();

#if 0
	if (avres != -1) { 
		/* Now we are sure that the server is up again reconnect in order to avoid partial connection */
		close_verbio();
		avres = init_verbio();
	}
#endif

	if (avres != -1) {
		/* Load applications that are not ASR/TTS dependent */
		res |= ast_register_application(verbio_last_err_app, verbio_last_err, "Get Verbio last error", verbio_last_err_descrip);
		res |= ast_register_application(verbio_free_channel_app, verbio_free_channel, "Free verbio resources", verbio_free_channel_descrip);

		if (avres == 0) {
			/* ASR and TTS */
			/* Register ASR and TTS related apps */
			res |= ast_register_application(verbio_load_vcb_app, verbio_load_vcb, "Load vocabulary", verbio_load_vcb_descrip);
			res |= ast_register_application(verbio_unload_vcb_app, verbio_unload_vcb, "Unload vocabulary", verbio_unload_vcb_descrip);
			res |= ast_register_application(verbio_rec_app, verbio_rec, "Recognize application", verbio_rec_descrip);
			res |= ast_register_application(verbio_stream_and_rec_app, verbio_stream_and_rec, "Stream and recognize application", verbio_stream_and_rec_descrip);
			res |= ast_register_application(verbio_prompt_app, verbio_prompt, "Text to speech application", verbio_prompt_descrip);
		} else if (avres == 1) {
			/* Only ASR */
			/* Register ASR related apps */
			res |= ast_register_application(verbio_load_vcb_app, verbio_load_vcb, "Load vocabulary", verbio_load_vcb_descrip);
			res |= ast_register_application(verbio_unload_vcb_app, verbio_unload_vcb, "Unload vocabulary", verbio_unload_vcb_descrip);
			res |= ast_register_application(verbio_rec_app, verbio_rec, "Recognize application", verbio_rec_descrip);
			res |= ast_register_application(verbio_stream_and_rec_app, verbio_stream_and_rec, "Stream and recognize application", verbio_stream_and_rec_descrip);
		} else if (avres == 2) {
			/* Only TTS */
			/* Register TTS related apps */
			res |= ast_register_application( verbio_prompt_app, verbio_prompt, "Text to speech application", verbio_prompt_descrip);
		}
	} else {
		ast_log(LOG_ERROR, "Error loading verbio. Please check if server is up and running\n");
		return AST_MODULE_LOAD_DECLINE;
	}

//DELETED: 2017-03-28
//	return res;
//DELETED: 2017-03-28
//ADDED: 2017-03-28
	return res ? AST_MODULE_LOAD_DECLINE : 0;
//ADDED: 2017-03-28
}
 
AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "Verbio Speech Technologies Applications",
		.load = load_module,
		.unload = unload_module,
		);