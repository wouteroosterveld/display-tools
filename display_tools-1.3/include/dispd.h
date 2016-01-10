#ifndef DISPD_H
#define DISPD_H
#define TTYDISPLAY	"/dev/ttyS1"
#define FIFO		"/tmp/disp_fifo"

#define WAITTIME	100000

#define STATE_INIT	0
#define STATE_BOOT	1
#define STATE_INFO	2
#define STATE_MENU	3
#define STATE_OVERLAY	4
#define STATE_IDLE	5

#define STATE_ERROR	99

#define LED_POWER_ON	"l=01\n"
#define LED_POWER_OFF	"l=00\n"
#define LED_ALARM_ON	"l=11\n"
#define LED_ALARM_OFF	"l=10\n"

#define BUTTON_READ_ON	"b=1\n"
#define BUTTON_READ_OFF	"b=0\n"

#define DISPLAY_PREFIX	"d="
#define DISPLAY_PREFIX_CLEAR	"D="

#define CONTROL_CLEAR	"c=c\n"
#define CONTROL_HOME	"c=h\n"
#define CONTROL_MOVERIGHT	"c=r\n"
#define CONTROL_MOVELEFT	"c=l\n"
#define CONTROL_BLINK	"c=b\n"
#define CONTROL_NORMAL	"c=n\n"

#define LIGHT_ON	"z=1\n"
#define LIGHT_OFF	"z=0\n"
#define BUFFER_LINE_SIZE	20

#define CURSOR_FIRSTLINE	"g=00\n"
#define CURSOR_SECONDLINE	"g=10\n"

#define LED_ON		1
#define LED_OFF		0

struct ipsecureit_display {
	char info_line_1[BUFFER_LINE_SIZE];
	char info_line_2[BUFFER_LINE_SIZE];
	char overlay_line_1[BUFFER_LINE_SIZE];
	char overlay_line_2[BUFFER_LINE_SIZE];
	char menu_line_1[BUFFER_LINE_SIZE];
	char menu_line_2[BUFFER_LINE_SIZE];
	int power_led;
	int alarm_led;
};

#define IPC_BOOTFINISHED	0x41
#define IPC_ENTEROVERLAY	0x42
#define IPC_EXITOVERLAY		0x43
#define IPC_ENTERMENU		0x44
#define IPC_EXITMENU		0x45
#define IPC_LEDPOWERON		0x46
#define IPC_LEDPOWEROFF		0x47
#define IPC_LEDALARMON		0x48
#define IPC_LEDALARMOFF		0x49
#define IPC_SETOVERLAYLINE1	0x4a
#define IPC_SETOVERLAYLINE2	0x4b
#define IPC_SETMENULINE1	0x4c
#define IPC_SETMENULINE2	0x4d
#define IPC_SETINFOLINE1	0x4e
#define IPC_SETINFOLINE2	0x4f
#define IPC_BUTTONREADON	0x50
#define IPC_BUTTONREADOFF	0x51


#endif /* DISPD_H */
