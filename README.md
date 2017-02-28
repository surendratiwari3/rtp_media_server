# rtp_media_server

This module is implementing belledonne communiation media streamer(ms2) and RTP (oRTP) libraries in Kamailio
Kamailio is doing all the SIP and some SDP related task
MS2/oRTP is doing all the RTP and media processing

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
- implement : minimal IVR feature set : play, record, dtmf, transfer
- implement : IVR interface to feature : sql, xml, etc ?

t 
