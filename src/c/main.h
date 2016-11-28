//
// Copyright 2015
// PebbAuth for the Pebble Smartwatch
// Author: Kevin Cooper
// https://github.com/JumpMaster/PebbleAuth
//

#pragma once
#include "pebble.h"
	
typedef struct {
	GFont font;
	bool isCustom;
} AppFont;

#define MAX_OTP 30
#define MAX_LABEL_LENGTH 21 // 20 + termination
#define MAX_KEY_LENGTH 129 // 128 + termination
#define MAX_COMBINED_LENGTH MAX_LABEL_LENGTH+MAX_KEY_LENGTH
#define APP_VERSION 31
#define DEBUG true

#define MyTupletCString(_key, _cstring) \
((const Tuplet) { .type = TUPLE_CSTRING, .key = _key, .cstring = { .data = _cstring, .length = strlen(_cstring) + 1 }})

// Persistant Storage Keys
enum {
	PS_TIMEZONE_KEY,
	PS_DEFAULT_KEY,
	PS_FONT,
	PS_IDLE_TIMEOUT,
	PS_FOREGROUND_COLOR,
	PS_BACKGROUND_COLOR,
	PS_WINDOW_LAYOUT,
	PS_SECRET = 0x40 // Needs 30 spaces, should always be last
};


// Animation Directions
enum { 
	UP,
	DOWN,
	LEFT,
	RIGHT
};

extern GColor bg_color;
extern GColor fg_color;
extern AppFont font_pin;
extern AppFont font_label;

extern char otp_labels[MAX_OTP][MAX_LABEL_LENGTH];
extern char otp_keys[MAX_OTP][MAX_KEY_LENGTH];

extern unsigned int font;
extern unsigned int watch_otp_count;
extern unsigned int otp_selected;
extern unsigned int otp_update_tick;
extern unsigned int otp_updated_at_tick;
extern int timezone_offset;

extern bool loading_complete;
extern bool refresh_required;
extern bool fonts_changed;
extern bool colors_changed;

void set_default_key(int key_id, bool force_refresh);
void request_delete(int key_id);
void resetIdleTime();
void switch_window_layout();
void animate_layer(Layer *layer, AnimationCurve curve, GRect *start, GRect *finish, int duration, AnimationStoppedHandler callback);
void add_countdown_layer(struct Layer *window_layer);
void set_countdown_layer_color(GColor color);
void show_countdown_layer();
void hide_countdown_layer();
void move_key_position(unsigned int key_position, unsigned int new_position);
void apply_new_colors();