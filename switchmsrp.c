#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "msrp.h"


/* Callbacks from MSRP library */
void callback_msrp(int event, MsrpSwitch *switcher, unsigned short int user, void *data, int bytes);
void events_msrp(int event, void *info);
/* Helper to load and parse a configuration file */
int load_config_file(char *filename);
void show_menu(void);

/*
Global variables to store the configuration values:

	debug		= 0/1 (whether to show or not error and log event notifications)
	address		= the conference address
	port		= the conference port

MSRP URIs (both From-Path and To-Path) are formed as such:
	msrp://address:port/session:tcp
e.g.:
	msrp://atlanta.example.com:7654/jshA7weztas;tcp

*/
int debug, be_alive;
char *address, *welcome;
unsigned short int port;
char *display[100];
char buffer[65535];

int main(int argc, char *argv[])
{
	if(argc != 2) {
		printf("Usage: %s conffile (e.g. %s switch.conf)\n", argv[0], argv[0]);
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
	/* Setup the callback to receive switch related info */
	msrp_sw_callback(callback_msrp);

	/* Create a new switch instance */
	MsrpSwitch *conference = msrp_switch_new();
	if(!conference) {
		printf("Error creating new switch...\n");
		return -1;
	}
	printf("New switch (ID %lu) created:\n", conference->ID);

	/* Setup the MSRP server */
	if(msrp_switch_set_from(conference, address, port) < 0) {
		printf("Error creating new peer (%s:%hu) for switch...\n", address, port);
		return -1;
	}
	printf("\tConference Path:\t%s\n", msrp_switch_get_fullpath(conference));

	/*
	   Now, in real world, we would wait for SIP/SDP negotiation to add users...
	   Since this is an example, we'll ask these values on the console
	   to simulate negotiated peers to be our conference users:

		msrp://to_uri:to_port/to_session;tcp
	*/
	be_alive = 1;
	unsigned short int ID = 0, tempID = 0;
	int flags = MSRP_ACTIVE | MSRP_OVER_TCP;
	char *lineptr = NULL;
	char line[80];
	int choice = 0;
	char *path = NULL, *text = NULL;
	char *log = "\n ###########\n ### LOG ###\n ###########\n\n";
	strcpy(buffer, log);
	strcat(buffer, " *** ");
	strcat(buffer, welcome);
	strcat(buffer, "\n");
	show_menu();
	while(fgets(line, 79, stdin) != NULL) {
		lineptr = line;
		while(*lineptr == ' ' || *lineptr == '\t')
			++lineptr;
		choice = atoi(lineptr);
		switch(choice) {
			case 1:		/* Add user */
				printf("Insert the new user's nickname: ");
				scanf(" %a[^\n]", &text);
				printf("Insert the user's full 'To' path: ");
				scanf(" %a[^\n]", &path);
				ID = msrp_switch_add_user(conference, text, path, MSRP_TEXT_PLAIN, flags, MSRP_SENDRECV);
				if(!ID)
					printf("\tCouldn't add %s (%s) to the conference\n", text, path);
				else {
					display[ID-1] = calloc(strlen(text)+1, sizeof(char));
					strcpy(display[ID-1], text);
					printf("\tAdded %s (%s) to the conference with ID %hu\n", display[ID-1], path, ID);
				}
				break;
			case 2:		/* Mute user */
				tempID = 0;
				printf("Insert the user's number: ");
				scanf("%hu", &tempID);
				if(msrp_switch_update_user_rights(conference, tempID, MSRP_RECVONLY) < 0)
					printf("\tCouldn't mute user %hu (%s) in the conference\n", tempID, display[tempID-1]);
				else
					printf("\tMuted user %hu (%s) in the conference\n", tempID, display[tempID-1]);
				sprintf(line, "%s has been muted", display[tempID-1]);
				msrp_switch_announcement(conference, 0, line);
				strcat(buffer, " *** ");
				strcat(buffer, line);
				strcat(buffer, "\n");
				break;
			case 3:		/* Unmute user */
				tempID = 0;
				printf("Insert the user's number: ");
				scanf("%hu", &tempID);
				if(msrp_switch_update_user_rights(conference, tempID, MSRP_SENDRECV) < 0)
					printf("\tCouldn't unmute user %hu (%s) in the conference\n", tempID, display[tempID-1]);
				else
					printf("\tUnmuted user %hu (%s) in the conference\n", tempID, display[tempID-1]);
				sprintf(line, "%s has been unmuted", display[tempID-1]);
				msrp_switch_announcement(conference, 0, line);
				strcat(buffer, " *** ");
				strcat(buffer, line);
				strcat(buffer, "\n");
				break;
			case 4:		/* Remove user */
				tempID = 0;
				printf("Insert the user's number: ");
				scanf("%hu", &tempID);
				if(msrp_switch_remove_user(conference, tempID) < 0)
					printf("\tCouldn't remove user %hu (%s) from the conference\n", tempID, display[tempID-1]);
				else
					printf("\tRemoved user %hu (%s) from the conference\n", tempID, display[tempID-1]);
				display[ID-1] = NULL;
				break;
			case 5:		/* Play annoucement */
				printf("Insert the announcement text: ");
				scanf(" %a[^\n]", &text);
				if(msrp_switch_announcement(conference, 0, text) < 0)
					printf("\tCouldn't send annoucement on to conference\n");
				else
					printf("\tAnnouncement sent on the conference\n");
				strcat(buffer, " *** ");
				strcat(buffer, text);
				strcat(buffer, "\n");
				break;
			case 6:		/* Print log */
				printf("%s", buffer);
				printf("\n ###########\n ### END ###\n ###########\n\n");
				break;
			case 7:		/* Show info */
				printf("\n ############\n ### INFO ###\n ############\n\n");
				printf(" Conference Path:\t%s\n", msrp_switch_get_fullpath(conference));
				printf("\n ############\n ### INFO ###\n ############\n\n");
				break;
			case 8:		/* Destroy conference */
				be_alive = 0;
				printf("Destroying conference and closing connections...\n");
				break;
			case 9:		/* Show menu */
				show_menu();
				break;
			default:
				break;
		}
		if(!be_alive)
			break;
	}

	if(msrp_switch_destroy(conference) < 0)
		printf("Could not destroy conference...\n");
	sleep(1);

	msrp_quit();

	exit(0);
}


/* The callback to receive switch-related events from the MSRP library */
void callback_msrp(int event, MsrpSwitch *switcher, unsigned short int user, void *data, int bytes)
{
	if(event < 0)
		return;
	if(event == MSRP_CONFUSER_SEND) {
		char new[2000];
		sprintf(new, "<%s> %s\n", display[user-1] ? display[user-1] : "??",
			data ? (char *)data : "??");
		strcat(buffer, new);;
	} else if(event == MSRP_CONFUSER_JOIN) {
		char new[2000];
		sprintf(new, " --> User %d (%s) joined\n", user,
			data ? (char *)data : (display[user-1] ? display[user-1] : "??"));
		strcat(buffer, new);
		/* Play welcome message for this user only */
		if(msrp_switch_announcement(switcher, user, welcome) < 0)
			printf("\tCouldn't send welcome message to the user\n");
		/* Play an announcement for this too */
		memset(new, 0, 2000);
		sprintf(new, "%s joined", data ? (char *)data : (display[user-1] ? display[user-1] : "??"));
		if(msrp_switch_announcement(switcher, 0, new) < 0)
			printf("\tCouldn't send annoucement on to conference\n");
	} else if(event == MSRP_CONFUSER_LEAVE) {
		char new[2000];
		sprintf(new, " <-- User %d (%s) left\n", user,
			data ? (char *)data : (display[user-1] ? display[user-1] : "??"));
		strcat(buffer, new);
		/* Play an announcement for this too */
		memset(new, 0, 2000);
		sprintf(new, "%s left", data ? (char *)data : (display[user-1] ? display[user-1] : "??"));
		if(msrp_switch_announcement(switcher, 0, new) < 0)
			printf("\tCouldn't send annoucement on to conference\n");
	}
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

/* A helper to read and parse the application configuration file */
int load_config_file(char *filename)
{
	if(!filename)
		return -1;

	FILE *f = fopen(filename, "rt");
	if(!f) {
		printf("Couldn't open file %s...\n", filename);
		return -1;
	}
	printf("\nConfiguration file: %s\n", filename);

	debug = -1;
	address = NULL; port = 0;

	char buffer[500];
	char *var  = NULL, *value = NULL;
        while(fgets(buffer, 100, f) != NULL) {
		value = buffer;
		var = strsep(&value, "=");
		if(var) {
			if(!strcasecmp(var, "debug")) {
				debug = atoi(value);
				if(debug != 0)
					debug = 1;
				printf("\tDebug:\t\t%s\n", debug ? "yes" : "no");
			} else if(!strcasecmp(var, "welcome")) {
				welcome = calloc(strlen(value) + 1, sizeof(char));
				strncpy(welcome, value, strlen(value) - 1);
				printf("\tWelcome:\t%s\n", welcome);
			} else if(!strcasecmp(var, "address")) {
				address = calloc(strlen(value) + 1, sizeof(char));
				strncpy(address, value, strlen(value) - 1);
				printf("\tAddress:\t%s\n", address);
			} else if(!strcasecmp(var, "port")) {
				port = atoi(value);
				printf("\tPort:\t\t%hu\n", port);
			}
		}
        }
	fclose(f);

	if(debug < 0) {
		debug = 0;
		printf("\tNo 'debug' in %s, defaulting to '%s'\n", filename, debug ? "yes" : "no");
	}
	if(!welcome) {
		welcome = "Default welcome message";
		printf("\tNo 'welcome' in %s, defaulting to %s\n", filename, welcome);
	}
	if(!address) {
		address = "127.0.0.1";
		printf("\tNo 'address' in %s, defaulting to %s\n", filename, address);
	}
	if(!port) {
		printf("\tNo 'port' in %s, choosing random one\n", filename);
	}

	return 0;
}

void show_menu()
{
	printf("###\n");
	printf("1) Add a new user (fake SDP negotiation)\n");
	printf("2) Mute an user\n");
	printf("3) Unmute an user\n");
	printf("4) Remove an existing user\n");
	printf("5) Play room announcement\n");
	printf("6) Print room buffer (chat log)\n");
	printf("7) Show switch/conference info\n");
	printf("8) Destroy the conference room and quit\n");
	printf("9) Show menu\n");
	printf("###\n");
}
