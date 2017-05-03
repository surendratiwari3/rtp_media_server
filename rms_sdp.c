#include "rms_sdp.h"
#include "../../core/data_lump.h"
#include "../../core/parser/parse_content.h"

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

static char* rms_shm_strdup(char *source) {
	char *copy;
	if (!source)
		return NULL;
	copy = (char*)shm_malloc(strlen(source) + 1);
	if (!copy)
		return NULL;
	strcpy(copy, source);
	return copy;
}

static char* rms_sdp_get_rtpmap(str body, int type_number) {
	char *pos = body.s;
	while ((pos = strstr(pos, "a=rtpmap:"))) {
		int id;
		char codec[64];
		sscanf(pos,"a=rtpmap:%d %64s", &id, codec);
		if(id == type_number) {
			LM_INFO("[%d][%s]\n", id, codec);
			return rms_shm_strdup(codec);
		}
		pos++;
	}
	return NULL;
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

// should be called "prepare" not actualy set ?
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
			pt->mime_type = rms_shm_strdup(strtok(rtpmap, "/"));
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
			pt->mime_type=rms_shm_strdup("pcmu"); /* ia=rtpmap:0 PCMU/8000*/
		} else if (pt->type == 8) {
			pt->mime_type=rms_shm_strdup("pcma");
		} else if (pt->type == 9) {
			pt->mime_type=rms_shm_strdup("g722");
		} else if (pt->type == 18) {
			pt->mime_type=rms_shm_strdup("g729");
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

str content_type_sdp = str_init("application/sdp");

int rms_sdp_set_body(struct sip_msg* msg, str* new_body) {
	struct lump *anchor;
	char* buf;
	int len;
	char* value_s;
	int value_len;
	str body = {0,0};

	if (!new_body->s || new_body->len == 0) {
		LM_ERR("invalid body parameter\n");
		return -1;
	}

	body.len = 0;
	body.s = get_body(msg);
	if (body.s==0) {
		LM_ERR("malformed sip message\n");
		return -1;
	}

	del_nonshm_lump( &(msg->body_lumps) );
	msg->body_lumps = NULL;

	if (msg->content_length) {
		body.len = get_content_length( msg );
		if (body.len > 0) {
			if (body.s+body.len>msg->buf+msg->len) {
				LM_ERR("invalid content length: %d\n", body.len);
				return -1;
			}
			if(del_lump(msg, body.s - msg->buf, body.len, 0) == 0) {
				LM_ERR("cannot delete existing body");
				return -1;
			}
		}
	}

	anchor = anchor_lump(msg, msg->unparsed - msg->buf, 0, 0);
	if (! anchor) {
		LM_ERR("failed to get anchor\n");
		return -1;
	}

	if ( msg->content_length == 0 ) {
		/* need to add Content-Length */
		len = new_body->len;
		value_s=int2str(len, &value_len);
		LM_DBG("content-length: %d (%s)\n", value_len, value_s);

		len=CONTENT_LENGTH_LEN+value_len+CRLF_LEN;
		buf=pkg_malloc(sizeof(char)*(len));

		if (buf==0) {
			LM_ERR("out of pkg memory\n");
			return -1;
		}

		memcpy(buf, CONTENT_LENGTH, CONTENT_LENGTH_LEN);
		memcpy(buf+CONTENT_LENGTH_LEN, value_s, value_len);
		memcpy(buf+CONTENT_LENGTH_LEN+value_len, CRLF, CRLF_LEN);
		if (insert_new_lump_after(anchor, buf, len, 0) == 0) {
			LM_ERR("failed to insert content-length lump\n");
			pkg_free(buf);
			return -1;
		}
	}

	/* add content-type */
	if (msg->content_type==NULL || msg->content_type->body.len != content_type_sdp.len
			|| strncmp(msg->content_type->body.s, content_type_sdp.s, content_type_sdp.len)!=0) {
		if (msg->content_type!=NULL) {
			if (del_lump(msg, msg->content_type->name.s-msg->buf, msg->content_type->len, 0) == 0) {
				LM_ERR("failed to delete content type\n");
				return -1;
			}
		}
		value_len = content_type_sdp.len;
		len=sizeof("Content-Type: ") - 1 + value_len + CRLF_LEN;
		buf=pkg_malloc(sizeof(char)*(len));

		if (buf==0) {
			LM_ERR("out of pkg memory\n");
			return -1;
		}
		memcpy(buf, "Content-Type: ", sizeof("Content-Type: ") - 1);
		memcpy(buf+sizeof("Content-Type: ") - 1, content_type_sdp.s, value_len);
		memcpy(buf+sizeof("Content-Type: ") - 1 + value_len, CRLF, CRLF_LEN);
		if (insert_new_lump_after(anchor, buf, len, 0) == 0) {
			LM_ERR("failed to insert content-type lump\n");
			pkg_free(buf);
			return -1;
		}
	}
	anchor = anchor_lump(msg, body.s - msg->buf, 0, 0);

	if (anchor == 0) {
		LM_ERR("failed to get body anchor\n");
		return -1;
	}

	buf=pkg_malloc(sizeof(char)*(new_body->len));
	if (buf==0) {
		LM_ERR("out of pkg memory\n");
		return -1;
	}
	memcpy(buf, new_body->s, new_body->len);
	if (!insert_new_lump_after(anchor, buf, new_body->len, 0)) {
		LM_ERR("failed to insert body lump\n");
		pkg_free(buf);
		return -1;
	}
	LM_DBG("new body: [%.*s]", new_body->len, new_body->s);
	return 1;
}
