# rtp_media_server
The RTP media server Kamailio module :

Transcoding gateway with audio quality reporting

- Support for most freecodecs(Opus, Codec2) and (g729a, g722)
- Audio quality reporting using

RTP Control Protocol Extended Reports (RTCP XR)
https://tools.ietf.org/html/rfc3611

Session Initiation Protocol Event Package for Voice Quality Reporting
https://tools.ietf.org/html/rfc6035


# external libraries
This module is using belledonne communication media streamer(ms2) and RTP (oRTP) libraries in Kamailio

Kamailio is doing all the SIP and some SDP related task

MS2/oRTP is doing all the RTP and media processing


# routing script example

### call bridging
```
modparam("rtp_media_server", "server_address", "127.0.0.102")
modparam("rtp_media_server", "log_file_name", "/tmp/rms_transfer.log")

onreply_route[rms_reply] {
	if (has_body("application/sdp")) {
		rms_sessions_dump();     # dump the call-id of each active session
		rms_sdp_answer();
	}
}

request_route {
	if (is_method("INVITE") && !has_totag() && has_body("application/sdp")) {
		record_route();
		rewritehostport("127.0.0.101:5060");
		t_on_reply("rms_reply");
		if(!rms_sdp_offer()) {
			xlog("L_ERR","rtp_media_server transfer error!");
		}
		rms_sessions_dump();     # dump the call-id of each active session
	}
	if (is_method("ACK") && has_body("application/sdp"))
		rms_sdp_answer();

	if (is_method("BYE")){
		rms_sessions_dump();    # dump the call-id of each active media session
		rms_media_stop();
	}
}
```


### This is only for initial proof of concept to start an RTP session and playback a file
```
        if(is_method("INVITE") && !has_totag()) {
                xlog("L_INFO","INVITE RECEIVED [$ci]\n");
                t_check_trans();         # drop retransmissions and start message parsing
                rms_sessions_dump();     # dump the call-id of each active session
                if(!rms_media_offer()) {
                        xlog("L_ERR","rtp_media_server error!");
                }
        }

        if(is_method("BYE")){
                xlog("L_INFO","BYE RECEIVED [$ci]\n");
                t_check_trans();        # drop retransmissions and start message parsing
                rms_sessions_dump();    # dump the call-id of each active media session
                rms_media_stop();
        }
```

### work in progress / to be done

- SDP parsing / payload offer-answer
- process synchronization
- Memory leak check refactoring
- implement : transcoding capabilities
- implement : SIP audio quality reporting

### transcoding gateway

```
1) receive INVITE
create caller filters : rtprecv1/rtpsend1/encoder1/decoder1
2) send INVITE / receive 200 OK
create caller filters : rtprecv2/rtpsend2/encoder2/decoder2
3) create the sending graph (ms_filter_link)
rtprecv1/decoder1/encoder2/rtpsend2
4) create the receiving graph (ms_filter_link)
rtprecv2/decoder2/encoder1/rtpsend1
5) attach the graphs to the ticker (ms_ticker_attach_multiple)

rtp_session_set_remote_addr_full(rtps,rem_rtp_ip,rem_rtp_port,rem_rtcp_ip,rem_rtcp_port);
ms_filter_call_method(stream->ms.rtpsend,MS_RTP_SEND_SET_SESSION,rtps);
stream->ms.rtprecv=ms_factory_create_filter(stream->ms.factory,MS_RTP_RECV_ID);
stream->soundread = ms_factory_create_filter(stream->ms.factory, MS_RTP_RECV_ID);
ms_filter_call_method(stream->soundread, MS_RTP_RECV_SET_SESSION, stream->rtp_io_session);
stream->read_decoder = ms_factory_create_decoder(stream->ms.factory, pt->mime_type);
```
