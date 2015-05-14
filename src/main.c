//
// Copyright 2015
// PebbleAuth for the Pebble Smartwatch
// Author: Kevin Cooper
// https://github.com/JumpMaster/PebbleAuth
//

#include "pebble.h"
#include "main.h"
#include "google-authenticator.h"
#include "select_window.h"
#include <ctype.h>

// Main Window
Window *main_window;
static TextLayer *countdown_layer;
static TextLayer *text_pin_layer;
static TextLayer *text_label_layer;
static TextLayer *swipe_layer;

static GRect text_pin_rect;
static GRect text_label_rect;
static GRect countdown_rect;
static GRect display_bounds;

// Colors
GColor bg_color;
GColor fg_color;

// Fonts
static unsigned int font_style;
static AppFont font_pin;
static AppFont font_label;

bool fonts_changed;
bool loading_complete = false;

unsigned int js_message_retry_count = 0;
unsigned int js_message_max_retry_count = 5;

int timezone_offset = 0;

unsigned int phone_otp_count = 0;
unsigned int watch_otp_count = 0;
unsigned int idle_second_count = 0;
unsigned int idle_timeout = 300;
unsigned int otp_selected = 0;
unsigned int otp_default = 0;
unsigned int otp_update_tick = 0;
unsigned int otp_updated_at_tick = 0;
unsigned int requesting_code = 0;
unsigned int animation_direction = LEFT;
unsigned int animation_state = 0;
unsigned int animation_count = 0;
bool override_second_counter = false;

char otp_labels[MAX_OTP][MAX_LABEL_LENGTH];
char otp_keys[MAX_OTP][MAX_KEY_LENGTH];
char label_text[MAX_LABEL_LENGTH];
char pin_text[MAX_KEY_LENGTH];

#ifdef PBL_COLOR
	char basalt_colors[13];
#else
	unsigned int aplite_theme = 0;  // 0 = Dark, 1 = Light
#endif

// MAIN APP

void refresh_screen_data(int direction) {
	if (!loading_complete)
		return;
	
	animation_direction = direction;
	animation_state = 40;
	animation_control();
}

void resetIdleTime() {
	idle_second_count = 0;
}

void update_screen_fonts() {
	fonts_changed = true;
	refresh_screen_data(DOWN);
}

void set_default_key(int key_id) {
	otp_default = key_id;
	persist_write_int(PS_DEFAULT_KEY, otp_default);
	
	if (otp_selected != otp_default) {
		otp_selected = otp_default;
		refresh_screen_data(DOWN);
	}
}

void expand_key(char *inputString, bool new_code) {
	if (strstr(inputString, ":") == NULL) {
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: SUPER NULL input string, ignoring");
		return;
	}
	
	bool colonFound = false;
	int outputChar = 0;
	
	char otp_key[MAX_KEY_LENGTH];
	char otp_label[MAX_LABEL_LENGTH];
	
	for(unsigned int i = 0; i < strlen(inputString); i++) {
		if (inputString[i] == ':') {
			otp_label[outputChar] = '\0';
			colonFound = true;
			outputChar = 0;
		} else {
			if (colonFound) 
				otp_key[outputChar] = inputString[i]; 
			else
				otp_label[outputChar] = inputString[i];
			
			outputChar++;
		}
	}
	otp_key[outputChar] = '\0';
	
	// If the label or key are null ignore them
	if (strlen(otp_label) <= 0 || strlen(otp_key) <= 2) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: NULL key or label, ignoring");
		return;
	}
	
	bool updating_label = false;
	if (new_code) {
		for(unsigned int i = 0; i < watch_otp_count; i++) {
			if (strcmp(otp_key, otp_keys[i]) == 0) {
				updating_label = true;
				if (DEBUG) {
					APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Code exists. Relabeling");
					APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Saving to location: %d", PS_SECRET+i);
				}

				strcpy(otp_labels[i], otp_label);
				persist_write_string(PS_SECRET+i, inputString);
				if (otp_selected != i)
					otp_selected = i;
				
				refresh_screen_data(DOWN);
			}
		}
	}
	
	if (!updating_label) {
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Adding Code");
		strcpy(otp_keys[watch_otp_count], otp_key);
		strcpy(otp_labels[watch_otp_count], otp_label);
		if (new_code) {
			if (DEBUG)
				APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Saving to location: %d", PS_SECRET+watch_otp_count);
			persist_write_string(PS_SECRET+watch_otp_count, inputString);
		}
		watch_otp_count++;
		otp_selected = watch_otp_count-1;
		refresh_screen_data(DOWN);
	}
}

void check_load_status() {
	if ((phone_otp_count > 0 && phone_otp_count < requesting_code) || requesting_code > MAX_OTP) {
		requesting_code = 0;
		loading_complete = true;
		refresh_screen_data(DOWN);
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: FINISHED REQUESTING");
	}
	else if (requesting_code > 0) {
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: REQUESTING ANOTHER!");
		request_key(requesting_code++);
	}
}

void on_animation_stopped(Animation *anim, bool finished, void *context) {
	//Free the memory used by the Animation
	property_animation_destroy((PropertyAnimation*) anim);
	anim = NULL;
}

void on_animation_stopped_callback(Animation *anim, bool finished, void *context) {
	#ifdef PBL_SDK_2
		on_animation_stopped(anim, finished, context);
	#endif
	animation_control();
}

void animate_layer(Layer *layer, AnimationCurve curve, GRect *start, GRect *finish, int duration, bool callback) {
	//Declare animation
	PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);

	//Set characteristics
	animation_set_duration((Animation*) anim, duration);
	animation_set_curve((Animation*) anim, curve);
	

	if (callback) {
		AnimationHandlers handlers = {
			.stopped = (AnimationStoppedHandler) on_animation_stopped_callback
		};
		animation_set_handlers((Animation*) anim, handlers, NULL);
	}
	
	#ifdef PBL_SDK_2
		if (!callback) {
			AnimationHandlers handlers = {
				.stopped = (AnimationStoppedHandler) on_animation_stopped
			};
			animation_set_handlers((Animation*) anim, handlers, NULL);
		}
	#endif
		
	//Start animation!	
	animation_schedule((Animation*) anim); 
}

void animate_label_on() {
	
	if (watch_otp_count)
		strcpy(label_text, otp_labels[otp_selected]);
	else
		strcpy(label_text, "EMPTY");

	GRect start = text_label_rect;
	switch(animation_direction)
	{
		case UP :
			start.origin.y += display_bounds.size.h;
			break;
		case DOWN :
			start.origin.y -= display_bounds.size.h;
			break;
		case LEFT :
			start.origin.x += display_bounds.size.w;
			break;
		case RIGHT :
			start.origin.x -= display_bounds.size.w;
			break;
	}
	animate_layer(text_layer_get_layer(text_label_layer), AnimationCurveEaseOut, &start, &text_label_rect, 300, true);
}

void animate_label_off() {
	GRect finish = text_label_rect;
		
	switch(animation_direction) {
		case UP :
			finish.origin.y -= display_bounds.size.h;
			break;
		case DOWN :
			finish.origin.y += display_bounds.size.h;
			break;
		case LEFT :
			finish.origin.x -= display_bounds.size.w;
			break;
		case RIGHT :
			finish.origin.x += display_bounds.size.w;
			break;
	}
	animate_layer(text_layer_get_layer(text_label_layer), AnimationCurveEaseIn, &text_label_rect, &finish, 300, true);
}

void animate_code_on() {
	if (watch_otp_count)
		strcpy(pin_text, generateCode(otp_keys[otp_selected], timezone_offset));
	else
		strcpy(pin_text, "123456");
	
	otp_updated_at_tick = otp_update_tick;
	
	GRect start = text_pin_rect;
	switch(animation_direction)
	{
		case UP :
			start.origin.y += display_bounds.size.h;
			break;
		case DOWN :
			start.origin.y -= display_bounds.size.h;
			break;
		case LEFT :
			start.origin.x += display_bounds.size.w;
			break;
		case RIGHT :
			start.origin.x -= display_bounds.size.w;
			break;
	}
	animate_layer(text_layer_get_layer(text_pin_layer), AnimationCurveEaseOut, &start, &text_pin_rect, 300, true);
}

void animate_code_off() {
	GRect finish = text_pin_rect;
	switch(animation_direction)
	{
		case UP :
			finish.origin.y -= display_bounds.size.h;
			break;
		case DOWN :
			finish.origin.y += display_bounds.size.h;
			break;
		case LEFT :
			finish.origin.x -= display_bounds.size.w;
			break;
		case RIGHT :
			finish.origin.x += display_bounds.size.w;
			break;
	}
	animate_layer(text_layer_get_layer(text_pin_layer), AnimationCurveEaseIn, &text_pin_rect, &finish, 300, true);
}

void animate_second_counter(bool toZero) {
	time_t temp = time(NULL);
	struct tm *time = localtime(&temp);
	int seconds = 30-(time->tm_sec%30);

	if (seconds == 30)
		otp_update_tick++;
			
	if	(otp_updated_at_tick != otp_update_tick) {
		animation_state = 20;
		animation_control();
	}
	
	// update countdown layer
	GRect start = layer_get_frame(text_layer_get_layer(countdown_layer));
	GRect finish = countdown_rect;

	finish.size.w = ((float)4.8) * seconds;

	#ifdef PBL_COLOR
		switch(seconds)
		{
			case 6 :
				text_layer_set_background_color(countdown_layer, GColorRed);
				break;
			case 5 :
				text_layer_set_background_color(countdown_layer, fg_color);
				break;
			case 4 :
				text_layer_set_background_color(countdown_layer, GColorRed);
				break;
			case 3 :
				text_layer_set_background_color(countdown_layer, fg_color);
				break;
			case 2 :
				text_layer_set_background_color(countdown_layer, GColorRed);
				break;
			case 1 :
				text_layer_set_background_color(countdown_layer, fg_color);
				break;
		}
	#endif
	
	int animationTime = 900;
	if (toZero) {
		finish.origin.y = display_bounds.size.h;
		//animationTime = 300;
		animate_layer(text_layer_get_layer(countdown_layer), AnimationCurveEaseInOut, &start, &finish, animationTime, true);
	} else {
		if (seconds % 30 == 0) {
			animate_layer(text_layer_get_layer(countdown_layer), AnimationCurveEaseInOut, &start, &finish, animationTime, false);
		}
		else
			animate_layer(text_layer_get_layer(countdown_layer), AnimationCurveLinear, &start, &finish, animationTime, false);
	}
}

void animation_control() {	
	if (animation_count > 0) {
		animation_count--;
		return;
	}
	
	switch (animation_state) {
		case 0: // initial launch, animate the code and label on screen
			animation_state = 10;
			if (fonts_changed)
				set_fonts();
			animate_code_on();
			animate_label_on();
			break;
		case 10: // just update the second counter
			override_second_counter = false;
			break;
		case 20: // animate the code off screen
			animation_state = 30;
			animation_direction = DOWN;
			animate_code_off();
			break;
		case 30: // animate the code on screen
			animation_state = 10;
			animation_direction = LEFT;
			animate_code_on();
			break;
		case 40: // animate the code and label off screen
			animation_state = 0;
			animation_count = 1;
			animate_code_off();
			animate_label_off();
			break;
		case 50: // animate the code and label and second counter off screen
			animation_state = 60;
			animation_unschedule_all();
			override_second_counter = true;
			animation_count = 2;
			animation_direction = RIGHT;
			animate_code_off();
			animate_label_off();
			animate_second_counter(true);
			break;
		case 60: // animate the swipe layer on screen using the background color
			animation_state = 70;
			GRect start = GRect(0, display_bounds.size.h*-1, display_bounds.size.w, display_bounds.size.h);
			swipe_layer = text_layer_create(start);
			text_layer_set_background_color(swipe_layer, bg_color);
			Layer *window_layer = window_get_root_layer(main_window);
			layer_add_child(window_layer, text_layer_get_layer(swipe_layer));
			animate_layer(text_layer_get_layer(swipe_layer), AnimationCurveEaseInOut, &start, &display_bounds, 500, true);
			break;
		case 70: // clear the swipe layer and set the new colors
			apply_display_colors();
			text_layer_destroy(swipe_layer);
			animation_state = 0;
			animation_control();
			break;
	}
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	
	if (idle_timeout > 0) {
		// If app is idle after X minutes then exit
		if (idle_second_count >= idle_timeout) {
			if (DEBUG)
				APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Timer reached %d, exiting", idle_second_count);
			window_stack_pop_all(true);
			return;
		}
		else
			idle_second_count += 1;	
	}
	
	if (!loading_complete || override_second_counter)
		return;
	
	animate_second_counter(false);
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	resetIdleTime();
	if (watch_otp_count) {
		if (otp_selected == 0)
			otp_selected = (watch_otp_count-1);
		else
			otp_selected--;
		
		refresh_screen_data(DOWN);
	}
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	resetIdleTime();
	if (watch_otp_count) {
		if (otp_selected == (watch_otp_count-1))
			otp_selected = 0;
		else
			otp_selected++;
		
		refresh_screen_data(UP);
	}
}

void sendJSMessage(Tuplet data_tuple) {
	DictionaryIterator *iter;
	int begin = app_message_outbox_begin(&iter);
	
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Begin sendJSMessage = %d", begin);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &data_tuple);
	dict_write_end(iter);
	
	int send = app_message_outbox_send();
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Send sendJSMessage = %d", send);
}

void request_delete(int key_id) {
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Pebble Requesting delete: %s", otp_keys[key_id]);

	sendJSMessage(MyTupletCString(JS_DELETE_KEY, otp_keys[key_id]));
}

void request_key(int code_id) {
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Requesting code: %d", code_id);

	sendJSMessage(TupletInteger(JS_REQUEST_KEY, code_id));
}

void send_key(int requested_key) {
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Phone Requesting key: %d", requested_key);

	char keylabelpair[MAX_COMBINED_LENGTH];
	
	if (persist_exists(PS_SECRET+requested_key)) {
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: SENDING CODE FROM LOCATION %d", PS_SECRET+requested_key);

		persist_read_string(PS_SECRET+requested_key, keylabelpair, MAX_COMBINED_LENGTH);
	}
	else
		strcat(keylabelpair,"NULL");

	sendJSMessage(MyTupletCString(JS_TRANSMIT_KEY, keylabelpair));
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	resetIdleTime();
	if (watch_otp_count)
		push_select_window(otp_selected);
}

void window_config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
	js_message_retry_count = 0;
	
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Outgoing Message Delivered");
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Outgoing Message Failed");
	
	if (requesting_code > 0 && js_message_retry_count < js_message_max_retry_count) {
		js_message_retry_count++;
		
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: RETRY:%d REQUESTING ANOTHER!", js_message_retry_count);
		
		request_key(requesting_code);
	}
}


void apply_display_colors() {
	window_set_background_color(main_window, bg_color);
	text_layer_set_background_color(countdown_layer, fg_color);
	text_layer_set_text_color(text_label_layer, fg_color);
	text_layer_set_text_color(text_pin_layer, fg_color);
}

#ifdef PBL_COLOR

	unsigned int HexStringToUInt(char const* hexstring)
	{
		unsigned int result = 0;
		char const *c = hexstring;
		unsigned char thisC;

		while( (thisC = *c) != 0 )
		{
			thisC = toupper(thisC);
			result <<= 4;

			if( isdigit(thisC))
				result += thisC - '0';
			else if(isxdigit(thisC))
				result += thisC - 'A' + 10;
			else
			{
				APP_LOG(APP_LOG_LEVEL_DEBUG, "ERROR: Unrecognised hex character \"%c\"", thisC);
				return 0;
			}
			++c;
		}
		return result;  
	}


	void set_display_colors() {

		char str_color1[7];
		char str_color2[7];

		if (strlen(basalt_colors) == 12) {
			memcpy(str_color1, &basalt_colors[0], 6);
			memcpy(str_color2, &basalt_colors[6], 6);
			str_color1[6] = '\0';
			str_color2[6] = '\0';
		}
		else
			return;

		int color1 = HexStringToUInt(str_color1);
		int color2 = HexStringToUInt(str_color2);

		bg_color = GColorFromHEX(color1);
		fg_color = GColorFromHEX(color2);
	}

#else

	void set_display_colors() { 
		if (aplite_theme) {
			bg_color = GColorWhite;
			fg_color = GColorBlack;
		} else {
			bg_color = GColorBlack;
			fg_color = GColorWhite;
		}
}
	
#endif

void set_fonts() {
	if (font_label.isCustom)
		fonts_unload_custom_font(font_label.font);

	if (font_pin.isCustom)
		fonts_unload_custom_font(font_pin.font);
	
	switch(font_style)
	{
		case 1 :
			font_label.font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
			font_label.isCustom = false;
			text_label_rect.origin.y = 58;
			text_label_rect.size.h = 40;
			font_pin.font = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
			font_pin.isCustom = false;
			break;
		case 2 :
			font_label.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_28));
			font_label.isCustom = true;
			text_label_rect.origin.y = 58;
			text_label_rect.size.h = 40;
			font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_38));
			font_pin.isCustom = true;
			break;
		case 3 :
			font_label.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BD_CARTOON_20));
			font_label.isCustom = true;
			text_label_rect.origin.y = 52;
			text_label_rect.size.h = 22;
			font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BD_CARTOON_28));
			font_pin.isCustom = true;
			break;
		default :
			font_style = 0;
			font_label.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_28));
			font_label.isCustom = true;
			text_label_rect.origin.y = 50;
			text_label_rect.size.h = 40;
			font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITWISE_32));
			font_pin.isCustom = true;
			break;
	}
	text_layer_set_font(text_label_layer, font_label.font);
	text_layer_set_font(text_pin_layer, font_pin.font);
	fonts_changed = false;
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	// Check for fields you expect to receive
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Message Received");
	
	resetIdleTime();
	Tuple *key_count_tuple = dict_find(iter, JS_KEY_COUNT);
	Tuple *key_tuple = dict_find(iter, JS_TRANSMIT_KEY);
	Tuple *timezone_tuple = dict_find(iter, JS_TIMEZONE);
	Tuple *key_delete_tuple = dict_find(iter, JS_DELETE_KEY);
	Tuple *font_style_tuple = dict_find(iter, JS_FONT_STYLE);
	Tuple *delete_all_tuple = dict_find(iter, JS_DELETE_ALL);
	Tuple *idle_timeout_tuple = dict_find(iter, JS_IDLE_TIMEOUT);
	Tuple *key_request_tuple = dict_find(iter, JS_REQUEST_KEY);
	
	#ifdef PBL_COLOR
		Tuple *basalt_colors_tuple = dict_find(iter, JS_BASALT_COLORS);
	#else
		Tuple *aplite_theme_tuple = dict_find(iter, JS_THEME);
	#endif
	
	// Act on the found fields received
	if (delete_all_tuple) {
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Delete all requested");
		
		for (unsigned int i = 0; i < MAX_OTP; i++) {
			if (DEBUG)
				APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Deleting key: %d", i);
			persist_delete(PS_SECRET+i);
		}
		otp_default = 0;
		watch_otp_count = 0;
		phone_otp_count = 0;
		otp_selected = 0;
		persist_write_int(PS_DEFAULT_KEY, otp_default);
		refresh_screen_data(DOWN);
	} // delete_all_tuple
	
	if (key_count_tuple) {
		phone_otp_count = key_count_tuple->value->int16;
		
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Key count from watch: %d", watch_otp_count);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Key count from phone: %d", phone_otp_count);
		
		if (watch_otp_count < phone_otp_count) {
			if (DEBUG)
				APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: REQUESTING CODES");
			loading_complete = false;
			requesting_code = 1;
			request_key(requesting_code++);
		}
	} // key_count_tuple
	
	if (key_tuple) {
		char key_value[MAX_COMBINED_LENGTH];
		memcpy(key_value, key_tuple->value->cstring, key_tuple->length);
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Text: %s", key_value);
		expand_key(key_value, true);
		check_load_status();
	} // key_tuple
	
	if (key_delete_tuple) {
		char key_value[MAX_COMBINED_LENGTH];
		memcpy(key_value, key_delete_tuple->value->cstring, key_delete_tuple->length);
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Deleting requested Key: %s", key_value);
		
		unsigned int key_found = MAX_OTP;
		for(unsigned int i = 0; i < watch_otp_count; i++) {
			if (strcmp(key_value, otp_keys[i]) == 0)
				key_found = i;
		}
		
		if(key_found < MAX_OTP) {
			char buff[MAX_COMBINED_LENGTH];
			for (unsigned int i = key_found; i < watch_otp_count; i++) {
				strcpy(otp_keys[i], otp_keys[i+1]);
				strcpy(otp_labels[i], otp_labels[i+1]);
				
				buff[0] = '\0';
				strcat(buff,otp_labels[i]);
				strcat(buff,":");
				strcat(buff,otp_keys[i]);
				persist_write_string(PS_SECRET+i, buff);
			}
			watch_otp_count--;
			persist_delete(PS_SECRET+watch_otp_count);
			
			if (otp_selected >= key_found) {
				if (otp_selected == key_found)
					refresh_screen_data(DOWN);
				otp_selected--;
			}
			
			if (otp_default > 0 && otp_default >= key_found)
				otp_default--;
		}
	} // key_delete_tuple
	
	if (timezone_tuple) {
		int tz_offset = timezone_tuple->value->int16;
		if (tz_offset != timezone_offset) {
			timezone_offset = tz_offset;
			persist_write_int(PS_TIMEZONE_KEY, timezone_offset);
			#ifdef PBL_SDK_2
				refresh_screen_data(DOWN);
			#endif
		}
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Timezone Offset: %d", timezone_offset);
	} // timezone_tuple

	#ifdef PBL_COLOR
		
		if (basalt_colors_tuple) {
			char basalt_colors_value[13];
			memcpy(basalt_colors_value, basalt_colors_tuple->value->cstring, basalt_colors_tuple->length);
		
			if (strcmp(basalt_colors, basalt_colors_value) != 0) {
				memcpy(basalt_colors, basalt_colors_value, 13);
				if (DEBUG)
					APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: basalt_colors: %s", basalt_colors);
				persist_write_string(PS_BASALT_COLORS, basalt_colors);
				set_display_colors();
				animation_state = 50;
				animation_control();
			}		
		} // basalt_colors_tuple
	
	#else

		if (aplite_theme_tuple) {
			unsigned int aplite_theme_value = aplite_theme_tuple->value->int16;
			if (aplite_theme != aplite_theme_value) {
				aplite_theme = aplite_theme_value;
				persist_write_int(PS_APLITE_THEME, aplite_theme);
				set_display_colors();
				animation_state = 50;
				animation_control();
				if (DEBUG)
					APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Theme: %d", aplite_theme);
			}
		} // aplite_theme_tuple
	
	#endif
	
	if (font_style_tuple) {
		unsigned int font_style_value = font_style_tuple->value->int16;
		if (font_style != font_style_value) {
			font_style = font_style_value;
			persist_write_int(PS_FONT_STYLE, font_style);
			update_screen_fonts();
			if (DEBUG)
				APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Font style: %d", font_style);
		}
	} // font_style_tuple
	
	if (idle_timeout_tuple) {
		if (idle_timeout_tuple->value->int16 >= 0) {
			unsigned int idle_timeout_value = idle_timeout_tuple->value->int16;
			if (idle_timeout != idle_timeout_value) {
				idle_timeout = idle_timeout_value;
				persist_write_int(PS_IDLE_TIMEOUT, idle_timeout);
			}
		}	
		else
			idle_timeout = 0;
		
		resetIdleTime();
		if (DEBUG)
			APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Idle Timeout: %d", idle_timeout);
	} // idle_timeout_tuple
	
	if (key_request_tuple) {
		int requested_key_value = key_request_tuple->value->int16;
		send_key(requested_key_value);
	} // key_request_tuple
}

void in_dropped_handler(AppMessageResult reason, void *context) {
	// incoming message dropped
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Incoming Message Dropped");
}

void load_persistent_data() {	
	timezone_offset = persist_exists(PS_TIMEZONE_KEY) ? persist_read_int(PS_TIMEZONE_KEY) : 0;
	
	#ifdef PBL_COLOR
		if (persist_exists(PS_BASALT_COLORS))
			persist_read_string(PS_BASALT_COLORS, basalt_colors, 13);
		else
			memcpy(basalt_colors, "0000FFFFFFFF"+'\0', 13);
	#else
		aplite_theme = persist_exists(PS_APLITE_THEME) ? persist_read_int(PS_APLITE_THEME) : 0;
	#endif
		
	otp_default = persist_exists(PS_DEFAULT_KEY) ? persist_read_int(PS_DEFAULT_KEY) : 0;
	font_style = persist_exists(PS_FONT_STYLE) ? persist_read_int(PS_FONT_STYLE) : 0;
	idle_timeout = persist_exists(PS_IDLE_TIMEOUT) ? persist_read_int(PS_IDLE_TIMEOUT) : 300;
		
	if (persist_exists(PS_SECRET)) {
		for(int i = 0; i < MAX_OTP; i++) {
			if (persist_exists(PS_SECRET+i)) {
				if (DEBUG)
					APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: LOADING CODE FROM LOCATION %d", PS_SECRET+i);
				
				char keylabelpair[MAX_COMBINED_LENGTH];
				persist_read_string(PS_SECRET+i, keylabelpair, MAX_COMBINED_LENGTH);
				
				if (DEBUG)
					APP_LOG(APP_LOG_LEVEL_DEBUG, "'%s'", keylabelpair);
				
				expand_key(keylabelpair, false);
			}
			else
				break;
		}
	} else
		APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: NO CODES ON WATCH!");
	
	if (otp_default >= watch_otp_count)
		otp_default = 0;
	
	otp_selected = otp_default;
}

static void window_load(Window *window) {
	window_set_click_config_provider(main_window, (ClickConfigProvider) window_config_provider);
	
	Layer *window_layer = window_get_root_layer(main_window);
	display_bounds = layer_get_frame(window_layer);
	
	countdown_rect = (GRect(0, display_bounds.size.h-10, display_bounds.size.w, 10));
	GRect countdown_start = countdown_rect;
	countdown_start.size.w = 0;
	countdown_layer = text_layer_create(countdown_start);
	layer_add_child(window_layer, text_layer_get_layer(countdown_layer));
	
	text_label_rect = GRect(2, 50, display_bounds.size.w-2, 40);
	text_label_layer = text_layer_create(text_label_rect);
	text_layer_set_background_color(text_label_layer, GColorClear);
	text_layer_set_text_alignment(text_label_layer, GTextAlignmentLeft);
	text_layer_set_overflow_mode(text_label_layer, GTextOverflowModeWordWrap);
	text_layer_set_text(text_label_layer, label_text);
	layer_add_child(window_layer, text_layer_get_layer(text_label_layer));
	
	text_pin_rect = GRect(0, 80, display_bounds.size.w, 40);
	text_pin_layer = text_layer_create(text_pin_rect);
	text_layer_set_background_color(text_pin_layer, GColorClear);
	text_layer_set_text_alignment(text_pin_layer, GTextAlignmentCenter);
	text_layer_set_text(text_pin_layer, pin_text);
	layer_add_child(window_layer, text_layer_get_layer(text_pin_layer));
	
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
	
	set_display_colors();
	apply_display_colors();
	set_fonts();
	loading_complete = true;
	animation_control();
}

void window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	text_layer_destroy(countdown_layer);
	text_layer_destroy(text_label_layer);
	text_layer_destroy(text_pin_layer);
}

void handle_init(void) {
	load_persistent_data();
	
	main_window = window_create();
	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	
	#ifdef PBL_SDK_2
		window_set_fullscreen(main_window, true);
	#endif
	window_stack_push(main_window, false /* Animated */);
	
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);

	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());    //Largest possible input and output buffer size
}

void handle_deinit(void) {
	animation_unschedule_all();
	if (font_label.isCustom)
		fonts_unload_custom_font(font_label.font);
	if (font_pin.isCustom)
		fonts_unload_custom_font(font_pin.font);
	window_destroy(main_window);
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}