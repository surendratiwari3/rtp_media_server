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

#ifndef rms_h
#define rms_h

#include "../../sr_module.h"
#include "../../parser/sdp/sdp_helpr_funcs.h"
#include "../../parser/parse_content.h"
#include "../../modules/tm/tm_load.h"
#include "../../modules/sdpops/api.h"
#include "../../data_lump_rpl.h"
#include "../../clist.h"

#include <mediastreamer2/mediastream.h>
#include <ortp/ortp.h>

#include "rms_sdp.h"

// documentation
// https://www.kamailio.org/dokuwiki/doku.php/development:write-module
// http://www.kamailio.org/docs/kamailio-devel-guide/#c16makefile

static int rms_media_stop(struct sip_msg *, char *, char *);
static int rms_media_offer(struct sip_msg *, char *, char *);
static int rms_transfer(struct sip_msg *, char *, char *);
static int rms_sessions_dump(struct sip_msg *, char *, char *);

typedef struct rms {
	int udp_start_port;
	int udp_end_port;
	int udp_last_port;
} rms_t;

struct tm_binds tmb;

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
	PayloadType *pt;
} rms_session_info_t;

#endif
