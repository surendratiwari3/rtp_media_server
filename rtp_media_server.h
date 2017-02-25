#include "../../sr_module.h"
#include "../../parser/sdp/sdp_helpr_funcs.h"
#include "../../parser/parse_content.h"
#include "../../modules/tm/tm_load.h"
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

struct tm_binds tmb;
typedef struct rms_sdp_info {
	char * remote_ip;
	char * payloads;
	char * remote_port;
	int ipv6;
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
