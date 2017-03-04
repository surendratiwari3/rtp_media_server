#include "rms_sdp.h"

// https://tools.ietf.org/html/rfc4566
// (protocol version)
const char *sdp_v = "v=0\r\n";
// (originator and session identifier)
const char *sdp_o = "o=- 1028316687 1 IN IP4 127.0.0.2\r\n";
// (session name)
const char *sdp_s = "s=-\r\n";
// (connection information -- not required if included in all media)
const char *sdp_c = "c=IN IP4 127.0.0.2\r\n";
// (time the session is active)
const char *sdp_t = "t=0 0\r\n";

//"a=rtpmap:101 telephone-event/8000\r\n"
//"a=fmtp:101 0-15\r\n";
//"a=rtpmap:0 PCMU/8000\r\n"
//"a=rtpmap:8 PCMA/8000\r\n"
//"a=rtpmap:96 opus/48000/2\r\n"
//"a=fmtp:96 useinbandfec=1\r\n";

void rms_sdp_set_reply_body(rms_sdp_info_t * sdp_info, int payload_type_number) {
	if(sdp_info->repl_body.s)
		return;

	str *body = &sdp_info->repl_body;
	body->len=strlen(sdp_v)+strlen(sdp_o)+strlen(sdp_s);
	body->len+=strlen(sdp_c)+strlen(sdp_t);

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
