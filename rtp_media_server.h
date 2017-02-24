
typedef struct rms_sdp_info {
	char * remote_ip;
	char * payloads;
	char * remote_port;
	int ipv6;
} rms_sdp_info_t;

typedef struct rms_session_info {
	struct rms_session_info* next;
	struct rms_session_info* prev;
	rms_sdp_info_t sdp_info;
	char * session_id;
} rms_session_info_t;


//SLIST_HEAD(session_pool_head, rms_session_info);
//struct session_pool_head session_pool = SLIST_HEAD_INITIALIZER(session_pool);
