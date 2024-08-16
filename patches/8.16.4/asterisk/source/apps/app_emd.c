/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2003 - 2006, Aheeva Technology.
 *
 * Claude Klimos (claude.klimos@aheeva.com)
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
 *
 * A license has been granted to Digium (via disclaimer) for the use of
 * this code.
 */

/*! \file
 *
 * \brief Early media detection
 *
 * \author Claude Klimos (claude.klimos@aheeva.com)
 */

/*** MODULEINFO
	<support_level>extended</support_level>
	<defaultenabled>no</defaultenabled>
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 363374 $")

#include "asterisk/module.h"
#include "asterisk/lock.h"
#include "asterisk/channel.h"
#include "asterisk/dsp.h"
#include "asterisk/pbx.h"
#include "asterisk/config.h"
#include "asterisk/app.h"

/*** DOCUMENTATION
	<application name="EMD" language="en_US">
		<synopsis>
			Attempt to detect early media.
		</synopsis>
		<syntax>
			<parameter name="totalAnalysis Time" required="false">
				<para>Is the maximum time allowed for the algorithm</para>
				<para>to decide HUMAN or MACHINE</para>
			</parameter>
			<parameter name="silenceThreshold" required="false">
				<para>How long do we consider silence</para>
			</parameter>
		</syntax>
		<description>
			<para>This application attempts to detect early media besides ringing.</para>
			<variablelist>
				<variable name="EMDSTATUS">
					<para>This is the status of the early media detection</para>
					<value name="ANSWER" />
					<value name="HANGUP" />
					<value name="TIMEOUT" />
					<value name="EARLYMEDIA" />
				</variable>
			</variablelist>
		</description>
		<see-also>
			<ref type="application">AMD</ref>
		</see-also>
	</application>

 ***/

static const char app[] = "EMD";

/* Some default values for the algorithm parameters. These defaults will be overwritten from emd.conf */
static int dfltMaximumNumberOfRinging    = 1;
static int dfltMaximumShortMessageLength = 500;
static int dfltMaximumLongMessageLength  = 1000;
static int dfltTotalAnalysisTime         = 30000;
static int dfltSilenceThreshold          = 256;

/* Set to the lowest ms value provided in emd.conf or application parameters */
static int dfltMaxWaitTimeForFrame  = 50;

static void isEarlyMedia(struct ast_channel *chan, const char *data)
{
	int res = 0;
	struct ast_frame *f = NULL;
	struct ast_frame *f2 = NULL;
	struct ast_dsp *dsp = NULL;
	int readFormat, framelength = 0;
	int voiceDuration = 0;
	int iTotalTime = 0;
	int iRingingCount = 0;
	char emdCause[256] = "", emdStatus[256] = "";
	char *parse = ast_strdupa(data);

	if (chan->_state == AST_STATE_UP) {
		ast_log(LOG_WARNING, "EMD: Channel: %s is already answered\n", chan->name);
		pbx_builtin_setvar_helper(chan, "EMDSTATUS", "ANSWER");
		pbx_builtin_setvar_helper(chan, "EMDCAUSE", "");
		return;
	}

	/* Lets set the initial values of the variables that will control the algorithm.
	   The initial values are the default ones. If they are passed as arguments
	   when invoking the application, then the default values will be overwritten
	   by the ones passed as parameters. */
	int maximumNumberOfRinging      = dfltMaximumNumberOfRinging;
	int maximumShortMessageLength   = dfltMaximumShortMessageLength;
	int maximumLongMessageLength    = dfltMaximumLongMessageLength;
	int totalAnalysisTime           = dfltTotalAnalysisTime;
	int silenceThreshold            = dfltSilenceThreshold;
	int maxWaitTimeForFrame         = dfltMaxWaitTimeForFrame;

	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(argMaximumNumberOfRinging);
		AST_APP_ARG(argMaximumShortMessageLength);
		AST_APP_ARG(argMaximumLongMessageLength);
		AST_APP_ARG(argTotalAnalysisTime);
		AST_APP_ARG(argSilenceThreshold);
		AST_APP_ARG(argMaximumWordLength);
	);

	ast_verb(3, "EMD: Channel: %s, Format: %s\n", chan->name, ast_getformatname(chan->readformat));

	/* Lets parse the arguments. */
	if (!ast_strlen_zero(parse)) {
		/* Some arguments have been passed. Lets parse them and overwrite the defaults. */
		AST_STANDARD_APP_ARGS(args, parse);
		if (!ast_strlen_zero(args.argMaximumNumberOfRinging))
			maximumNumberOfRinging = atoi(args.argMaximumNumberOfRinging);
		if (!ast_strlen_zero(args.argMaximumShortMessageLength))
			maximumShortMessageLength = atoi(args.argMaximumShortMessageLength);
		if (!ast_strlen_zero(args.argMaximumLongMessageLength))
			maximumLongMessageLength = atoi(args.argMaximumLongMessageLength);
		if (!ast_strlen_zero(args.argTotalAnalysisTime))
			totalAnalysisTime = atoi(args.argTotalAnalysisTime);
		if (!ast_strlen_zero(args.argSilenceThreshold))
			silenceThreshold = atoi(args.argSilenceThreshold);
	} else {
		ast_debug(1, "EMD using the default parameters\n");
	}

	/* Find lowest ms value, that will be max wait time for a frame */
	if (maxWaitTimeForFrame > maximumShortMessageLength)
		maxWaitTimeForFrame = maximumShortMessageLength;
	if (maxWaitTimeForFrame > maximumLongMessageLength)
		maxWaitTimeForFrame = maximumLongMessageLength;

	/* Now we're ready to roll! */
	ast_verb(3, "EMD: Channel: %s. TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d, SilenceThreshold: %d\n",
		chan->name, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength, silenceThreshold);

	/* Set read format to signed linear so we get signed linear frames in */
	readFormat = chan->readformat;
	if (ast_set_read_format(chan, AST_FORMAT_SLINEAR) < 0) {
		ast_log(LOG_WARNING, "EMD: Channel: %s, unable to set to linear mode\n", chan->name);
		pbx_builtin_setvar_helper(chan, "EMDSTATUS", "");
		pbx_builtin_setvar_helper(chan, "EMDCAUSE", "");
		return;
	}

	/* Create a new DSP that will detect the silence */
	if (!(dsp = ast_dsp_new())) {
		ast_log(LOG_WARNING, "EMD: Channel: %s, unable to create early media detector\n", chan->name);
		pbx_builtin_setvar_helper(chan, "EMDSTATUS", "");
		pbx_builtin_setvar_helper(chan, "EMDCAUSE", "");
		return;
	}

	ast_dsp_set_features(dsp, DSP_PROGRESS_RINGING | DSP_PROGRESS_EARLY_MEDIA);

	/* Set silence threshold to specified value */
	ast_dsp_set_threshold(dsp, silenceThreshold);

	/* Now we go into a loop waiting for frames from the channel */
	while ((res = ast_waitfor(chan, 2 * maxWaitTimeForFrame)) > -1) {
		/* If we fail to read in a frame, that means they hung up */
		if (!(f = ast_read(chan))) {
			ast_verb(3, "EMD: Channel: %s hungup. TotalTime: %d, RingingCount: %d, VoiceDuration: %d, TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d\n",
				chan->name, iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
			strcpy(emdStatus, "HANGUP");
			sprintf(emdCause, "%d-%d-%d-%d-%d-%d-%d", iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
			res = 1;
			break;
		}

		if (f->frametype == AST_FRAME_VOICE || f->frametype == AST_FRAME_NULL) {
			/* If the total time exceeds the analysis time then give up as we are not too sure */
			if (f->frametype == AST_FRAME_VOICE) {
				framelength = (ast_codec_get_samples(f) / DEFAULT_SAMPLES_PER_MS);
			} else {
				framelength = 2 * maxWaitTimeForFrame;
			}

			iTotalTime += framelength;
			if (iTotalTime >= totalAnalysisTime) {
				ast_verb(3, "EMD: Channel: %s, timeout reached. TotalTime: %d, RingingCount: %d, VoiceDuration: %d, TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d\n",
					chan->name, iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
				ast_frfree(f);
				strcpy(emdStatus, "TIMEOUT");
				sprintf(emdCause, "%d-%d-%d-%d-%d-%d-%d", iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
				break;
			}
		}

		f2 = ast_dsp_process(chan, dsp, f);
		if (!f2) {
			ast_log(LOG_ERROR, "DSP received is NULL (what happened?)\n");
			f2 = f;
		}
		if (f2->frametype == AST_FRAME_CONTROL) {
			if (f2->subclass.integer == AST_CONTROL_RINGING) {
				iRingingCount++;
				ast_verb(3, "EMD: Channel: %s is ringing. TotalTime: %d, RingingCount: %d, VoiceDuration: %d, TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d\n",
					chan->name, iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
			} else if (f2->subclass.integer == AST_CONTROL_ANSWER) {
				ast_verb(3, "EMD: Channel: %s answered. TotalTime: %d, RingingCount: %d, VoiceDuration: %d, TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d\n",
					chan->name, iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
				ast_frfree(f);
				strcpy(emdStatus, "ANSWER");
				sprintf(emdCause, "%d-%d-%d-%d-%d-%d-%d", iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
				res = 1;
				break;
			} else if (f2->subclass.integer == AST_CONTROL_EARLY_MEDIA) {
				voiceDuration += framelength;

				if (voiceDuration >= maximumLongMessageLength) {
					ast_verb(3, "EMD: Channel: %s, long message detected. TotalTime: %d, RingingCount: %d, VoiceDuration: %d, TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d\n",
						chan->name, iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
					ast_frfree(f);
					strcpy(emdStatus, "EARLYMEDIA");
					sprintf(emdCause, "LONGMESSAGE-%d-%d-%d-%d-%d-%d-%d", iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
					res = 1;
					break;
				} else if (voiceDuration >= maximumShortMessageLength && iRingingCount >= maximumNumberOfRinging) {
					ast_verb(3, "EMD: Channel: %s, short message w/ringing detected. TotalTime: %d, RingingCount: %d, VoiceDuration: %d, TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d\n",
						chan->name, iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
					ast_frfree(f);
					strcpy(emdStatus, "EARLYMEDIA");
					sprintf(emdCause, "SHORTMESSAGE-%d-%d-%d-%d-%d-%d-%d", iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
					res = 1;
					break;
				}
			}
		}
		ast_frfree(f);
	}
	
	if (!res) {
		/* It took too long to get a frame back. Giving up. */
		ast_verb(3, "EMD: Channel: %s, timeout reached. TotalTime: %d, RingingCount: %d, VoiceDuration: %d, TotalAnalysisTime: %d, MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d\n",
			chan->name, iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
		strcpy(emdStatus, "TIMEOUT");
		sprintf(emdCause, "%d-%d-%d-%d-%d-%d-%d", iTotalTime, iRingingCount, voiceDuration, totalAnalysisTime, maximumNumberOfRinging, maximumShortMessageLength, maximumLongMessageLength);
	}

	/* Set the status and cause on the channel */
	pbx_builtin_setvar_helper(chan, "EMDSTATUS", emdStatus);
	pbx_builtin_setvar_helper(chan, "EMDCAUSE", emdCause);

	/* Restore channel read format */
	if (readFormat && ast_set_read_format(chan, readFormat))
		ast_log(LOG_WARNING, "EMD: Unable to restore read format on '%s'\n", chan->name);

	/* Free the DSP used to detect early media */
	ast_dsp_free(dsp);

	return;
}

static int emd_exec(struct ast_channel *chan, const char *data)
{
	isEarlyMedia(chan, data);

	return 0;
}

static int load_config(int reload)
{
	struct ast_config *cfg = NULL;
	char *cat = NULL;
	struct ast_variable *var = NULL;
	struct ast_flags config_flags = { reload ? CONFIG_FLAG_FILEUNCHANGED : 0 };

	dfltSilenceThreshold = ast_dsp_get_threshold_from_settings(THRESHOLD_SILENCE);

	if (!(cfg = ast_config_load("emd.conf", config_flags))) {
		ast_log(LOG_ERROR, "Configuration file emd.conf missing\n");
		return -1;
	} else if (cfg == CONFIG_STATUS_FILEUNCHANGED) {
		return 0;
	} else if (cfg == CONFIG_STATUS_FILEINVALID) {
		ast_log(LOG_ERROR, "Config file emd.conf is in an invalid format. Aborting\n");
		return -1;
	}

	cat = ast_category_browse(cfg, NULL);

	while (cat) {
		if (!strcasecmp(cat, "general") ) {
			var = ast_variable_browse(cfg, cat);
			while (var) {
				if (!strcasecmp(var->name, "maximum_number_of_ringing")) {
					dfltMaximumNumberOfRinging = atoi(var->value);
				} else if (!strcasecmp(var->name, "maximum_short_message_length")) {
					dfltMaximumShortMessageLength = atoi(var->value);
				} else if (!strcasecmp(var->name, "maximum_long_message_length")) {
					dfltMaximumLongMessageLength = atoi(var->value);
				} else if (!strcasecmp(var->name, "silence_threshold")) {
					dfltSilenceThreshold = atoi(var->value);
				} else if (!strcasecmp(var->name, "total_analysis_time")) {
					dfltTotalAnalysisTime = atoi(var->value);
				} else {
					ast_log(LOG_WARNING, "%s: Cat:%s. Unknown keyword %s at line %d of emd.conf\n",
						app, cat, var->name, var->lineno);
				}
				var = var->next;
			}
		}
		cat = ast_category_browse(cfg, cat);
	}

	ast_config_destroy(cfg);

	ast_verb(3, "EMD defaults. MaximumNumberOfRinging: %d, MaximumShortMessageLength: %d, MaximumLongMessageLength: %d, TotalAnalysisTime: %d, SilenceThreshold: %d\n",
		dfltMaximumNumberOfRinging, dfltMaximumShortMessageLength, dfltMaximumLongMessageLength, dfltTotalAnalysisTime, dfltSilenceThreshold);

	return 0;
}

static int unload_module(void)
{
	return ast_unregister_application(app);
}

static int load_module(void)
{
	if (load_config(0))
		return AST_MODULE_LOAD_DECLINE;
	if (ast_register_application_xml(app, emd_exec))
		return AST_MODULE_LOAD_FAILURE;
	return AST_MODULE_LOAD_SUCCESS;
}

static int reload(void)
{
	if (load_config(1))
		return AST_MODULE_LOAD_DECLINE;
	return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "Early Media Detection Application",
		.load = load_module,
		.unload = unload_module,
		.reload = reload,
);