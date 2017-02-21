#include "../../sr_module.h"
#include "../../parser/sdp/sdp_helpr_funcs.h"
#include "../../parser/parse_content.h"
#include "../../modules/tm/tm_load.h"
#include "../../data_lump_rpl.h"

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

struct tm_binds tmb;
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
	if (load_tm_api(&tmb)!=0) {
		LM_ERR( "can't load TM API\n");
		return -1;
	}
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


typedef struct rms_sdp_info {
	char * remote_ip;
} rms_sdp_info_t;


static int rms_get_sdp_info (rms_sdp_info_t *sdp_info, struct sip_msg* msg) {
	str tmp;
	sdp_session_cell_t* sdp_session;
	sdp_stream_cell_t* sdp_stream;
	str media_ip;
	int sdp_session_num = 0;
	int sdp_stream_num = get_sdp_stream_num(msg);
	if(parse_sdp(msg) < 0) {
		LM_INFO("can not parse sdp\n");
		return -1;
	}
	sdp_info_t *sdp = (sdp_info_t*)msg->body;
	if(!sdp) {
		LM_INFO("sdp null\n");
		return -1;
	}
	LM_INFO("sdp body type[%d]\n", sdp->type);
	if (sdp_stream_num > 1 || !sdp_stream_num) {
		LM_INFO("only support one stream[%d]\n[%s]\n", sdp_stream_num, get_body(msg));
	}
	sdp_stream_num = 0;
	sdp_session = get_sdp_session(msg, sdp_session_num);
	if(!sdp_session) {
		return -1;
	} else {
		int sdp_stream_num = 0;
		sdp_stream = get_sdp_stream(msg, sdp_session_num, sdp_stream_num);
		if(!sdp_stream) {
			LM_INFO("can not get the sdp stream\n");
			return -1;
		} else {
			char *payloads=pkg_malloc(sdp_stream->payloads.len+1);
			strncpy(payloads,sdp_stream->payloads.s,sdp_stream->payloads.len);
			payloads[sdp_stream->payloads.len]='\0';
			LM_INFO("payloads_num[%d] payload[%s]\n", sdp_stream->payloads_num, payloads);
		}
	}
	if (sdp_stream->ip_addr.s && sdp_stream->ip_addr.len>0) {
		media_ip = sdp_stream->ip_addr;
		//pf = sdp_stream->pf;
	} else {
		media_ip = sdp_session->ip_addr;
		//pf = sdp_session->pf;
	}
	//if(media)
//	sdp_info->remote_ip = pkg_malloc(tmp.len + 1);
//	strncpy(sdp_info->remote_ip, media_ip.s, media_ip.len);
//	sdp_info->remote_ip[media_ip.len] = '\0';
//	LM_INFO("remote media IP[%s]\n", sdp_info->remote_ip);
	return 1;
}

static int rms_answer_call(struct sip_msg* msg) {
	int status = 0;
	if(msg->REQ_METHOD!=METHOD_INVITE) {
		LM_INFO("only invite is supported for offer \n");
		return -1;
	}
	LM_INFO("invite processing\n");
	status = tmb.t_newtran(msg);
	LM_INFO("invite new transaction[%d]\n", status);
	if(status < 0) {
		LM_INFO("error creating transaction \n");
		return -1;
	} else if (status == 0) {
		LM_INFO("retransmission");
		return 0;
	}
	LM_INFO("transaction created\n");
	char header[255] = "Contact: <sip:rtp_server@127.0.0.2>\n";
	if (!add_lump_rpl2 (msg, header, strlen(header), LUMP_RPL_HDR)) {
		LM_ERR ("error adding contact header! [%s]\n", header);
	}
	//t_reply_with_body();
	if(!tmb.t_reply(msg,200,"OK")) {
		LM_INFO("t_reply error");
	}
	return 1;
}

int rtp_media_offer(struct sip_msg* msg, char* param1, char* param2) {
	rms_sdp_info_t sdp_info;
	if(!rms_get_sdp_info(&sdp_info, msg)) {
		return -1;
	}
	//char * remote_ip;// = str_init("255.255.255.255");
	RtpProfile *profile = rtp_profile_new("remote");
	LM_INFO("rtp_profile created: %s\n", profile->name);

	if(!rms_answer_call(msg)) {
		return -1;
	}

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
