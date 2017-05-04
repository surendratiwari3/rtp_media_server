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

