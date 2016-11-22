#include "cmdparser.h"
#include "debug.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

extern bool _kill;

static int parse_cmd(char* string);

int cmdloop(void) {
	int cmd = INVALID;
	char stringBuffer[128]; //Unsafe, but who cares
	memset(stringBuffer, 0, 64);

	while (cmd == INVALID) {
		printf(">>> ");
		fgets(stringBuffer, 128, stdin);

		if (_kill) {
			return STOP;
		}

		for (int i = 0; stringBuffer[i]; i++) {
			stringBuffer[i] = tolower(stringBuffer[i]);
		}

		stringBuffer[strcspn(stringBuffer, "\r\n")] = 0; //remove trailing newline

		cmd = parse_cmd(stringBuffer);

		if (cmd == INVALID) {
			printf("Invalid command, try 'help'.\n");
		}
	}

	return cmd;
}

static int parse_cmd(char* string) {
	if (strcmp(string, "connect") == 0) {
		return CONNECT;
	} else if (strcmp(string, "start") == 0) {
		return START;
	} else if (strcmp(string, "run") == 0) {
		return START;
	} else if (strcmp(string, "stop") == 0) {
		return STOP;
	} else if (strcmp(string, "help") == 0) {
		return HELP;
	} else if (strcmp(string, "quit") == 0) {
		return QUIT;
	} else if (strcmp(string, "exit") == 0) {
		return QUIT;
	} else {
		return INVALID;
	}
}