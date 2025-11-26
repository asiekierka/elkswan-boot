#include <stdio.h>
#include <wonderful.h>
#include <ws.h>
#include "../assets/wsx_console_font_default.h"

#ifdef __WONDERFUL_WWITCH__
#include <sys/bios.h>

void console_init(void) {
    text_screen_init();
}
#else
#include <wsx/zx0.h>

#define screen1 (*((ws_screen_t*) 0x3000))

void console_init(void) {
	ws_display_set_control(0);
	ws_display_set_shade_lut_default();
	memset(&screen1, 0, sizeof(screen1));
	memset(WS_TILE_MEM(384), 0, 16);
	wsx_zx0_decompress(WS_TILE_MEM(384 + 32), wsx_console_font_default);
	outportw(WS_SCR_PAL_0_PORT, 0x2570);
	ws_display_scroll_screen1_to(0, 0);
	ws_display_set_screen1_address(&screen1);
	ws_display_set_control(WS_DISPLAY_CTRL_SCR1_ENABLE);
}

void text_put_char(uint8_t x, uint8_t y, uint16_t ch) {
	screen1.row[y].cell[x] = ch + 384;
}
#endif

int console_x = 0;
int console_y = 0;

static void console_advance_line(void) {
	console_y++;
	if (console_y >= 18)
		ws_display_scroll_screen1_by(0, 8);
}

void cputs(char __far* buf) {
	for (char __far* s = buf; *s; s++) {
		if (*s == '\n') {
			console_x = 0;
			console_advance_line();
		} else {
			text_put_char(console_x++, console_y, *s);
			if (console_x >= WS_DISPLAY_WIDTH_TILES) {
				console_x = 0;
				console_advance_line();
			}
		}
	}
}

void cprintf(const char __far* format, ...) {
    char buf[128+1];
    va_list val;
    va_start(val, format);
	buf[sizeof(buf) - 1] = 0;
    vsnprintf(buf, sizeof(buf) - 1, format, val);
    va_end(val);
	cputs(buf);
}

__attribute__((noreturn))
void panic(const char __far* format, ...) {
    char buf[128+1];
    va_list val;
    va_start(val, format);
	buf[sizeof(buf) - 1] = 0;
    vsnprintf(buf, sizeof(buf) - 1, format, val);
    va_end(val);
	cputs("\nPANIC: ");
	cputs(buf);
	while(1);
}
