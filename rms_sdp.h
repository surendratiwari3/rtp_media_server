
#ifndef rms_sdp_h
#define rms_sdp_h

#include "../../sr_module.h"
#include <mediastreamer2/mediastream.h>

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
void rms_sdp_info_init(rms_sdp_info_t * sdp_info);
void rms_sdp_info_free(rms_sdp_info_t * sdp_info);
PayloadType* rms_sdp_check_payload(rms_sdp_info_t *);

#endif
