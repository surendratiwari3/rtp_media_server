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

#ifndef rms_media_h
#define rms_media_h

#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msrtp.h"
#include <mediastreamer2/mediastream.h>
#include <ortp/ortp.h>

typedef struct call_leg_media {
	RtpSession *rtps;
	PayloadType *pt;
	MSFilter *ms_encoder;
	MSFilter *ms_decoder;
	MSFilter *ms_rtprecv;
	MSFilter *ms_rtpsend;
	char* local_ip;
	int local_port;
	char* remote_ip;
	int remote_port;
} call_leg_media_t;

int create_call_leg_media(call_leg_media_t *m);

int rms_media_init();

void rms_media_destroy();

MSFactory *rms_get_factory();

#endif
