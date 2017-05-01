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
str server_address = {0, 0};
str playback_fn = {0, 0};

static rms_t rms;

static cmd_export_t cmds[] = {
	{"rms_media_offer",(cmd_function)rms_media_offer,0,0,0,ANY_ROUTE },
	{"rms_transfer",(cmd_function)rms_transfer,0,0,0,ANY_ROUTE },
	{"rms_media_stop",(cmd_function)rms_media_stop,0,0,0,ANY_ROUTE },
	{"rms_sessions_dump",(cmd_function)rms_sessions_dump,0,0,0,ANY_ROUTE },
	{0, 0, 0, 0, 0, 0}
};

static pv_export_t mod_pvs[] = {
	{{0, 0}, 0, 0, 0, 0, 0, 0, 0}
};

static param_export_t mod_params[]={
	{"server_address", PARAM_STR, &server_address},
	{"playback_file_name", PARAM_STR, &playback_fn},
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
	0,     /* exported MI functions */
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
	rms_media_init();

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
	rms_media_destroy();
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

	signal(SIGINT,rms_signal_handler);
	int rtn = 0;
	return(rtn);
}

int rms_str_dup(str* dst, str* src, int shared) {
	if (!dst) {
		LM_ERR("dst null\n");
		return -1;
	}
	dst->len = 0;
	dst->s = NULL;
	if (!src) {
		LM_ERR("src null\n");
		return 0;
	}
	if ( (!src->s) || (src->len < 1)) {
		LM_ERR("empty src\n");
		return 0;
	}
	if (shared) {
		dst->s = shm_malloc(src->len +1);
	} else {
		dst->s = pkg_malloc(src->len +1);
	}
	if (!dst->s) {
		LM_ERR("%s_malloc: can't allocate memory (%d bytes)\n", shared?"shm":"pkg", src->len);
		return -1;
	}
	strncpy(dst->s, src->s, src->len);
	dst->len = src->len;
	return 1;
}

int rms_get_sdp_info (rms_sdp_info_t *sdp_info, struct sip_msg* msg) {
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

static int rms_relay_call(struct sip_msg* msg) {
	if(!tmb.t_relay(msg,NULL,NULL)) {
		LM_INFO("t_ralay error\n");
		return -1;
	}
	return 1;
}

str reply_headers = {0,0};
str headers = str_init("Max-Forwards: 70" CRLF);
str method_bye = str_init("BYE");
str method_ok = str_init("OK");
str body = {0,0};
str server_socket = {0,0};
str to = str_init("caller@127.0.0.111");
str from = str_init("media_server@127.0.0.101");

static int rms_hangup_call(char * call_id) {
	uac_req_t uac_r;
	int result;

	set_uac_req(&uac_r, &method_bye, &headers, &body, NULL,
		TMCB_LOCAL_COMPLETED, NULL, call_id);
	uac_r.ssock = &server_socket;
	result = tmb.t_request(&uac_r, &to, &to, &from, NULL);
	if(result < 0) {
		LM_ERR("error in tmb.t_request\n");
		return -1;
	}
	return 1;
}

static int rms_answer_call(struct sip_msg* msg, rms_session_info_t *si) {
	int status = 0;
	str reason;
	str to_tag;
	str contact_hdr;
	rms_sdp_info_t *sdp_info = &si->sdp_info;

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
	char buffer[128];
	snprintf(buffer,128,"Contact: <sip:rtp_server@%s>\r\nContent-Type: application/sdp\r\n", server_address.s);
	contact_hdr.len = strlen(buffer);
	contact_hdr.s = pkg_malloc(contact_hdr.len+1);
	strcpy(contact_hdr.s,buffer);
	sdp_info->local_ip = server_address.s;
	rms_sdp_set_reply_body(sdp_info, si->caller_media.pt->type);
	reason = method_ok;
	to_tag.s = "faketotag";
	to_tag.len = strlen("faketotag");
	if(!tmb.t_reply_with_body(tmb.t_gett(),200,&reason,&sdp_info->repl_body,&contact_hdr,&to_tag)) {
		LM_INFO("t_reply error");
	}
	return 1;
}

static rms_session_info_t * rms_session_search(char *callid, int len) {
	rms_session_info_t *si;
	clist_foreach(&rms_session_list, si, next){
		if (strncmp(callid, si->callid.s, len) == 0)
			return si;
	}
	return NULL;
}

static int rms_check_msg(struct sip_msg* msg) {
	if(!msg || !msg->callid || !msg->callid->body.s) {
		LM_INFO("no callid ?\n");
		return -1;
	}
	if(rms_session_search(msg->callid->body.s, msg->callid->body.len))
		return -1;
	return 1;
}

int rms_session_free(rms_session_info_t *si) {
	clist_rm(si,next,prev);
	rms_sdp_info_free(&si->sdp_info);
	if (si->caller_media.pt) {
		payload_type_destroy(si->caller_media.pt);
		si->caller_media.pt = NULL;
	}
	if (si->callee_media.pt) {
		payload_type_destroy(si->callee_media.pt);
		si->callee_media.pt = NULL;
	}
	if (si->callid.s) {
		shm_free(si->callid.s);
		si->callid.s = NULL;
	}
	if (si->from.s) {
		shm_free(si->from.s);
		si->from.s = NULL;
	}
	if (si->to.s) {
		shm_free(si->to.s);
		si->to.s = NULL;
	}
	shm_free(si);
	si = NULL;
	return 1;
}

rms_session_info_t *rms_session_new(struct sip_msg* msg) {
	if(!rms_check_msg(msg))
		return NULL;
	rms_session_info_t *si = shm_malloc(sizeof(rms_session_info_t));
	if (!si) {
		LM_ERR("can not allocate session info !\n");
		return NULL;
	}
	memset(si,0,sizeof(rms_session_info_t));
	//rms_sdp_info_init(&si->sdp_info);
	//memset(&si->callee_media,0,sizeof(call_leg_media_t));
	//memset(&si->caller_media,0,sizeof(call_leg_media_t));

	if (!rms_str_dup(&si->callid, &msg->callid->body,1)) {
		LM_ERR("can not get callid .\n");
		return NULL;
	}
	if (!rms_str_dup(&si->from, &msg->from->body,1))
		return NULL;
	if (!rms_str_dup(&si->to, &msg->to->body,1))
		return NULL;

	si->cseq = atoi(msg->cseq->body.s);

	rms_sdp_info_t *sdp_info = &si->sdp_info;
	if(!rms_get_sdp_info(sdp_info, msg)) {
		rms_session_free(si);
		return NULL;
	}
	si->caller_media.pt = rms_sdp_check_payload(sdp_info);
	if(!si->caller_media.pt) {
		rms_session_free(si);
		tmb.t_newtran(msg);
		tmb.t_reply(msg,488,"incompatible media format");
		return NULL;
	}
	clist_append(&rms_session_list,si,next,prev);
	return si;
}

int rms_sessions_dump(struct sip_msg* msg, char* param1, char* param2) {
	int x=1;
	rms_session_info_t *si;
	clist_foreach(&rms_session_list, si, next){
		LM_INFO("[%d] callid[%s] from[%s] to[%s] cseq[%d]", x, si->callid.s, si->from.s, si->to.s, si->cseq);
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
	if(!si){
		LM_INFO("session not found ci[%.*s]\n",  msg->callid->body.len, msg->callid->body.s);
		return 1;
	}
	LM_INFO("session found [%s] stopping\n", si->callid.s);
	//audio_stream_stop(si->ms.audio_stream);
	//rtp_profile_destroy(si->ms.rtp_profile);
	rms_stop_media(&si->caller_media);
	rms_session_free(si);
	tmb.t_newtran(msg);
	if(!tmb.t_reply(msg,200,"OK")) {
		return -1;
	}
	return 0;
}

static int rms_get_udp_port(void) {
	// RTP UDP port
	LM_INFO("last port[%d]", rms.udp_last_port);
	rms.udp_last_port += 2;
	if (rms.udp_last_port > rms.udp_end_port)
		rms.udp_last_port = rms.udp_start_port;
	LM_INFO("last port[%d]", rms.udp_last_port);
	return rms.udp_last_port;
}

int rms_create_call_leg(struct sip_msg* msg, rms_session_info_t *si, call_leg_media_t *m) {
	rms_sdp_info_t *sdp_info = &si->sdp_info;
	m->local_port = rms_get_udp_port();
	sdp_info->udp_local_port = m->local_port;

	LM_INFO("server IP[%s][%d]", server_address.s, server_address.len);
	m->local_ip = server_address.s;
	m->remote_port = atoi(sdp_info->remote_port);
	m->remote_ip = sdp_info->remote_ip;

	LM_INFO("remote_socket[%s:%s] local_socket[%s:%d] pt[%s]",
			sdp_info->remote_ip, sdp_info->remote_port,
			m->local_ip, m->local_port,
			si->caller_media.pt->mime_type);
	create_call_leg_media(m);
	return 1;
}

int rms_transfer(struct sip_msg* msg, char* param1, char* param2) {
	if(!rms_relay_call(msg)) {
		return -1;
	}
	rms_session_info_t *si = rms_session_new(msg);
	if (!si)
		return -1;
	if (!rms_create_call_leg(msg, si, &si->caller_media))
		return -1;
	return 1;
}

int rms_media_offer(struct sip_msg* msg, char* param1, char* param2) {
	rms_session_info_t *si = rms_session_new(msg);
	if (!si)
		return -1;
	if (!rms_create_call_leg(msg, si, &si->caller_media))
		return -1;
	if (!rms_answer_call(msg, si)) {
		return -1;
	}
	rms_playfile(&si->caller_media, playback_fn.s);
	return 0;
}
