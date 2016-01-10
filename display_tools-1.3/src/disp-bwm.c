/*	filename:	disp-bwm.c
 *	date:		20 february 2007
 *	version:	1.3
 *	author:		w.g.oosterveld
 *	e-mail:		w.g.oosterveld@greenmont.nl
 */

/* based on good-old bwm */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "dispd.h"

#define DEVFILE "/proc/net/dev"
#define DISPIFACE "/etc/iface"
#define MAX_INTERFACES 16

FILE *disp_fifo;
FILE *devfile;
FILE *dispiface;

struct interface_info {
	char name[12];
	
	unsigned long tx_bytes_old;
	unsigned long rx_bytes_old;
	unsigned long tx_bytes_new;
	unsigned long rx_bytes_new;
	unsigned long tx_kbytes_dif;
	unsigned long rx_kbytes_dif;

	struct timeval time_old;
	struct timeval time_new;

	unsigned long time_diff_ms;
	
	unsigned long tx_rate_whole;
	unsigned long rx_rate_whole;
	unsigned long tot_rate_whole;
	unsigned tx_rate_part;
	unsigned rx_rate_part;
	unsigned tot_rate_part;
};

unsigned long bwm_calc_remainder(unsigned long num, unsigned long den);

struct options {
	int ifaceset;
};

/* some globals */
struct options op;
struct ipsecureit_display disp;
char show_iface[20];
char buf[20];

void display_help() {
	printf("IPsecureIT display bandwidth monitor\n");
	printf("(c) 2007 by GreenMont Systems Nederland\n\n");
	printf("usage: disp-bwm <option>\n\n");
	printf("  options are:\n\n");
	printf("  -i <iface>            show interface <iface>\n\n");
	printf("This version only supports showing the interface in /etc/iface\n");
}	


void do_work() {
	struct interface_info iface[MAX_INTERFACES];
	struct timezone tz;
	int field_number, total_counter, first_pass = 1;
	int inum = -1;
	
	char buffer[256];
	char *buffer_pointer;
	char send[BUFFER_LINE_SIZE +3];

	unsigned long rx_bw_total_whole = 0;
	unsigned long tx_bw_total_whole = 0;
	unsigned long tot_bw_total_whole = 0;
	unsigned rx_bw_total_part = 0;
	unsigned tx_bw_total_part = 0;
	unsigned tot_bw_total_part = 0;
	
	unsigned long int conv_field;
	

	while(1) {
		inum = -1;
		/* open devfile */
		if ((devfile = fopen(DEVFILE, "r")) == NULL) {
			printf("Cannot open /proc/net/dev. %s \n", strerror(errno));
			exit(errno);
		}

		/* check interface */
		if ((dispiface = fopen(DISPIFACE, "r")) == NULL) {
			printf("Cannot open %s. %s\n", DISPIFACE, strerror(errno));
			exit(errno);
		}
		fgets(show_iface, 5, dispiface);
		fclose(dispiface);
		
		/* read statistics, hail to bwm! */
		fgets(buffer, 255, devfile); /* skip first line */

		while(fgets(buffer, 255, devfile) != NULL &&
				inum++ < MAX_INTERFACES -1) {
			iface[inum].time_old = iface[inum].time_new;
			gettimeofday(&iface[inum].time_new, &tz);

			/* compute time delta */
			iface[inum].time_diff_ms = 
				(unsigned long)(iface[inum].time_new.tv_sec * 1000 +
				iface[inum].time_new.tv_usec / 1000) -
				(iface[inum].time_old.tv_sec * 1000 +
				iface[inum].time_old.tv_usec / 1000);

			/* fill data structures */
			if (inum > 0) {
				buffer_pointer = buffer;
				buffer_pointer = strtok(buffer_pointer, " :");
				strncpy(iface[inum].name,
						buffer_pointer, 11);
				field_number = 0;
				while((buffer_pointer = strtok(NULL, " :")) != 
					NULL) {
					conv_field = strtoul(buffer_pointer,
							NULL, 10);
					field_number++;

					switch (field_number) {
						case 1: {
							iface[inum].rx_bytes_old = iface[inum].rx_bytes_new;
							iface[inum].rx_bytes_new = conv_field;
							iface[inum].rx_kbytes_dif = (iface[inum].rx_bytes_new - iface[inum].rx_bytes_old) / 1024 * 1000;
							iface[inum].rx_rate_whole = iface[inum].rx_kbytes_dif / iface[inum].time_diff_ms;
							iface[inum].rx_rate_part = bwm_calc_remainder(iface[inum].rx_kbytes_dif, iface[inum].time_diff_ms);
						}
						break;

						case 9: {
							iface[inum].tx_bytes_old = iface[inum].tx_bytes_new;
							iface[inum].tx_bytes_new = conv_field;
							iface[inum].tx_kbytes_dif = (iface[inum].tx_bytes_new - iface[inum].tx_bytes_old) / 1024 * 1000;
							iface[inum].tx_rate_whole = iface[inum].tx_kbytes_dif / iface[inum].time_diff_ms;
							iface[inum].tx_rate_part = bwm_calc_remainder(iface[inum].tx_kbytes_dif, iface[inum].time_diff_ms);
							iface[inum].tot_rate_whole = iface[inum].rx_rate_whole + iface[inum].tx_rate_whole;
							iface[inum].tot_rate_part = iface[inum].rx_rate_part + iface[inum].tx_rate_part;
							if (iface[inum].tot_rate_part > 1000) {
								iface[inum].tot_rate_whole++;
								iface[inum].tot_rate_part -=1000;
							} /* case */
						} /* switch */
						break;
					} /* while tokens */
				} /* if inum > 0 */
				if (!first_pass) {
					/* print results for this interface */
					/* printf("%s RX:%3lu.%1uK/s TX:%3lu.%1uK/s\n",
						iface[inum].name,
						iface[inum].rx_rate_whole,
						iface[inum].rx_rate_part/100,
						iface[inum].tx_rate_whole,
						iface[inum].tx_rate_part/100);*/
							
				}
				if (!first_pass && op.ifaceset ==1 &&
						strcmp(show_iface, iface[inum].name) ==0) {
					/* print to display */
					disp_fifo = fopen(FIFO, "w+");
					if(disp_fifo == NULL) {
						printf(" can't open fifo\n");
					}

					/* build the display string */
					sprintf(buf, "RX:");
					if (iface[inum].rx_rate_whole >= 1000){
						sprintf(&(buf[3]), "%3lu.%1uM ",
							iface[inum].rx_rate_whole / 1000,
							(iface[inum].rx_rate_whole % 1000) / 100);
					} else {
						sprintf(&(buf[3]), "%3lu.%1uK ",
							iface[inum].rx_rate_whole,
							iface[inum].rx_rate_part/100);
					}
					sprintf(&(buf[10]), "TX:");
					if (iface[inum].tx_rate_whole >= 1000){
						sprintf(&(buf[13]), "%3lu.%1uM",
							iface[inum].tx_rate_whole / 1000,
							(iface[inum].tx_rate_whole % 1000) / 100);
					} else {
						sprintf(&(buf[13]), "%3lu.%1uK",
							iface[inum].tx_rate_whole,
							iface[inum].tx_rate_part/100);
					}


					bzero(send, BUFFER_LINE_SIZE +3);
					send[0] = IPC_SETINFOLINE2;
					strncpy(&(send[1]), buf, strlen(buf));
					send[BUFFER_LINE_SIZE +2] = '\n';
					fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
					fclose(disp_fifo);
				} /* if print to display */
			} /* if num > 0 */
		} /* while read a line from /proc/net/dev */

		fclose(devfile);

		/* compute global totals */
		rx_bw_total_whole = 0;
		tx_bw_total_whole = 0;
		rx_bw_total_part = 0;
		tx_bw_total_part = 0;
		for (total_counter = 1; total_counter < inum; total_counter++) {
			rx_bw_total_whole += iface[total_counter].rx_rate_whole;
			rx_bw_total_part += iface[total_counter].rx_rate_part;
			if (rx_bw_total_part > 1000) {
				rx_bw_total_whole++;
				rx_bw_total_part -= 1000;
			}
			tx_bw_total_whole += iface[total_counter].tx_rate_whole;
			tx_bw_total_part += iface[total_counter].tx_rate_part;
			if (tx_bw_total_part > 1000) {
				tx_bw_total_whole++;
				tx_bw_total_part -= 1000;
			}
			tot_bw_total_whole+=rx_bw_total_whole+tx_bw_total_whole;
			tot_bw_total_part+=rx_bw_total_whole+tx_bw_total_part;
			if (tot_bw_total_part > 1000) {
				tot_bw_total_whole++;
				tot_bw_total_part -= 1000;
			}
		} /* for all total counters */

		if (inum < 1) {
			printf("No interfaces up\n");
		}

		if (!first_pass) {
			/* print totals */
		}
		sleep(2);
		first_pass = 0;
	} /* while true and sleep */
}

unsigned long bwm_calc_remainder(unsigned long num, unsigned long den) {
	unsigned long long n = num;
	unsigned long long d = den;

	return ((( n - (n/d) *d) *1000) /d );
}

int main(int argc, char **argv) {
	int i, len;
	struct stat tmp;

	/* init to zero */
	op.ifaceset = 0;

	if (argc != 3) {
		display_help();
		exit(0);
	}

	/* parse command line */
#if 0
	for (i = 1; i < argc; i++) {
		/* parse */
		if (strcmp(argv[i], "-i") == 0) {
			op.ifaceset = 1;
			if ((len = strlen(argv[i+1])) > 20) len = 20;
			strncpy(show_iface, argv[i+1], len);
			break;
		} else {
			display_help();
			exit(0);
		} /* big if-else */
	} /* for */
#endif

	if (stat(DISPIFACE, &tmp) == -1) {
		printf("No interface defined. Check /etc/iface or set in menu.\nUsing eth0 as default.\n");
		strcpy(show_iface, "eth0");
	} else {
		if ((dispiface = fopen(DISPIFACE, "r")) == NULL) {
			printf("Cannot open %s. %s\n", DISPIFACE, strerror(errno));
			exit(errno);
		}
		fgets(show_iface, 5, dispiface);
		fclose(dispiface);
	}

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

	printf("using interface %s\n", show_iface);
	op.ifaceset = 1;
	do_work();
	fclose(disp_fifo);
	
	return 0;
}
