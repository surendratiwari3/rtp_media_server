#include "../../sr_module.h"

// https://www.kamailio.org/dokuwiki/doku.php/development:write-module

MODULE_VERSION /* Module */

static int mod_init(void);
static void mod_destroy(void);
static int child_init(int);

/*
 * Script commands we export.
 */
static cmd_export_t cmds[]={
  {0,0,0,0,0}
};

/*
 * Script parameters
 */
static param_export_t mod_params[]={
  { 0,0,0 }
};

/*
 * Export the statistics we have
 */
static stat_export_t mod_stats[] = {
  {0,0,0}
};

struct module_exports exports= {
  "media",	/* module's name */
  cmds,         /* exported functions */
  mod_params,   /* param exports */
  mod_stats,    /* exported statistics */
  mod_init,      /* module initialization function */
  0,            /* reply processing function FIXME Not sure when this is used */
  mod_destroy,   /* Destroy function */
  child_init     /* per-child init function */
};

/**
 * @return 0 to continue to load the OpenSER, -1 to stop the loading
 * and abort OpenSER.
 */
static int mod_init(void) {
	LM_INFO("RTP media server module init\n");
	return(0);
}

/**
 * Called only once when OpenSER is shuting down to clean up module
 * resources.
 */
static void mod_destroy() {
	LM_INFO("RTP media server module destroy\n");
	return;
}

/**
 * The rank will be o for the main process calling this function,
 * or 1 through n for each listener process. The rank can have a negative
 * value if it is a special process calling the child init function.
 * Other then the listeners, the rank will equal one of these values:
 * PROC_MAIN      0  Main ser process
 * PROC_TIMER    -1  Timer attendant process 
 * PROC_FIFO     -2  FIFO attendant process
 * PROC_TCP_MAIN -4  TCP main process
 * PROC_UNIXSOCK -5  Unix domain socket server processes
 *
 * If this function returns a nonzero value the loading of OpenSER will
 * stop.
 */
static int childInit(int rank) {
	int rtn = 0;
	return(rtn);
}
