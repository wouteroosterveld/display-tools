/*	filename:	menud.c
 *	date:		14 february 2006
 *	version:	1.3
 *	author:		w.g.oosterveld
 *	e-mail:		w.g.oosterveld@greenmont.nl
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include "dispd.h"
#include "menud.h"

FILE *display;
FILE *disp_fifo;

struct menu_item *menu_items[128];
struct menu_item *current_menu_item;
int current_parent_menu;

struct itimerval itimer;
struct sigaction sact;

int nr_menu_items = 0;

int state = MENU_STATE_NOMENU, next_state = MENU_STATE_NOMENU;

void alarm_handler() {
	/* do nothing, just exit */
}

int get_input()
{
	char buffer[10];
	
	/* loop, wait for input and process button press */
	while(1) {
		fgets(buffer, 10, display);
		if ((strncmp(&buffer[0], "#0", 2) == 0)) {
			return BUTTON_INPUT;
		} else
		if ((strncmp(&buffer[0], "#1", 2) == 0)) {
			return BUTTON_MENU;
		}
	}
}

void display_current_menu_item() {
	char send[BUFFER_LINE_SIZE +3];
	
	/* line 1 */
	bzero(send, BUFFER_LINE_SIZE +3);
	send[0] = IPC_SETMENULINE1;
	strncpy(&(send[1]), current_menu_item->line1,
			strlen(current_menu_item->line1));
	send[BUFFER_LINE_SIZE +2] = '\n';
	fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	
	/* wait 100 ms before continue */
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = alarm_handler;
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = WAITTIME;
	itimer.it_value.tv_sec = 0;
	itimer.it_value.tv_usec = WAITTIME;
	setitimer(ITIMER_REAL, &itimer, NULL);
	signal(SIGALRM, alarm_handler);
	pause();
	fflush(disp_fifo);

	/* line 2 */
	if (current_menu_item->item_type != MENU_ITEM_TYPE_DISPLAY) {
		bzero(send, BUFFER_LINE_SIZE +3);
		send[0] = IPC_SETMENULINE2;
		strncpy(&(send[1]), current_menu_item->line2,
				strlen(current_menu_item->line2));
		send[BUFFER_LINE_SIZE +2] = '\n';
		fwrite((void *)send, 1, BUFFER_LINE_SIZE +3, disp_fifo);
	
		/* wait 100 ms before continue */
		itimer.it_interval.tv_usec = WAITTIME;
		setitimer(ITIMER_REAL, &itimer, NULL);
		signal(SIGALRM, alarm_handler);
		pause();
		fflush(disp_fifo);
	}
}

void add_item_to_menu(int type, char *name, char *rhs)
{
	int i, end_of_chain = -1;
	int parent_item;
	int check_this_item = 0;
	int head, tail;
	struct menu_item *current_menu_head_node = NULL;
	struct menu_item *current_menu_tail_node = NULL;
	
	struct menu_item *tmp, *exit_node;

	/* initialize this menu item chain */
	/* if we have to add a menu item to an existing menu,
	 * int current_parent_menu points to the parent menu node.
	 */
	current_menu_head_node = menu_items[menu_items[current_parent_menu]->child];
	/* find the end of this chain */
	current_menu_tail_node = current_menu_head_node;
	tail = menu_items[current_parent_menu]->child;
	for(i = 0; i < nr_menu_items; i++) {
		/* the last item is called 'Exit' */
		if (strncmp(current_menu_tail_node->line1,"Exit",4)==0) {
			break;
		}
		tail = current_menu_tail_node->next;
		current_menu_tail_node = 
			menu_items[current_menu_tail_node->next];
	} /* tail is the index of current_menu_tail_node */

	/* fill the items' parameters */
	tmp = (struct menu_item *)malloc(sizeof(struct menu_item));
	tmp->item_type = type;
	strncpy(tmp->line1, name, strlen(name));
	strncpy(tmp->line2, "press INPUT to sel.", 19);
	if (type == MENU_ITEM_TYPE_COMMAND) {
		tmp->command = (char *)malloc(strlen(rhs) - 8);
		strncpy(tmp->command, &(rhs[8]), strlen(rhs));
	} if (type == MENU_ITEM_TYPE_DISPLAY) {
		tmp->command = (char *)malloc(strlen(rhs) - 5);
		strncpy(tmp->line2, &(rhs[5]), strlen(rhs));
	}
	tmp->parent = current_parent_menu;
	
	/* if this is a new menu item, create an exit node as child */
	if (type == MENU_ITEM_TYPE_SUBMENU) {
		exit_node = (struct menu_item *)
			malloc(sizeof(struct menu_item));
		exit_node->item_type = MENU_ITEM_TYPE_NULL;
		strncpy(exit_node->line1, "Exit", 4);
		strncpy(exit_node->line2, "press INPUT to exit\n", 20);
		exit_node->command = NULL;
		exit_node->parent = nr_menu_items+1;
		exit_node->child = 0;
		exit_node->prev = nr_menu_items;
		exit_node->next = nr_menu_items;
		menu_items[nr_menu_items] = (struct menu_item *)
			malloc(sizeof(struct menu_item));
		memcpy(menu_items[nr_menu_items++], exit_node, sizeof(struct menu_item));

	} /* exit node */
	
	if (type == MENU_ITEM_TYPE_SUBMENU) tmp->child = nr_menu_items-1;
	tmp->prev = menu_items[tail]->prev; /* add at end of chain, but before exit node */
	tmp->next = tail; /* next one is exit node */
	
	/* update previous item in chain */
	menu_items[current_menu_tail_node->prev]->next = nr_menu_items;

	/* update exit node to point back to this new one */
	current_menu_tail_node->prev = nr_menu_items;
	
	/* add this item to menu_items */
	menu_items[nr_menu_items] = (struct menu_item *)
		malloc(sizeof(struct menu_item));
	memcpy(menu_items[nr_menu_items], tmp, sizeof(struct menu_item));
	
	/* update parent if this is the first child node */
	if (menu_items[menu_items[current_parent_menu]->child]->next ==
			menu_items[menu_items[current_parent_menu]->child]->prev) {
		menu_items[current_parent_menu]->child = nr_menu_items;
	}

	
	nr_menu_items++;
}

void build_menu() {
	int fd_menu, i;
	FILE *menu_conf;
	int current_index = 0;
	char buf[255], item_name[255], rhs[255];
	struct menu_item *tmp;

	int previous_menu_item = 0;
	int building_a_menu = 0;
	int building_a_submenu = 0;
	int parent_for_this_submenu = 0;
	
	/* build root menu */
	/* node 0 (NULL) is the root */
	tmp = (struct menu_item *)malloc(sizeof(struct menu_item));

	/* fill root node */
	tmp->item_type = MENU_ITEM_TYPE_SUBMENU;
	strncpy(tmp->line1, "Main\n", 10);
	strncpy(tmp->line2, "press MENU to sel.\n", 17);
	tmp->command = NULL;
	tmp->parent = 0;
	tmp->child = 0;
	tmp->prev = 1;
	tmp->next = 1;

	/* allocate array item */
	menu_items[nr_menu_items] =
		(struct menu_item *)malloc(sizeof(struct menu_item));
	memcpy(menu_items[nr_menu_items++], tmp, sizeof(struct menu_item));

	/* fill exit_menu node */
	strncpy(tmp->line1, "Exit\n", 10);
	strncpy(tmp->line2, "press INPUT to exit\n", 20);
	tmp->prev = 0;
	tmp->next = 0;
	menu_items[nr_menu_items] =
		(struct menu_item *)malloc(sizeof(struct menu_item));
	memcpy(menu_items[nr_menu_items++], tmp, sizeof(struct menu_item));

	/* open menu configuration file */
	menu_conf = fopen(MENU, "r");
	
	/* parse file */
	while(fgets(buf, 255, menu_conf) != NULL) {
		int parent, child, next, prev;
		if (buf[0] == '[') {
			for (i = 0; i < strlen(buf); i++) {
				if (buf[i] == ']') {
					break;
				}
			}
			strncpy(item_name, &(buf[1]), i-1);
		//}
		//if (sscanf(buf, "[%s]", item_name) == 1) {
			item_name[strlen(item_name)-1] = '\0';
			/* find menu and set current_parent_menu */
			for(i = 0; i < nr_menu_items; i++) {
				if(strncmp(menu_items[i]->line1, item_name,
							strlen(item_name)) == 0) {
					current_parent_menu = i;
					break;
				}
			} /* for */
			memset((void *)item_name, '\0', 255);
		} else if (strlen(buf) > 1) {
			/* not a menu identifier, scan the equals sign */
			for (i = 0; i < strlen(buf); i++) {
				if (buf[i] == '=') {
					/* bingo */
					break;
				}
			}
			/* copy lhs and rhs */
			strncpy(item_name, buf, i-1);
			strncpy(rhs, &(buf[i+2]), 255);
			
		//if (sscanf(buf, "%s = %s", item_name, rhs) == 2) {
			if (strncmp(rhs, "menu", 4) == 0) {
				/* add item to menu, fill in later */
				parent = child = next = prev = 0;
				add_item_to_menu(MENU_ITEM_TYPE_SUBMENU,
						item_name, NULL);
			} else {
				/* this is a command of a show */
				int index = 0, type = 0;
				index += strlen(item_name);
				index += 3;
				strcpy(rhs, &(buf[index]));
				if (strncmp(rhs, "show", 4) == 0) {
					type = MENU_ITEM_TYPE_DISPLAY;
				} else if (strncmp(rhs, "command", 7) == 0) {
					type = MENU_ITEM_TYPE_COMMAND;
				} else {
					exit(0);
				}
				/* buf contains the complete command */
				add_item_to_menu(type, item_name, rhs);
			} /* if-menu-declatation-else-not */
		} /* if-menu-item-else-not-menu-item */
		memset((void *)item_name, '\0', 255);

	} /* while */
	fclose(menu_conf);
	current_menu_item = menu_items[0];
}

	
void enter_menu() {
	char send[2];
	bzero(send, 2);
	send[0] = IPC_ENTERMENU;
	send[1] = '\n';
	fwrite((void *)send, 1, 2, disp_fifo);
	fflush(disp_fifo);
}

void exit_menu() {
	char send[2];
	bzero(send, 2);
	send[0] = IPC_EXITMENU;
	send[1] = '\n';
	fwrite((void *)send, 1, 2, disp_fifo);
	fflush(disp_fifo);
}

void state_machine() {
	while((state = next_state) != MENU_STATE_ERROR) {
		switch(state) {
		case MENU_STATE_NOMENU:
		{
			int i, button = 0;
			button = get_input();
			if (button == BUTTON_MENU) {
				enter_menu();
				current_menu_item = menu_items[0];
				/*display_current_menu_item();*/
				next_state = MENU_STATE_MAINMENU;
			} else {
				next_state = MENU_STATE_NOMENU;
			}
			break;
		}
		case MENU_STATE_MAINMENU:
		{
			int button = 0;
			display_current_menu_item();
			button = get_input();
			if (button == BUTTON_MENU) {
				/* go to next item */
				current_menu_item = menu_items[current_menu_item->next];
			} else
			if (button == BUTTON_INPUT) {
				/* if exit, do exit now */
				if (strncmp(current_menu_item->line1, "Exit", 4) == 0) {
					if (current_menu_item->parent == 0) {
						/* exit node */
						exit_menu();
						current_menu_item = menu_items[0];
						next_state = MENU_STATE_NOMENU;
						break;
					} else {
						current_menu_item = menu_items[current_menu_item->parent];
						break;
					}
				} else if (current_menu_item->item_type ==
						MENU_ITEM_TYPE_SUBMENU) {
					/* dive into submenu */
					current_menu_item =
						menu_items[current_menu_item->child];
				} else if (current_menu_item->item_type ==
						MENU_ITEM_TYPE_COMMAND) {
						system(current_menu_item->command);
				} else if (current_menu_item->item_type ==
						MENU_ITEM_TYPE_DISPLAY	) {
				}
			}
			next_state = MENU_STATE_MAINMENU;
			break;
		}
		case MENU_STATE_SUBMENU:
		{
			/* code */
		}
		} /* switch */
	} /* while */
}

int main(int argc, char **argv)
{
	struct stat tmp;

	printf("IPsecureIT menu driver v1.3\n");
	printf("(c) 2007 by GreenMont Systems Nederland\n\n");

	/* TBD: check for hardware revision */
	printf("IPsecureIT hardware version 1.0 found\n");

	/* open display for reading */
	display = fopen(TTYDISPLAY, "r");
	if (display == NULL) {
		printf("Cannot open display\n");
		printf("%s", strerror(errno));
		exit(errno);
	}
	
	/* check if fifo exists */
	if (stat(FIFO, &tmp) == -1) {
		/* no fifo, exit */
		printf("No display driver found. Check if dispd is running.\n");
		printf("%s\n", strerror(errno));
		exit(errno);
	}
	
	/* check if menu configuration file exists */
	if (stat(MENU, &tmp) == -1) {
		/* no menu conf, exit */
		printf("No menu configuration file /etc/menu.conf found.\n");
		printf("%s\n", strerror(errno));
		exit(errno);
	}
	
	/* open fifo for writing */
	if ((disp_fifo = fopen(FIFO, "w")) == NULL) {
		printf("Cannot open fifo for writing: %s\n",
				strerror(errno));
		exit(errno);
	}
	
	/* parse_menu() */
	/* xml parser not implemented */
	
	build_menu();
	exit_menu();
	state_machine();
	
	fclose(display);
	fclose(disp_fifo);
	
	return 0;
}
