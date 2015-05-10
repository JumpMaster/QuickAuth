//
// Copyright 2015
// PebbleAuth for the Pebble Smartwatch
// Author: Kevin Cooper
// https://github.com/JumpMaster/PebbleAuth
//

#ifndef _MAIN_H_
#define _MAIN_H_

#include "pebble.h"
	
typedef struct {
	GFont font;
	bool isCustom;
} AppFont;

#define ASCII_0_VALU 48
#define ASCII_9_VALU 57
#define ASCII_A_VALU 65
#define ASCII_F_VALU 70

#define MAX_OTP 16
#define MAX_LABEL_LENGTH 21 // 20 + termination
#define MAX_KEY_LENGTH 65 // 64 + termination
#define MAX_COMBINED_LENGTH MAX_LABEL_LENGTH+MAX_KEY_LENGTH
#define APP_VERSION 20
#define DEBUG false

#define MyTupletCString(_key, _cstring) \
((const Tuplet) { .type = TUPLE_CSTRING, .key = _key, .cstring = { .data = _cstring, .length = strlen(_cstring) + 1 }})

// Persistant Storage Keys
enum {
	PS_TIMEZONE_KEY,
	PS_APLITE_THEME,
	PS_DEFAULT_KEY,
	PS_FONT_STYLE,
	PS_IDLE_TIMEOUT,
	PS_BASALT_COLORS,
	PS_SECRET = 0x40 // Needs 16 spaces, should always be last
};

// JScript Keys
enum {
	JS_KEY_COUNT,
	JS_REQUEST_KEY,
	JS_TRANSMIT_KEY,
	JS_TIMEZONE,
	JS_DISPLAY_MESSAGE,
	JS_THEME,
	JS_DELETE_KEY,
	JS_FONT_STYLE,
	JS_DELETE_ALL,
	JS_IDLE_TIMEOUT,
	JS_BASALT_COLORS
};

enum { 
	UP,
	DOWN,
	LEFT,
	RIGHT
};


//extern bool loading_complete;

// define stubs
void window_config_provider(Window *window);
void request_key(int code_id);
void set_fonts();
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed);
void start_refreshing();
void finish_refreshing();
void animation_control();
void set_display_colors();
void apply_display_colors();

#endif /* _MAIN_H_ */