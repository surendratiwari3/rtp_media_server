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
#define CRLF "\r\n"
#define CRLF_LEN (sizeof(CRLF) - 1)

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

void rms_signal_handler(int signum) {
	LM_INFO("signal received [%d]\n", signum);
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
	ortp_init();
	//ms_init();
	signal(SIGINT,rms_signal_handler);
	int rtn = 0;
	return(rtn);
}

typedef struct rms_sdp_info {
	char * remote_ip;
	char * payloads;
	char * remote_port;
	int ipv6;
} rms_sdp_info_t;

static void rms_sdp_info_init(rms_sdp_info_t * sdp_info) {
	sdp_info->remote_ip=NULL;
	sdp_info->remote_port=NULL;
	sdp_info->payloads=NULL;
	sdp_info->ipv6=0;
}

static void rms_sdp_info_free(rms_sdp_info_t * sdp_info) {
	if(sdp_info->remote_ip) {
		pkg_free(sdp_info->remote_ip);
		sdp_info->remote_ip = NULL;
	}
	if(sdp_info->remote_port) {
		pkg_free(sdp_info->remote_port);
		sdp_info->remote_port = NULL;
	}
	if(sdp_info->payloads) {
		pkg_free(sdp_info->payloads);
		sdp_info->payloads = NULL;
	}
}

const char *reply_body =
"v=0\r\n"
"o=- 1028316687 1 IN IP4 127.0.0.2\r\n"
"s=-\r\n"
"c=IN IP4 127.0.0.2\r\n"
"t=0 0\r\n"
"m=audio 49170 RTP/AVP 0 101\r\n";
//"a=rtpmap:101 telephone-event/8000\r\n"
//"a=fmtp:101 0-15\r\n";

//"v=0\r\n"
//"s=Talk\r\n"
//"c=IN IP4 127.0.0.2\r\n"
//"t=0 0\r\n"
//"m=audio 49170 RTP/AVP 0 8 96\r\n"
//"a=rtpmap:0 PCMU/8000\r\n"
//"a=rtpmap:8 PCMA/8000\r\n"
//"a=rtpmap:96 opus/48000/2\r\n"
//"a=fmtp:96 useinbandfec=1\r\n";

static int rms_get_sdp_info (rms_sdp_info_t *sdp_info, struct sip_msg* msg) {
	sdp_session_cell_t* sdp_session;
	sdp_stream_cell_t* sdp_stream;
	str media_ip, media_port;
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
			sdp_info->payloads=pkg_malloc(sdp_stream->payloads.len+1);
			strncpy(sdp_info->payloads,sdp_stream->payloads.s,sdp_stream->payloads.len);
			sdp_info->payloads[sdp_stream->payloads.len]='\0';
		}
	}
	if (sdp_stream->ip_addr.s && sdp_stream->ip_addr.len>0) {
		media_ip = sdp_stream->ip_addr;
		//pf = sdp_stream->pf;
	} else {
		media_ip = sdp_session->ip_addr;
		//pf = sdp_session->pf;
	}
	sdp_info->remote_ip=pkg_malloc(media_ip.len+1);
	strncpy(sdp_info->remote_ip, media_ip.s, media_ip.len);
	sdp_info->remote_ip[media_ip.len]='\0';

	media_port = sdp_stream->port;
	sdp_info->remote_port=pkg_malloc(media_port.len+1);
	strncpy(sdp_info->remote_port, media_port.s, media_port.len);
	sdp_info->remote_port[media_port.len]='\0';
	return 1;
}

static int rms_answer_call(struct sip_msg* msg) {
	int status = 0;
	str r_body;
	str reason;
	str to_tag;
	str contact_hdr;

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
	contact_hdr.s = strdup("Contact: <sip:rtp_server@127.0.0.2>\r\nContent-Type: application/sdp\r\n");
	contact_hdr.len = strlen("Contact: <sip:rtp_server@127.0.0.2>\r\nContent-Type: application/sdp\r\n");
	r_body.s = strdup(reply_body);
	r_body.len = strlen(reply_body);
	reason.s = strdup("OK");
	reason.len = strlen("OK");
	to_tag.s = strdup("faketotag");
	to_tag.len = strlen("faketotag");
	if(!tmb.t_reply_with_body(tmb.t_gett(),200,&reason,&r_body,&contact_hdr,&to_tag)) {
		LM_INFO("t_reply error");
	}
	return 1;
}


int rtp_media_offer(struct sip_msg* msg, char* param1, char* param2) {
	rms_sdp_info_t sdp_info;
	rms_sdp_info_init(&sdp_info);
	if(!rms_get_sdp_info(&sdp_info, msg)) {
		return -1;
	}

	LM_INFO("remote ip[%s]", sdp_info.remote_ip);
	LM_INFO("remote port[%s]", sdp_info.remote_port);
	LM_INFO("payloads[%s]", sdp_info.payloads);
{ //stat stream oRTP/src/tests/rtprecv.c

	//ortp_scheduler_init();
	//signal(SIGINT,rms_signal_handler);
	const char * log_fn = "/tmp/ortp.log";
	FILE * log_file =  fopen (log_fn, "w+");
	if (log_file) {
		LM_INFO("ortp logs are redirected [%s]\n", log_fn);
	} else {
		log_file = stdout;
		LM_INFO("ortp can not open logs file [%s]\n", log_fn);
	}
	ortp_set_log_file(log_file);
	ortp_set_log_level_mask(NULL, ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	int local_port=50000;
	MSFactory *ms_factory = ms_factory_new_with_voip(); // ms_factory_new();
	PayloadType *pt = payload_type_new();

	pt->clock_rate=8000;
	pt->mime_type=strdup("pcmu"); /* ia=rtpmap:0 PCMU/8000*/
	pt->channels=1;

	if (ms_factory) {
		LM_INFO("ms_factory->mtu[%d]", ms_factory->mtu);
	}

//	number=payload_type_get_number(pt);
//	if (rtp_profile_get_payload(prof,number)!=NULL){
//		ms_warning("A payload type with number %i already exists in profile !",number);
//	}else
//		rtp_profile_set_payload(prof,number,pt);


//	int type; /**< one of PAYLOAD_* macros*/
//	int clock_rate; /**< rtp clock rate*/
//	char bits_per_sample;	/* in case of continuous audio data */
//	char *zero_pattern;
//	int pattern_length;
//	int normal_bitrate;	/*in bit/s */
//	char *mime_type; /**<actually the submime, ex: pcm, pcma, gsm*/
//	int channels; /**< number of channels of audio */
//	char *recv_fmtp; /* various format parameters for the incoming stream */
//	char *send_fmtp; /* various format parameters for the outgoing stream */
//	struct _PayloadTypeAvpfParams avpf; /* AVPF parameters */
//	int flags;
//	void *user_data;
//
	AudioStream *audio_stream = audio_stream_new(ms_factory,local_port,local_port+1,sdp_info.ipv6);
	int pt_idx = 0;
	if(audio_stream) {
		LM_INFO("ms audio_stream created\n");
	}
	RtpProfile *rtp_profile = rtp_profile_new("remote");
	if(rtp_profile) {
		rtp_profile_set_payload(rtp_profile, pt_idx, pt);
		LM_INFO("rtp_profile created: %s\n", rtp_profile->name);
	}
	const char *infile = strdup("/home/cloud/git/bc-linphone/mediastreamer2/tester/sounds/hello8000.wav");
	const char *outfile = NULL;
	audio_stream_start_with_files(audio_stream,
                                      rtp_profile,
                                      sdp_info.remote_ip,
                                      (int)atoi(sdp_info.remote_port),
                                      (int)(atoi(sdp_info.remote_port)+1),
                                      pt_idx, 60, infile, outfile);
}
	rms_sdp_info_free(&sdp_info);

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
