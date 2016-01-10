/*	filename:	disp.c
 *	date:		14 february 2007
 *	version:	1.3
 *	author:		w.g.oosterveld
 *	e-mail:		w.g.oosterveld@greenmont.nl
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "dispd.h"

FILE *disp_fifo;

struct options {
	int alarmledon;
	int alarmledoff;
	int powerledon;
	int powerledoff;
	int overlay1;
	int overlay2;
	int info1;
	int info2;
	int menu1;
	int menu2;
	int exitboot;
	int enteroverlay;
	int exitoverlay;
	int entermenu;
	int exitmenu;
	int buttonon;
	int buttonoff;
};

/* some globals */
struct options op;
struct ipsecureit_display disp;

void display_help() {
	printf("IPsecureIT display controller v1.3\n");
	printf("(c) 2007 by GreenMont Systems Nederland\n\n");
	printf("usage: disp <option>\n\n");
	printf("  options are:\n\n");
	printf("  --alarmled=[on|off]     turn alarm led on or off\n");
	printf("  --powerled=[on|off]     turn power led on or off\n");
	printf("  --overlay1 \"string\"    set line 1 of overlay mode to \"string\"\n");
	printf("  --overlay2 \"string\"    set line 1 of overlay mode to \"string\"\n");
	printf("  --info1 \"string\"       set line 1 of info mode to \"string\"\n");
	printf("  --info2 \"string\"       set line 1 of info mode to \"string\"\n");
	printf("  --menu1 \"string\"       set line 1 of menu mode to \"string\"\n");
	printf("  --menu2 \"string\"       set line 1 of menu mode to \"string\"\n");
	printf("  --exitboot              boot is finished, enter info mode\n");
	printf("  --enteroverlay	  enter overlay mode\n");
	printf("  --exitoverlay		  exit overlay mode, enter info mode\n");
	printf("  --entermenu             enter menu mode\n");
	printf("  --exitmenu              exit menu mode, enter info mode\n");
	printf("  --buttonon              enable the buttons\n");
	printf("  --buttonoff             disable the buttons\n");
	printf("\nAll \"string\"s will be chopped to 20 bytes\n");
}	

void do_work() {
	/* check sanity */
	if ((op.alarmledon == 1) && (op.alarmledoff == 1)) {
		printf("Alarm LED can not be on and off at the same time\n");
		exit(0);
	}
	if ((op.powerledon == 1) && (op.powerledoff == 1)) {
		printf("Power LED can not be on and off at the same time\n");
		exit(0);
	}
	if ((op.enteroverlay == 1) && (op.exitoverlay== 1)) {
		printf("State machine needs a day off\n");
		exit(0);
	}
	if ((op.entermenu == 1) && (op.exitmenu == 1)) {
		printf("Guru meditation error #00000001\n");
		exit(0);
	}

	if (op.alarmledon == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_LEDALARMON;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}

	if (op.alarmledoff == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_LEDALARMOFF;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}

	if (op.powerledon == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_LEDPOWERON;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}

	if (op.powerledoff == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_LEDPOWEROFF;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}

	if (op.exitboot == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_BOOTFINISHED;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}
	if (op.enteroverlay == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_ENTEROVERLAY;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}

	if (op.exitoverlay == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_EXITOVERLAY;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}
	if (op.entermenu == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_ENTERMENU;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}
	if (op.exitmenu == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_EXITMENU;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}
	if (op.buttonon == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_BUTTONREADON;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}
	if (op.buttonoff == 1) {
		char send[2];
		bzero(send, 2);
		send[0] = IPC_BUTTONREADOFF;
		send[1] = '\n';
		fwrite((void *)send, 1, 2, disp_fifo);
	}
	if (op.info1 == 1) {
		char send[BUFFER_LINE_SIZE +3];
		bzero(send, BUFFER_LINE_SIZE +3);
		send[0] = IPC_SETINFOLINE1;
		strncpy(&(send[1]), disp.info_line_1,
				strlen(disp.info_line_1));
		send[BUFFER_LINE_SIZE +2] = '\n';
		fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	}
	if (op.info2 == 1) {
		char send[BUFFER_LINE_SIZE +3];
		bzero(send, BUFFER_LINE_SIZE +3);
		send[0] = IPC_SETINFOLINE2;
		strncpy(&(send[1]), disp.info_line_2,
				strlen(disp.info_line_2));
		send[BUFFER_LINE_SIZE +2] = '\n';
		fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	}
	if (op.menu1 == 1) {
		char send[BUFFER_LINE_SIZE +3];
		bzero(send, BUFFER_LINE_SIZE +3);
		send[0] = IPC_SETMENULINE1;
		strncpy(&(send[1]), disp.menu_line_1,
				strlen(disp.menu_line_1));
		send[BUFFER_LINE_SIZE +2] = '\n';
		fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	}
	if (op.menu2 == 1) {
		char send[BUFFER_LINE_SIZE +3];
		bzero(send, BUFFER_LINE_SIZE +3);
		send[0] = IPC_SETMENULINE2;
		strncpy(&(send[1]), disp.menu_line_2,
				strlen(disp.menu_line_2));
		send[BUFFER_LINE_SIZE +2] = '\n';
		fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	}
	if (op.overlay1 == 1) {
		char send[BUFFER_LINE_SIZE +3];
		bzero(send, BUFFER_LINE_SIZE +3);
		send[0] = IPC_SETOVERLAYLINE1;
		strncpy(&(send[1]), disp.overlay_line_1,
				strlen(disp.overlay_line_1));
		send[BUFFER_LINE_SIZE +2] = '\n';
		fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	}
	if (op.overlay2 == 1) {
		char send[BUFFER_LINE_SIZE +3];
		bzero(send, BUFFER_LINE_SIZE +3);
		send[0] = IPC_SETOVERLAYLINE2;
		strncpy(&(send[1]), disp.overlay_line_2,
				strlen(disp.overlay_line_2));
		send[BUFFER_LINE_SIZE +2] = '\n';
		fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	}
}


int main(int argc, char **argv) {
	int i;
	struct stat tmp;

	/* init to zero */
	op.alarmledon = 0;
	op.alarmledoff = 0;
	op.powerledon = 0;
	op.powerledoff = 0;
	op.overlay1 = 0;
	op.overlay2 = 0;
	op.info1 = 0;
	op.info2 = 0;
	op.menu1 = 0;
	op.menu2 = 0;
	op.exitboot = 0;
	op.enteroverlay = 0;
	op.exitoverlay = 0;
	op.entermenu = 0;
	op.exitmenu = 0;
	op.buttonon = 0;
	op.buttonoff = 0;

	if (argc == 1) {
		display_help();
		exit(0);
	}

	/* parse command line */
	for (i = 1; i < argc; i++) {
		/* parse */
		if (strcmp(argv[i], "--alarmled=on") == 0) {
			op.alarmledon = 1;
		} else if (strcmp(argv[i], "--alarmled=off") == 0) {
			op.alarmledoff = 1;
		} else if (strcmp(argv[i], "--powerled=on") == 0) {
			op.powerledon = 1;
		} else if (strcmp(argv[i], "--powerled=off") == 0) {
			op.powerledoff = 1;
		} else if (strcmp(argv[i], "--exitboot") == 0) {
			op.exitboot = 1;
		} else if (strcmp(argv[i], "--enteroverlay") == 0) {
			op.enteroverlay = 1;
		} else if (strcmp(argv[i], "--exitoverlay") == 0) {
			op.exitoverlay = 1;
		} else if (strcmp(argv[i], "--entermenu") == 0) {
			op.entermenu = 1;
		} else if (strcmp(argv[i], "--exitmenu") == 0) {
			op.exitmenu = 1;
		} else if (strcmp(argv[i], "--buttonon") == 0) {
			op.buttonon = 1;
		} else if (strcmp(argv[i], "--buttonoff") == 0) {
			op.buttonoff = 1;
		} else if (strcmp(argv[i], "--info1") == 0) {
			int len = 0;
			if ((len = strlen(argv[i+1])) > 20) len = 20;
			strncpy(disp.info_line_1, argv[i+1], len);
			op.info1 = 1;
			break;
		} else if (strcmp(argv[i], "--info2") == 0) {
			int len = 0;
			if ((len = strlen(argv[i+1])) > 20) len = 20;
			strncpy(disp.info_line_2, argv[i+1], len);
			op.info2 = 1;
			break;
		} else if (strcmp(argv[i], "--menu1") == 0) {
			int len = 0;
			if ((len = strlen(argv[i+1])) > 20) len = 20;
			strncpy(disp.menu_line_1, argv[i+1], len);
			op.menu1 = 1;
			break;
		} else if (strcmp(argv[i], "--menu2") == 0) {
			int len = 0;
			if ((len = strlen(argv[i+1])) > 20) len = 20;
			strncpy(disp.menu_line_2, argv[i+1], len);
			op.menu2 = 1;
			break;
		} else if (strcmp(argv[i], "--overlay1") == 0) {
			int len = 0;
			if ((len = strlen(argv[i+1])) > 20) len = 20;
			strncpy(disp.overlay_line_1, argv[i+1], len);
			op.overlay1 = 1;
			break;
		} else if (strcmp(argv[i], "--overlay2") == 0) {
			int len = 0;
			if ((len = strlen(argv[i+1])) > 20) len = 20;
			strncpy(disp.overlay_line_2, argv[i+1], len);
			op.overlay2 = 1;
			break;
		} else {
			display_help();
			exit(0);
		} /* big if-else */
	} /* for */

	/* check if FIFO exists */
	if (stat(FIFO, &tmp) == -1) {
		/* no fifo, exit */
		printf("No display driver found. Check if dispd is running.\n");
		printf("%s\n", strerror(errno));
		exit(errno);
	}

	/* open fifo for writing */
	if ((disp_fifo = fopen(FIFO, "w+")) == NULL) {
		printf("Cannot open fifo for writing: %s\n",
				strerror(errno));
		exit(errno);
	}

	do_work();
	fclose(disp_fifo);
	
	return 0;
}
