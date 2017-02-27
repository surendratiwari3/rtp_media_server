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

#include "../../sr_module.h"
#include "../../parser/sdp/sdp_helpr_funcs.h"
#include "../../parser/parse_content.h"
#include "../../modules/tm/tm_load.h"
#include "../../modules/sdpops/api.h"
#include "../../data_lump_rpl.h"
#include "../../clist.h"

#include <mediastreamer2/mediastream.h>
#include <ortp/ortp.h>

// documentation
// https://www.kamailio.org/dokuwiki/doku.php/development:write-module
// http://www.kamailio.org/docs/kamailio-devel-guide/#c16makefile

static int rms_media_stop(struct sip_msg *, char *, char *);
static int rms_media_offer(struct sip_msg *, char *, char *);
static int rms_sessions_dump(struct sip_msg *, char *, char *);

static PayloadType* rms_check_payload(struct sip_msg*);

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

typedef struct rms {
	int udp_start_port;
	int udp_end_port;
	int udp_last_port;
} rms_t;

struct tm_binds tmb;
typedef struct rms_sdp_info {
	char * remote_ip;
	char * payloads;
	char * remote_port;
	int ipv6;
	str reply_body;
	int udp_local_port;
} rms_sdp_info_t;

typedef struct ms_res {
	AudioStream *audio_stream;
	PayloadType *pt;
	RtpProfile *rtp_profile;
} ms_res_t;

typedef struct rms_session_info {
	struct rms_session_info* next;
	struct rms_session_info* prev;
	rms_sdp_info_t sdp_info;
	char * session_id;
	ms_res_t ms;
} rms_session_info_t;
