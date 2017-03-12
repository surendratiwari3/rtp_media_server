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

#include "rms_media.h"

static MSFactory *ms_factory;

int rms_media_init() {
	ortp_init();
	ms_factory = ms_factory_new_with_voip();
	return 1;
}

static MSTicker * rms_create_ticker(char *name) {
	MSTickerParams params;
	params.name = name;
	params.prio = MS_TICKER_PRIO_NORMAL;
	return ms_ticker_new_with_params(&params);
}
//	ms_ticker_destroy(ms_tester_ticker);

void rms_media_destroy() {
	ms_factory_destroy(ms_factory);
}

MSFactory *rms_get_factory() {
	return ms_factory;
}

int create_call_leg_media(call_leg_media_t *m){
	// create caller RTP session
	m->rtps = ms_create_duplex_rtp_session(m->local_ip, m->local_port, m->local_port+1, ms_factory_get_mtu(ms_factory));
	rtp_session_set_remote_addr_full(m->rtps, m->remote_ip, m->remote_port, m->remote_ip, m->remote_port+1);
	rtp_session_set_payload_type(m->rtps, 8);
	rtp_session_enable_rtcp(m->rtps,FALSE);
	// create caller filters : rtprecv1/rtpsend1/encoder1/decoder1

	m->ms_rtprecv = ms_factory_create_filter(ms_factory, MS_RTP_RECV_ID);
	m->ms_rtpsend = ms_factory_create_filter(ms_factory, MS_RTP_SEND_ID);
	m->ms_encoder = ms_factory_create_encoder(ms_factory, m->pt->mime_type);
	m->ms_decoder = ms_factory_create_decoder(ms_factory, m->pt->mime_type);
	// set filter params
	ms_filter_call_method(m->ms_rtpsend, MS_RTP_SEND_SET_SESSION, m->rtps);
	ms_filter_call_method(m->ms_rtprecv, MS_RTP_RECV_SET_SESSION, m->rtps);
	// linking filters
	//MSConnectionHelper h;
	//ms_connection_helper_start(&h);
	//ms_connection_helper_link(&h, ms_tester_voidsource, -1, 0);
	//ms_connection_helper_link(&h, ms_tester_dtmfgen, 0, 0);
	//ms_connection_helper_link(&h, ms_tester_encoder, 0, 0);
	//ms_connection_helper_link(&h, ms_tester_rtpsend, 0, -1);
	//ms_connection_helper_start(&h);
	//ms_connection_helper_link(&h, ms_tester_rtprecv, -1, 0);
	//ms_connection_helper_link(&h, ms_tester_decoder, 0, 0);
	//ms_connection_helper_link(&h, ms_tester_tonedet, 0, 0);
	//ms_connection_helper_link(&h, ms_tester_voidsink, 0, -1);
	//ms_ticker_attach_multiple(ms_tester_ticker, ms_tester_voidsource, ms_tester_rtprecv, NULL);
	return 1;
}

#define MS_UNUSED(x) ((void)(x))
static void rms_player_eof(void *user_data, MSFilter *f, unsigned int event, void *event_data) {
	if (event == MS_FILE_PLAYER_EOF) {
		int *done = (int *)user_data;
		*done = TRUE;
	}
	MS_UNUSED(f), MS_UNUSED(event_data);
}

int rms_playfile(call_leg_media_t *m, const char* file_name) {
	MSConnectionHelper h;
	m->ms_ticker = rms_create_ticker(NULL);
	ms_filter_add_notify_callback(m->ms_player, rms_player_eof, NULL, TRUE);
	ms_filter_call_method_noarg(m->ms_player, MS_FILE_PLAYER_START);
	ms_filter_call_method(m->ms_player, MS_FILE_PLAYER_OPEN, (void *) file_name);

	ms_connection_helper_start(&h);
	ms_connection_helper_link(&h, m->ms_player, -1, 0);
	ms_connection_helper_link(&h, m->ms_encoder, 0, 0);
	ms_connection_helper_link(&h, m->ms_rtpsend, 0, -1);

	ms_ticker_attach(m->ms_ticker, m->ms_player);

	return 1;
}


