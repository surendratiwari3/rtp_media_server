#include "../../sr_module.h"
#include "../../parser/sdp/sdp_helpr_funcs.h"

#include <mediastreamer2/mediastream.h>
#include <ortp/ortp.h>

// https://www.kamailio.org/dokuwiki/doku.php/development:write-module
// http://www.kamailio.org/docs/kamailio-devel-guide/#c16makefile
//

MODULE_VERSION

/* Module */

static int mod_init(void);
static void mod_destroy(void);
static int child_init(int);

static int rtp_media_offer(struct sip_msg *, char *, char *);

static cmd_export_t cmds[] = {
	{"rtp_media_offer",(cmd_function)rtp_media_offer,0,0,0,ANY_ROUTE },
	{0, 0, 0, 0, 0, 0}
};

static pv_export_t mod_pvs[] = {
	{{0, 0}, 0, 0, 0, 0, 0, 0, 0}
};

static mi_export_t mi_cmds[] = {
	{0,0,0,0,0}
};

static param_export_t mod_params[]={
	{0,0,0}
};

static stat_export_t mod_stats[] = {
	{0,0,0}
};

struct module_exports exports = {
	"rtp_media_server",
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,
	mod_params,
	mod_stats,   /* exported statistics */
	mi_cmds,     /* exported MI functions */
	mod_pvs,     /* exported pseudo-variables */
	0,           /* extra processes */
	mod_init,
	0,           /* reply processing */
	mod_destroy, /* destroy function */
	child_init
};

/**
 * @return 0 to continue to load the OpenSER, -1 to stop the loading
 * and abort OpenSER.
 */
static int mod_init(void) {
	LM_INFO("RTP media server module init\n");
	return(0);
}

/**
 * Called only once when OpenSER is shuting down to clean up module
 * resources.
 */
static void mod_destroy() {
	LM_INFO("RTP media server module destroy\n");
	return;
}

/**
 * The rank will be o for the main process calling this function,
 * or 1 through n for each listener process. The rank can have a negative
 * value if it is a special process calling the child init function.
 * Other then the listeners, the rank will equal one of these values:
 * PROC_MAIN      0  Main ser process
 * PROC_TIMER    -1  Timer attendant process 
 * PROC_FIFO     -2  FIFO attendant process
 * PROC_TCP_MAIN -4  TCP main process
 * PROC_UNIXSOCK -5  Unix domain socket server processes
 *
 * If this function returns a nonzero value the loading of OpenSER will
 * stop.
 */
static int child_init(int rank) {
	int rtn = 0;
	return(rtn);
}


int rtp_media_offer(struct sip_msg* msg, char* param1, char* param2) {
	str body;
	int remote_ip_type=0;
	str remote_ip = str_init("255.255.255.255");
	char sdp_line[260];
	// sample/submodules/linphone/console/sipomatic.c
	//PayloadType *payload;
	//char *plabackfile;
	RtpProfile *profile = rtp_profile_new("remote");
	LM_INFO("rtp_profile created: %s", profile->name);
	body.s = get_body(msg);
	if(body.s == 0)
		return 0;
{
	str body_tmp = body;
	LM_INFO("SDP[%s]\n", body_tmp.s);
	extract_mediaip(&body_tmp, &remote_ip, &remote_ip_type, sdp_line);
	//LM_INFO("media IP[%s][%s]\n", remote_ip.s, sdp_line);
	LM_INFO("line[%s]\n", sdp_line);
}
	// sdp_parse_payload_types

// int audio_stream_start_with_files(AudioStream *stream, RtpProfile *prof,const char *remip, int remport,
// 	int rem_rtcp_port, int pt,int jitt_comp, const char *infile, const char * outfile)
// {
// 	return audio_stream_start_full(stream,prof,remip,remport,remip,rem_rtcp_port,pt,jitt_comp,infile,outfile,NULL,NULL,FALSE);
// }

	

 /*
  * int audio_stream_start_from_io(AudioStream *stream
  *                                RtpProfile *profile,
  *                                const char *rem_rtp_ip,
  *                                int rem_rtp_port,
                                   const char *rem_rtcp_ip,
				   int rem_rtcp_port,
				   int payload,
				   const MSMediaStreamIO *io) {
 */

/*
 *
	char *name;
	PayloadType *payload[RTP_PROFILE_MAX_PAYLOADS];
int audio_stream_start_full(AudioStream *stream,
		RtpProfile *profile,
		const char *rem_rtp_ip,
		int rem_rtp_port,
		const char *rem_rtcp_ip,
		int rem_rtcp_port,
		int payload,
		int jitt_comp, const char *infile, const char *outfile,
		MSSndCard *playcard,
		MSSndCard *captcard,
		bool_t use_ec)
*/
	return 1;
}
