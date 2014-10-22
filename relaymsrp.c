#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "msrp.h"


/* Callbacks from MSRP library */
void callback_msrp(MsrpRelay *relay);
void events_msrp(int event, void *info);
/* Helper to load and parse a configuration file */
int load_config_file(char *filename);


int main(int argc, char *argv[])
{
	if(argc != 2) {
		printf("Usage: %s conffile (e.g. %s relay.conf)\n", argv[0], argv[0]);
		return -1;
	}
	if(load_config_file(argv[1]) < 0)
		return -1;

	/* Initialize the library, by passing the callback to receive events */
	if(msrp_init(events_msrp) < 0) {
		printf("Error initializing the MSRP library...\n");
		return -1;
	}
	printf("\nMSRP library initialized.\n");
	/* Setup the callback to receive relay related info */
	msrp_rl_callback(callback_msrp);

	msrp_quit();

	exit(0);
}

/* The callback to receive incoming events from the library */
void events_msrp(int event, void *info)
{
	if(!debug || (event == MSRP_NONE))
		return;

	/* Debug text notifications */
	char *head = NULL;
	if(event == MSRP_LOG)
		head = "MSRP_LOG";
	else if(event == MSRP_ERROR)
		head = "MSRP_ERROR";
	else
		return;

	printf("[%s] %s\n", head ? head : "??", info ? (char *)info : "??");
}
