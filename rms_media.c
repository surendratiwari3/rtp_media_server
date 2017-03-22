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

int rms_media_init() {
	ortp_init();
	return 1;
}

static MSFactory * rms_create_factory() {
	MSFactory *ms_factory = ms_factory_new_with_voip();
	ms_factory_enable_statistics(ms_factory, TRUE);
	ms_factory_reset_statistics(ms_factory);
	return ms_factory;
}
//	ms_factory_destroy
static MSTicker * rms_create_ticker(char *name) {
	MSTickerParams params;
	params.name = name;
	params.prio = MS_TICKER_PRIO_NORMAL;
	return ms_ticker_new_with_params(&params);
}
//	ms_ticker_destroy(ms_tester_ticker);

void rms_media_destroy() {
//	ms_factory_destroy(ms_factory);
}

int create_call_leg_media(call_leg_media_t *m){
	m->ms_factory = rms_create_factory();

	// create caller RTP session
	m->rtps = ms_create_duplex_rtp_session(m->local_ip, m->local_port, m->local_port+1, ms_factory_get_mtu(m->ms_factory));
	rtp_session_set_remote_addr_full(m->rtps, m->remote_ip, m->remote_port, m->remote_ip, m->remote_port+1);
	rtp_session_set_payload_type(m->rtps, m->pt->type);
	rtp_session_enable_rtcp(m->rtps,FALSE);
	// create caller filters : rtprecv1/rtpsend1/encoder1/decoder1
	m->ms_rtprecv = ms_factory_create_filter(m->ms_factory, MS_RTP_RECV_ID);
	m->ms_rtpsend = ms_factory_create_filter(m->ms_factory, MS_RTP_SEND_ID);
	m->ms_encoder = ms_factory_create_encoder(m->ms_factory, m->pt->mime_type);
	m->ms_decoder = ms_factory_create_decoder(m->ms_factory, m->pt->mime_type);
	// set filter params
	ms_filter_call_method(m->ms_rtpsend, MS_RTP_SEND_SET_SESSION, m->rtps);
	ms_filter_call_method(m->ms_rtprecv, MS_RTP_RECV_SET_SESSION, m->rtps);
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
	m->ms_player = ms_factory_create_filter(m->ms_factory, MS_FILE_PLAYER_ID);
	//m->ms_recorder = ms_factory_create_filter(m->ms_factory, MS_FILE_PLAYER_ID);
	m->ms_voidsink = ms_factory_create_filter(m->ms_factory, MS_VOID_SINK_ID);
	ms_filter_add_notify_callback(m->ms_player, (MSFilterNotifyFunc) rms_player_eof, NULL, TRUE);

	ms_filter_call_method(m->ms_player, MS_FILE_PLAYER_OPEN, (void *) file_name);
	int channels = 1;
	ms_filter_call_method(m->ms_player, MS_FILTER_SET_OUTPUT_NCHANNELS, &channels);
	ms_filter_call_method_noarg(m->ms_player, MS_FILE_PLAYER_START);

	// sending graph
	ms_connection_helper_start(&h);
	ms_connection_helper_link(&h, m->ms_player, -1, 0);
	ms_connection_helper_link(&h, m->ms_encoder, 0, 0);
	ms_connection_helper_link(&h, m->ms_rtpsend, 0, -1);
	//ms_ticker_attach(m->ms_ticker, m->ms_player);

	// receiving graph
	ms_connection_helper_start(&h);
	ms_connection_helper_link(&h, m->ms_rtprecv, -1, 0);
	//ms_connection_helper_link(&h, m->ms_decoder, 0, 0);
	ms_connection_helper_link(&h, m->ms_voidsink, 0, -1);

	ms_ticker_attach_multiple(m->ms_ticker, m->ms_player, m->ms_rtprecv, NULL);


	return 1;
}

int rms_stop_media(call_leg_media_t *m) {
	if (!m->ms_ticker)
		return -1;
	if (m->ms_rtpsend) {
		ms_ticker_detach(m->ms_ticker, m->ms_rtpsend);
	}
	if (m->ms_rtprecv) {
		ms_ticker_detach(m->ms_ticker, m->ms_rtprecv);
	}
	rtp_stats_display(rtp_session_get_stats(m->rtps)," AUDIO SESSION'S RTP STATISTICS ");
	ms_factory_log_statistics(m->ms_factory);
	return 1;
}



