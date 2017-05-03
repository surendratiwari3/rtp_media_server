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

#include "../../core/mem/shm.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/dtmfgen.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilerec.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/mstonedetector.h"
//#include <mediastreamer2/mediastream.h>
#include <ortp/ortp.h>
#include <ortp/port.h>

#define MS_UNUSED(x) ((void)(x))

typedef struct call_leg_media {
	MSFactory *ms_factory;
	RtpSession *rtps;
	PayloadType *pt;
	MSTicker *ms_ticker;
	MSFilter *ms_encoder;
	MSFilter *ms_decoder;
	MSFilter *ms_rtprecv;
	MSFilter *ms_rtpsend;
	MSFilter *ms_player;
	MSFilter *ms_recorder;
	MSFilter *ms_dtmfgen;
	MSFilter *ms_tonedet;
	MSFilter *ms_voidsource;
	MSFilter *ms_voidsink;
	char* local_ip;
	int local_port;
	char* remote_ip;
	int remote_port;
	str *callid;
} call_leg_media_t;

int create_call_leg_media(call_leg_media_t *m, str *callid);

int rms_media_init();
void rms_media_destroy();

MSFactory *rms_get_factory();

int rms_stop_media(call_leg_media_t *m);
int rms_playfile(call_leg_media_t *m, char* file_name);

#endif
