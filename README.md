# rtp_media_server

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
