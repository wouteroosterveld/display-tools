#ifndef MENU_D_H
#define MENU_D_H

#define MENU_STATE_NOMENU	0x0
#define MENU_STATE_MAINMENU	0x1
#define MENU_STATE_SUBMENU	0x2
#define MENU_STATE_ERROR	0xf

#define BUTTON_MENU	0x1
#define BUTTON_INPUT	0x2

#define MENU_ITEM_TYPE_NULL	0x0
#define MENU_ITEM_TYPE_SUBMENU	0x1
#define MENU_ITEM_TYPE_COMMAND	0x2
#define MENU_ITEM_TYPE_DISPLAY	0x3

#define WAITTIME		100000
#define	MENU			"/etc/menu.conf"

typedef struct menu_item {
	int item_type;
	char line1[20];
	char line2[20];
	char *command;
	int parent;
	int child;
	int prev;
	int next;
} menu_item;


#endif /* MENU_D_H */
