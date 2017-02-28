/*
 * Copyright (C) 2017 Julien Chavanton jchavanton@gmail.com
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "rtp_media_server.h"

MODULE_VERSION

static int mod_init(void);
static void mod_destroy(void);
static int child_init(int);

static rms_session_info_t rms_session_list;
static MSFactory *ms_factory;
static rms_t rms;

static sdpops_api_t sdp_ops;

static cmd_export_t cmds[] = {
	{"rms_media_offer",(cmd_function)rms_media_offer,0,0,0,ANY_ROUTE },
	{"rms_media_stop",(cmd_function)rms_media_stop,0,0,0,ANY_ROUTE },
	{"rms_sessions_dump",(cmd_function)rms_sessions_dump,0,0,0,ANY_ROUTE },
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
	rms.udp_start_port = 50000;
	rms.udp_end_port = 60000;
	rms.udp_last_port = 50000;

	clist_init(&rms_session_list,next,prev);
	ms_factory = ms_factory_new_with_voip();
	if (load_tm_api(&tmb)!=0) {
		LM_ERR( "can't load TM API\n");
		return -1;
	}
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
	return(0);
}

/**
 * Called only once when OpenSER is shuting down to clean up module
 * resources.
 */
static void mod_destroy() {
	ms_factory_destroy(ms_factory);
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
	signal(SIGINT,rms_signal_handler);
	int rtn = 0;
	return(rtn);
}

static void rms_sdp_info_init(rms_sdp_info_t * sdp_info) {
	sdp_info->remote_ip=NULL;
	sdp_info->remote_port=NULL;
	sdp_info->payloads=NULL;
	sdp_info->ipv6=0;
	sdp_info->repl_body.s=NULL;
	sdp_info->recv_body.s=NULL;
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
	if(sdp_info->repl_body.s) {
		pkg_free(sdp_info->repl_body.s);
		sdp_info->repl_body.s = NULL;
		sdp_info->repl_body.len = 0;
	}
}

static void set_reply_body(rms_sdp_info_t * sdp_info) {
	if(sdp_info->repl_body.s)
		return;

	str *body = &sdp_info->repl_body;
	body->len=strlen(sdp_v)+strlen(sdp_o)+strlen(sdp_s);
	body->len+=strlen(sdp_c)+strlen(sdp_t);

	char sdp_m[128];
	snprintf(sdp_m,128,"m=audio %d RTP/AVP 0 101\r\n",
                               sdp_info->udp_local_port);
	body->len+=strlen(sdp_m);

	body->s=pkg_malloc(body->len+1);
	strcpy(body->s, sdp_v);
	strcat(body->s, sdp_o);
	strcat(body->s, sdp_s);
	strcat(body->s, sdp_c);
	strcat(body->s, sdp_t);
	strcat(body->s, sdp_m);
}

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
	sdp_info->recv_body.s = sdp->text.s;
	sdp_info->recv_body.len = sdp->text.len;


	LM_INFO("sdp body - type[%d]\n", sdp->type);
	if (sdp_stream_num > 1 || !sdp_stream_num) {
		LM_INFO("only support one stream[%d]\n", sdp_stream_num);
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

static int rms_answer_call(struct sip_msg* msg, rms_sdp_info_t *sdp_info) {
	int status = 0;
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
	set_reply_body(sdp_info);
	reason.s = strdup("OK");
	reason.len = strlen("OK");
	to_tag.s = strdup("faketotag");
	to_tag.len = strlen("faketotag");
	if(!tmb.t_reply_with_body(tmb.t_gett(),200,&reason,&sdp_info->repl_body,&contact_hdr,&to_tag)) {
		LM_INFO("t_reply error");
	}
	return 1;
}

rms_session_info_t *rms_session_new() {
	rms_session_info_t *si = shm_malloc(sizeof(rms_session_info_t));
	rms_sdp_info_init(&si->sdp_info);
	clist_append(&rms_session_list,si,next,prev);
	return si;
}

int rms_session_free(rms_session_info_t *si) {
	clist_rm(si,next,prev);
	rms_sdp_info_free(&si->sdp_info);
	shm_free(si);
	si = NULL;
	return 1;
}

rms_session_info_t * rms_session_search(char *callid, int len) {
	rms_session_info_t *si;
	clist_foreach(&rms_session_list, si, next){
		if (strncmp(callid, si->session_id, len) == 0)
			return si;
	}
	return NULL;
}

int rms_sessions_dump(struct sip_msg* msg, char* param1, char* param2) {
	int x=1;
	rms_session_info_t *si;
	clist_foreach(&rms_session_list, si, next){
		LM_INFO("[%d] session_id[%s]", x, si->session_id);
		x++;
	}
	return 1;
}

int rms_media_stop(struct sip_msg* msg, char* param1, char* param2) {
	rms_session_info_t *si;
	if(!msg || !msg->callid || !msg->callid->body.s) {
		LM_INFO("no callid ?\n");
		return -1;
	}
	si = rms_session_search(msg->callid->body.s, msg->callid->body.len);
	if(!si)
		return 1;
	LM_INFO("session found [%s] stopping\n", si->session_id);
	audio_stream_stop(si->ms.audio_stream);
	rtp_profile_destroy(si->ms.rtp_profile);
	//payload_type_destroy(si->ms.pt);

	rms_session_free(si);
	tmb.t_newtran(msg);
	if(!tmb.t_reply(msg,200,"OK")) {
		return -1;
	}
	return 0;
}

static PayloadType* rms_check_payload(rms_sdp_info_t *sdp) {
	char *pos = sdp->recv_body.s;
	PayloadType *pt=NULL;
	while ((pos = strstr(pos, "a=rtpmap:"))) {
		int id;
		char codec[64];
		sscanf(pos,"a=rtpmap:%d %64s", &id, codec);
		LM_INFO("[%d][%s]\n", id, codec);
		pos++;
	}
	return pt;
}

int rms_media_offer(struct sip_msg* msg, char* param1, char* param2) {
	if(!msg || !msg->callid || !msg->callid->body.s) {
		LM_INFO("no callid ?\n");
		return -1;
	}
	if(rms_session_search(msg->callid->body.s, msg->callid->body.len))
		return 1;

	rms_session_info_t *si = rms_session_new();
	si->session_id = strndup(msg->callid->body.s, msg->callid->body.len);
	rms_sdp_info_t *sdp_info = &si->sdp_info;

	if(!rms_get_sdp_info(sdp_info, msg)) {
		return -1;
	}

	LM_INFO("remote ip[%s]", sdp_info->remote_ip);
	LM_INFO("remote port[%s]", sdp_info->remote_port);
	LM_INFO("payloads[%s]", sdp_info->payloads);
	rms_check_payload(sdp_info);
{
	PayloadType *pt = payload_type_new();

	pt->clock_rate=8000;
	pt->mime_type=strdup("pcmu"); /* ia=rtpmap:0 PCMU/8000*/
	pt->channels=1;

	if (ms_factory) {
		LM_INFO("ms_factory->mtu[%d]", ms_factory->mtu);
	}

	rms.udp_last_port += 2;
	if (rms.udp_last_port > rms.udp_end_port)
		rms.udp_last_port = rms.udp_start_port;
	sdp_info->udp_local_port = rms.udp_last_port;

	si->ms.audio_stream = audio_stream_new(ms_factory,
                 sdp_info->udp_local_port,
		 sdp_info->udp_local_port+1, sdp_info->ipv6);
	int pt_idx = 0;
	if(si->ms.audio_stream) {
		LM_INFO("ms audio_stream created\n");
	}
	si->ms.rtp_profile = rtp_profile_new("remote");
	if(si->ms.rtp_profile) {
		rtp_profile_set_payload(si->ms.rtp_profile, pt_idx, pt);
		LM_INFO("rtp_profile created: %s\n", si->ms.rtp_profile->name);
	}
	const char *infile = strdup("/home/cloud/git/bc-linphone/mediastreamer2/tester/sounds/hello8000.wav");
	const char *outfile = NULL;
	audio_stream_start_with_files(si->ms.audio_stream,
                                      si->ms.rtp_profile,
                                      sdp_info->remote_ip,
                                      (int)atoi(sdp_info->remote_port),
                                      (int)(atoi(sdp_info->remote_port)+1),
                                      pt_idx, 60, infile, outfile);
}
	if(!rms_answer_call(msg, sdp_info)) {
		return -1;
	}
	return 0;
}
