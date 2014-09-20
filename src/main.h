// HMAC_SHA1 implementation
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _MAIN_H_
#define _MAIN_H_

typedef struct {
	GFont font;
	bool isCustom;
} AppFont;


// Persistant Storage Keys
enum {
	PS_TIMEZONE_KEY,
	PS_THEME,
	PS_DEFAULT_KEY,
	PS_FONT_STYLE,
	PS_IDLE_TIMEOUT,
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
	JS_IDLE_TIMEOUT
};

enum { 
	UP,
	DOWN,
	LEFT,
	RIGHT
};

// define stubs
void window_config_provider(Window *window);
void request_key(int code_id);
void set_fonts();
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed);
void start_refreshing();
void finish_refreshing();

#endif /* _MAIN_H_ */