/*	filename:	dispd.c
 *	date:		14 february 2007
 *	version:	1.3
 *	author:		w.g.oosterveld
 *	e-mail:		w.g.oosterveld@greenmont.nl
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

#include "dispd.h"

int fd_display;		/* file descriptor for display */
FILE *disp_fifo;	/* STREAM descriptor for fifo */

/* The display device has to be opened and closed after each command,
 * otherwise the AVR microcontroller spawns 'err' messages and locks up.
 * A test for the device is done in the main() loop. If it succeeds, the
 * state machine is entered and valid operation of the display is assumed.
 */

/* This program includes code from ttydevinit.c, which can be found at:
 *
 *  http://www.linuxfocus.org/common/src/c/article236/c/ttydevinit.c.html
 */

/* our general display state memory, see dispd.h */
struct ipsecureit_display disp;

/* for the state machine */
int next_state = STATE_INIT, state = STATE_INIT;

/* for timing */
struct itimerval itimer;
struct sigaction sact;

/* forward declarations */
void display_write_lines();
void display_set_leds();
void button_read(int);

void signal_handler() {
	if (disp.power_led == LED_ON) {
		disp.power_led = LED_OFF;
	} else {
		disp.power_led = LED_ON;
	}
	display_set_leds();
	if (state == STATE_BOOT) {
		alarm(1);
	}
}

void wait_for_input() {
	/* Input is always in the form: <1-byte opcode>[parameters]. The
	 * total length is at most BUFFER_LINE_SIZE plus 1 byte opcode,
	 * plus end-of-line plus eventually end-of-file
	 */
	
	char readbuffer[BUFFER_LINE_SIZE+3];

	/* open the fifo for IPC */
	if ((disp_fifo = fopen(FIFO, "r")) == NULL) {
		printf("Cannot open fifo: %s\n", strerror(errno));
		exit(errno);
	}
	
	/* read from named pipe */
	fgets(readbuffer, BUFFER_LINE_SIZE+3, disp_fifo);

	/* switch over commands */
	switch(readbuffer[0]) {
		case IPC_BOOTFINISHED: {
			if (state == STATE_BOOT) {
				next_state = STATE_INFO;
				disp.power_led = LED_ON;
				alarm(0);
				display_set_leds();
			}
			break;
		}
		case IPC_ENTEROVERLAY: {
			if (state == STATE_INFO) {
				next_state = STATE_OVERLAY;
			}
			break;
		}
		case IPC_EXITOVERLAY: {
			if (state == STATE_OVERLAY) {
				next_state = STATE_INFO;
			}
			break;
		}
		case IPC_ENTERMENU: {
			if (state == STATE_INFO || state == STATE_OVERLAY) {
				next_state = STATE_MENU;
			}
			break;
		}
		case IPC_EXITMENU: {
			if (state == STATE_MENU) {
				next_state = STATE_INFO;
			}
			break;
		}
		case IPC_LEDPOWERON: {
			disp.power_led = LED_ON;
			display_set_leds();
			break;
		}
		case IPC_LEDPOWEROFF: {
			disp.power_led = LED_OFF;
			display_set_leds();
			break;
		}
		case IPC_LEDALARMON: {
			disp.alarm_led = LED_ON;
			display_set_leds();
			break;
		}
		case IPC_LEDALARMOFF: {
			disp.alarm_led = LED_OFF;
			display_set_leds();
			break;
		}
		case IPC_SETOVERLAYLINE1: {
			int len, i;
			if ((len = strlen(&(readbuffer[1]))) > 20) len = 20;
			strncpy(disp.overlay_line_1, &(readbuffer[1]), len);
			for(i = 0; i < (BUFFER_LINE_SIZE-len); i++) {
				disp.overlay_line_1[len+i] = ' ';
			}
			break;
		}
		case IPC_SETOVERLAYLINE2: {
			int len, i;
			if ((len = strlen(&(readbuffer[1]))) > 20) len = 20;
			strncpy(disp.overlay_line_2, &(readbuffer[1]), len);
			for(i = 0; i < (BUFFER_LINE_SIZE-len); i++) {
				disp.overlay_line_2[len+i] = ' ';
			}
			break;
		}
		case IPC_SETMENULINE1: { 
			int len, i;
			if ((len = strlen(&(readbuffer[1]))) > 20) len = 20;
			strncpy(disp.menu_line_1, &(readbuffer[1]), len);
			for(i = 0; i < (BUFFER_LINE_SIZE-len); i++) {
				disp.menu_line_1[len+i] = ' ';
			}
			break;
		}
		case IPC_SETMENULINE2: {
			int len, i;
			if ((len = strlen(&(readbuffer[1]))) > 20) len = 20;
			strncpy(disp.menu_line_2, &(readbuffer[1]), len);
			for(i = 0; i < (BUFFER_LINE_SIZE-len); i++) {
				disp.menu_line_2[len+i] = ' ';
			}
			break;
		}
		case IPC_SETINFOLINE1: {
			int len, i;
			if ((len = strlen(&(readbuffer[1]))) > 20) len = 20;
			strncpy(disp.info_line_1, &(readbuffer[1]), len);
			for(i = 0; i < (BUFFER_LINE_SIZE-len); i++) {
				disp.info_line_1[len+i] = ' ';
			}
			break;
		}
		case IPC_SETINFOLINE2: {
			int len, i;
			if ((len = strlen(&(readbuffer[1]))) > 20) len = 20;
			strncpy(disp.info_line_2, &(readbuffer[1]), len);
			for(i = 0; i < (BUFFER_LINE_SIZE-len); i++) {
				disp.info_line_2[len+i] = ' ';
			}
			break;
		}
		case IPC_BUTTONREADON: {
			button_read(1);
			break;
		}
		case IPC_BUTTONREADOFF: {
			button_read(0);
			break;
		}
	} /* switch */
	fclose(disp_fifo);
}

void button_read(int on) {
	int fd, size;
	
	/* create a string for the display */
	char DISPLAY_COMMAND[BUFFER_LINE_SIZE +3];

	bzero(DISPLAY_COMMAND, BUFFER_LINE_SIZE +3);
	if (on) {
		strcat(DISPLAY_COMMAND, BUTTON_READ_ON);
	} else {
		strcat(DISPLAY_COMMAND, BUTTON_READ_OFF);
	}
	fd = open(TTYDISPLAY, O_RDWR | O_NOCTTY);
	size = write(fd, DISPLAY_COMMAND, strlen(DISPLAY_COMMAND));
	if (size == -1) {
		printf("%s", strerror(errno));
		exit(errno);
	}
	close(fd);
}

void display_write_lines() {
	int fd, size;

	/* create a string for the display */
	char DISPLAY_COMMAND[BUFFER_LINE_SIZE +3];
	
	/* clear display, set cursor home */
	bzero(DISPLAY_COMMAND, BUFFER_LINE_SIZE +3);
	strcat(DISPLAY_COMMAND, CONTROL_CLEAR);
	fd = open(TTYDISPLAY, O_RDWR | O_NOCTTY);
	size = write(fd, DISPLAY_COMMAND, strlen(DISPLAY_COMMAND));
	if (size == -1) {
		printf("%s", strerror(errno));
		exit(errno);
	}
	close(fd);

	/* wait some time */
	
	/* write first line */
	bzero(DISPLAY_COMMAND, BUFFER_LINE_SIZE +3);
	strcat(DISPLAY_COMMAND, DISPLAY_PREFIX_CLEAR); /* defensive */

	/* switch over the state to select correct buffers */
	if (state == STATE_INIT || state == STATE_BOOT ||
			state == STATE_OVERLAY) { 
		strncat(DISPLAY_COMMAND, disp.overlay_line_1,
				(size_t)BUFFER_LINE_SIZE);
	} else if (state == STATE_MENU) {
		strncat(DISPLAY_COMMAND, disp.menu_line_1,
				(size_t)BUFFER_LINE_SIZE);
	} else if (state == STATE_INFO) {
		strncat(DISPLAY_COMMAND, disp.info_line_1,
				(size_t)BUFFER_LINE_SIZE);
	}
	strncat(DISPLAY_COMMAND, "\n", 1);
	fd = open(TTYDISPLAY, O_RDWR);
	size = write(fd, DISPLAY_COMMAND, strlen(DISPLAY_COMMAND));
	close(fd);

	/* set cursor on second line */
#if 1
	bzero(DISPLAY_COMMAND, BUFFER_LINE_SIZE +3);
	strcat(DISPLAY_COMMAND, CURSOR_SECONDLINE);
	fd = open(TTYDISPLAY, O_RDWR);
	size = write(fd, DISPLAY_COMMAND, strlen(DISPLAY_COMMAND));
	close(fd);

	/* write buffer 2 */
	bzero(DISPLAY_COMMAND, BUFFER_LINE_SIZE +3);
	strcat(DISPLAY_COMMAND, DISPLAY_PREFIX);

	/* switch over the state to select correct buffers */
	if (state == STATE_INIT || state == STATE_BOOT
			|| state == STATE_OVERLAY)	{ 
		strncat(DISPLAY_COMMAND, disp.overlay_line_2,
				(size_t)BUFFER_LINE_SIZE);
	} else if (state == STATE_MENU) {
		strncat(DISPLAY_COMMAND, disp.menu_line_2,
				(size_t)BUFFER_LINE_SIZE);
	} else if (state == STATE_INFO) {
		strncat(DISPLAY_COMMAND, disp.info_line_2,
				(size_t)BUFFER_LINE_SIZE);
	}
	
	strncat(DISPLAY_COMMAND, "\n", 1);
	fd = open(TTYDISPLAY, O_RDWR);
	size = write(fd, DISPLAY_COMMAND, strlen(DISPLAY_COMMAND));
	close(fd);
#endif

}
	
void display_set_leds() {
	int fd, size;
	char DISPLAY_COMMAND[BUFFER_LINE_SIZE +3];

	/* set power led */
	bzero(DISPLAY_COMMAND, BUFFER_LINE_SIZE +3);
	if (disp.power_led == LED_ON) {
		strcat(DISPLAY_COMMAND, LED_POWER_ON);
	} else if (disp.power_led == LED_OFF) {
		strcat(DISPLAY_COMMAND, LED_POWER_OFF);
	} else {
		strcat(DISPLAY_COMMAND, LED_POWER_OFF);
	}
	strncat(DISPLAY_COMMAND, "\n", 1);
	fd = open(TTYDISPLAY, O_RDWR);
	size = write(fd, DISPLAY_COMMAND, strlen(DISPLAY_COMMAND));
	close(fd);	

	/* set alarm led */
	bzero(DISPLAY_COMMAND, BUFFER_LINE_SIZE +3);
	if (disp.alarm_led == LED_ON) {
		strcat(DISPLAY_COMMAND, LED_ALARM_ON);
	} else if (disp.alarm_led == LED_OFF) {
		strcat(DISPLAY_COMMAND, LED_ALARM_OFF);
	} else {
		strcat(DISPLAY_COMMAND, LED_ALARM_OFF);
	}
	strncat(DISPLAY_COMMAND, "\n", 1);
	fd = open(TTYDISPLAY, O_RDWR);
	size = write(fd, DISPLAY_COMMAND, strlen(DISPLAY_COMMAND));
	close(fd);	
}
	
void state_machine( void ) {

	while((state = next_state) != STATE_ERROR) {
		switch(state) {
		case STATE_INIT:
		{
			/* The init state sets the serial port in the
			 * right mode to read and write from it. After
			 * manual tinkering with the port, it will always
			 * be set correctly after passing this state. This
			 * code is from ttydevinit.c, as referenced in the
			 * header of this file
			 */
			struct termios portset;
			
			/* open the port */
			fd_display = open(TTYDISPLAY, O_RDWR | O_NOCTTY | O_NDELAY);
			/* read initial parameter from tty */
			tcgetattr(fd_display, &portset);

			/* set default parameters, see man(3)tcgetattr */
			cfmakeraw(&portset);

			/* set baudrate */
			cfsetospeed(&portset, B9600);

			/* save the parameters to the tty */
			tcsetattr(fd_display, TCSANOW, &portset);
			close(fd_display);

			/* nullify the disp structure, see dispd.h */
			bzero(disp.info_line_1, BUFFER_LINE_SIZE);
			bzero(disp.info_line_2, BUFFER_LINE_SIZE);
			bzero(disp.overlay_line_1 ,BUFFER_LINE_SIZE);
			bzero(disp.overlay_line_2, BUFFER_LINE_SIZE);
			bzero(disp.menu_line_1, BUFFER_LINE_SIZE);
			bzero(disp.menu_line_2, BUFFER_LINE_SIZE);
			disp.power_led = LED_OFF;
			disp.alarm_led = LED_OFF;
			
			/* set initial values */
			disp.power_led = LED_ON;
			display_set_leds();

			/* set initial buffers */
			snprintf(disp.menu_line_1,(size_t)BUFFER_LINE_SIZE+1,
					"NO MENU\n");
			snprintf(disp.menu_line_2,(size_t)BUFFER_LINE_SIZE+1,
					"START MENU SERVER\n");
			snprintf(disp.overlay_line_1,(size_t)BUFFER_LINE_SIZE+1,
					"IPsecureIT\n");
			snprintf(disp.overlay_line_2,(size_t)BUFFER_LINE_SIZE+1,
					"Booting...\n");
			snprintf(disp.info_line_1,(size_t)BUFFER_LINE_SIZE+1,
					"IPsecureIT\n");
			snprintf(disp.info_line_2,(size_t)BUFFER_LINE_SIZE+1,
					"Infoscreen\n");
			display_write_lines();
			
			next_state = STATE_BOOT;
			break;
		}
		case STATE_BOOT:
		{
			display_write_lines();

			/* blinkenlights */
			signal(SIGALRM, signal_handler);
			alarm(1);
			
			/* wait for go_idle() */
			next_state = STATE_BOOT;
			wait_for_input();
			break;
		}
		case STATE_INFO:
		{
			display_write_lines();
			next_state = STATE_INFO;
			wait_for_input();
			break;
		}
		case STATE_MENU:
		{
			display_write_lines();
			next_state = STATE_MENU;
			wait_for_input();
			break;
		}
		case STATE_OVERLAY:
		{
			display_write_lines();
			next_state = STATE_OVERLAY;
			wait_for_input();
			break;
		}
		case STATE_IDLE:
		{
			/* nothing to do, wait for external trigger */
			while(1);
		}
		} /* switch */
	} /* while */
}

int main(int argc, char **argv) {
	int i;
	struct stat tmp;

	printf("IPsecureIT display driver v1.3\n");
	printf("(c) 2007 by GreenMont Systems Nederland\n");
	
	/* TBD: check for hardware revision */
	printf("IPsecureIT hardware version 1.0 found\n");

	/* check if display can be opened */
	fd_display = open(TTYDISPLAY, O_RDWR);
	if (fd_display == -1) {
		printf("%s", strerror(errno));
		exit(errno);
	}
	close(fd_display);

	/* check if fifo exists */
	if (stat(FIFO, &tmp) != -1) {
		/* unlink this one, it's kinda old */
		if ((i = unlink(FIFO)) != 0) {
			printf("Cannot unlink old fifo: %s\n",
					strerror(errno));
			exit(errno);
		}
	}
	/* make new fifo */
	if ((i = mkfifo(FIFO,
			S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IWOTH)) == -1) {
		printf("Cannot create fifo: %s\n", strerror(errno));
		exit(errno);
	}
	
		
	/* do the Real Hard Work (tm) */
	state_machine();

	fclose(disp_fifo);
	return 0;
}
