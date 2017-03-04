
#ifndef rms_sdp_h
#define rms_sdp_h

#include "../../sr_module.h"


typedef struct rms_sdp_info {
	char * remote_ip;
	char * payloads;
	char * remote_port;
	int ipv6;
	str repl_body;
	str recv_body;
	int udp_local_port;
} rms_sdp_info_t;

void rms_sdp_set_reply_body(rms_sdp_info_t *, int payload_type_number);

#endif
