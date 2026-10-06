#ifndef H_SYSSERVERMSGS
#define H_SYSSERVERMSGS

 
#include <err.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#define limit_msgs 34

typedef struct sys_msgs{
	char *msg;
	size_t len;

} server_system_msg;

static const server_system_msg all_msgs[] = {
	// Common:
	{"[server]: Online: %d.\n", 21},																	// 0
	{"[server]: Only letters, length: 3-9 characters.\n", 48},											// 1
	{"[server]: You are in global chat.\n", 34},														// 2
	{"[server]: No such username online: %s.\n", 39},													// 3
	{"[server]: Message can't start from '['.\n", 40},													// 4
	{"[server]: Hello, please write your name.\n", 41},													// 5
	{"[server]: Welcome to the chat server: %s.\n", 42},												// 6
	{"[server]: Something wrong, disconnecting.\n", 42},												// 7
	{"[server]: Too much requests, limit is 10.\n", 42},												// 8
	{"[server]: This name is already taken, choose another one.\n", 58},								// 9
	{"[server]: Message is too long.\n", 31},                                                   		// 10
	{"[server]: You can't send a request to yourself or accept on from yourself.\n", 76},         		// 11                	
	// Private:
	{"[server]: Request for private chat from username: %s.\n", 54},									// 12
	{"[server]: Private chat started with username: %s.\n", 50},										// 13
	{"[server]: Private chat accepted with username: %s.\n", 51},										// 14
	{"[server]: No such private chat request or time for waiting 10s has elapsed. Username: %s\n", 89},	// 15
	{"[server]: User has left the chat, you can switch to /global or other /private chat\n", 83},		// 16
	// File:
	{"[server]: Request for file sending from username: %s\n", 53},										// 17
	{"[server]: File sending accepted from: %s.\n", 42},												// 18
	{"[server]: File sending accepted. Destination username: %s.\n", 59},								// 19
	{"[server]: No such file send request or time for waiting 10s has elapsed.\n", 73},					// 20
	{"[server]: File received from user: %s.\n", 39},													// 21
	{"[server]: File send to user: %s.\n", 33},															// 22
	{"[server]: File sending is not activated from your account.\n", 59},								// 23
	{"[server]: File sent to user: %s.\n", 33},															// 24
	{"[server]: You can send or receive only one file.\n", 49},											// 25
	// Shell:
	{"[server]: Request to provide shell browsing from username: %s.\n", 63},							// 26
	{"[server]: Shell browsing started with username: %s.\n", 52},										// 27
	{"[server]: Shell browsing started from username: %s.\n", 52},										// 28
	{"[server]: No such shell request or time for waiting 10s has elapsed.\n", 74},						// 29
	{"[server]: Incoming username: %s canceled the shell managing.\n", 61},								// 30
	{"[server]: Outcoming username: %s canceled the shell managing.\n", 62},							// 31
	{"[server]: You can use only one outcoming or incoming shell managing.\n", 70},					    // 32
	{"[server]: No such shell request.\n", 33},															// 33

			
	{NULL, 0}
};



char *get_info_server_system_msg(size_t idx, size_t *len);
char *get_server_system_msg(size_t idx, char *name, size_t ln, int num, size_t *ret_ln);



#endif