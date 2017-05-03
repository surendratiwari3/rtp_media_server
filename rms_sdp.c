#include "rms_sdp.h"


// https://tools.ietf.org/html/rfc4566
// (protocol version)
const char *sdp_v = "v=0\r\n";
// (session name)
const char *sdp_s = "s=-\r\n";
// (time the session is active)
const char *sdp_t = "t=0 0\r\n";
//"a=rtpmap:101 telephone-event/8000\r\n"
//"a=fmtp:101 0-15\r\n";
//"a=rtpmap:0 PCMU/8000\r\n"
//"a=rtpmap:8 PCMA/8000\r\n"
//"a=rtpmap:96 opus/48000/2\r\n"
//"a=fmtp:96 useinbandfec=1\r\n";

inline static char* shm_strdup(char *source) {
	char *copy;
	if (!source)
		return NULL;
	copy = (char*)shm_malloc(strlen(source) + 1);
	if (!copy)
		return NULL;
	strcpy(copy, source);
	return copy;
}

void rms_sdp_info_init(rms_sdp_info_t * sdp_info) {
	sdp_info->remote_ip=NULL;
	sdp_info->remote_port=NULL;
	sdp_info->payloads=NULL;
	sdp_info->ipv6=0;
	sdp_info->repl_body.s=NULL;
	sdp_info->recv_body.s=NULL;
}

void rms_sdp_info_free(rms_sdp_info_t * sdp_info) {
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

void rms_sdp_set_reply_body(rms_sdp_info_t * sdp_info, int payload_type_number) {
	if(sdp_info->repl_body.s)
		return;

	str *body = &sdp_info->repl_body;
	body->len=strlen(sdp_v)+strlen(sdp_s)+strlen(sdp_t);

	// (originator and session identifier)
	char sdp_o[128];
	snprintf(sdp_o,128,"o=- 1028316687 1 IN IP4 %s\r\n", sdp_info->local_ip);
	body->len+=strlen(sdp_o);

	// (connection information -- not required if included in all media)
	char sdp_c[128];
	snprintf(sdp_c,128,"c=IN IP4 %s\r\n", sdp_info->local_ip);
	body->len+=strlen(sdp_c);

	char sdp_m[128];
	snprintf(sdp_m,128,"m=audio %d RTP/AVP %d\r\n",
                               sdp_info->udp_local_port, payload_type_number);
	body->len+=strlen(sdp_m);

	body->s=pkg_malloc(body->len+1);
	strcpy(body->s, sdp_v);
	strcat(body->s, sdp_o);
	strcat(body->s, sdp_s);
	strcat(body->s, sdp_c);
	strcat(body->s, sdp_t);
	strcat(body->s, sdp_m);
}

static char * rms_sdp_get_rtpmap(str body, int type_number) {
	char *pos = body.s;
	while ((pos = strstr(pos, "a=rtpmap:"))) {
		int id;
		char codec[64];
		sscanf(pos,"a=rtpmap:%d %64s", &id, codec);
		if(id == type_number) {
			LM_INFO("[%d][%s]\n", id, codec);
			return shm_strdup(codec);
		}
		pos++;
	}
	return NULL;
}

PayloadType* rms_sdp_check_payload(rms_sdp_info_t *sdp) {
	// https://tools.ietf.org/html/rfc3551
	LM_INFO("payloads[%s]", sdp->payloads); //0 8
	PayloadType *pt = payload_type_new();
	char * payloads = sdp->payloads;
	char * payload_type_number=strtok(payloads," ");
	if (!payload_type_number) {
		payload_type_destroy(pt);
		return NULL;
	}
	pt->type = atoi(payload_type_number);
	pt->clock_rate=8000;
	pt->channels=1;
	pt->mime_type=NULL;
	while(!pt->mime_type) {
		if (pt->type > 127) {
			return NULL;
		} else if (pt->type >= 96) {
			char *rtpmap = rms_sdp_get_rtpmap(sdp->recv_body, pt->type);
			pt->mime_type = shm_strdup(strtok(rtpmap, "/"));
			if (strcasecmp(pt->mime_type,"opus") == 0) {
				pt->clock_rate = atoi(strtok(NULL, "/"));
				pt->channels = atoi(strtok(NULL, "/"));
				free(rtpmap);
				return pt;
			}
			free(pt->mime_type);
			pt->mime_type=NULL;
			free(rtpmap);
		} else if (pt->type == 0) {
			pt->mime_type=shm_strdup("pcmu"); /* ia=rtpmap:0 PCMU/8000*/
		} else if (pt->type == 8) {
			pt->mime_type=shm_strdup("pcma");
		} else if (pt->type == 9) {
			pt->mime_type=shm_strdup("g722");
		} else if (pt->type == 18) {
			pt->mime_type=shm_strdup("g729");
		}
		if(pt->mime_type)
			break;
		payload_type_number=strtok(NULL," ");
		if (!payload_type_number) {
			payload_type_destroy(pt);
			return NULL;
		}
		pt->type = atoi(payload_type_number);
	}
	LM_INFO("payload_type:%d %s/%d/%d\n", pt->type, pt->mime_type, pt->clock_rate, pt->channels);
	return pt;
}
