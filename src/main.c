#include "pebble.h"
#include "google-authenticator.c"

#define MAX_OTP 8
#define MAX_LABEL_LENGTH 7 // 6 + termination
#define MAX_KEY_LENGTH 33 // 32 + termination

	
// Main Window
Window *window;
static TextLayer *countdown_layer;
static TextLayer *text_pin_layer;
static TextLayer *text_label_layer;

static GRect text_pin_rect;
static GRect text_label_rect;


// Selection Window
Window *select_window;
static SimpleMenuLayer *key_menu_layer;
static SimpleMenuSection key_menu_sections[1];
static SimpleMenuItem key_menu_items[MAX_OTP];


// Details Window
Window *details_window;
static TextLayer *details_title_layer;
static TextLayer *details_key_layer;
static ActionBarLayer *details_action_bar_layer;
static GBitmap *image_icon_yes;
static GBitmap *image_icon_no;

static GColor bg_color;
static GColor fg_color;

static GFont font_BITWISE_32;
static GFont font_ORBITRON_28;
static GFont font_UNISPACE_20;

bool loading_complete = false;
bool refresh_data;
bool finish_refresh;
bool requesting_codes;

int timezone_offset = 0;
unsigned int theme = 0;  // 0 = Dark, 1 = Light

unsigned int phone_otp_count = 0;
unsigned int otp_count = 0;
unsigned int otp_selected = 0;
unsigned int otp_default = 0;

char otp_labels[MAX_OTP][MAX_LABEL_LENGTH];
char otp_keys[MAX_OTP][MAX_KEY_LENGTH];

// Persistant Storage Keys
enum {
	PS_TIMEZONE_KEY,
	PS_THEME,
	PS_DEFAULT_KEY,
	PS_SECRET = 0x40 // Needs 8 spaces, should always be last
};


// JScript Keys
enum {
	JS_KEY_COUNT,
	JS_REQUEST_KEY,
	JS_TRANSMIT_KEY,
	JS_TIMEZONE,
	JS_DISPLAY_MESSAGE,
	JS_THEME,
	JS_DELETE_KEY
};

void window_config_provider(Window *window);

void expand_key(char *inputString)
{
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
	
	bool key_found = false;
	for(unsigned int i = 0; i < otp_count; i++) {
		if (strcmp(otp_key, otp_keys[i]) == 0)
		{
			key_found = true;
			APP_LOG(APP_LOG_LEVEL_DEBUG, "Code exists...relabeling");
			strcpy(otp_labels[i], otp_label);
		}
	}
			
	if (!key_found)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Adding Code");
		strcpy(otp_keys[otp_count], otp_key);
		strcpy(otp_labels[otp_count], otp_label);
		
		otp_count++;
		if (loading_complete) {
			otp_selected = otp_count-1;
			refresh_data = true;
		}
	}
	
	if (requesting_codes && otp_count == phone_otp_count) {
		requesting_codes = false;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "FINISHED REQUESTING");
	}
}

void on_animation_stopped(Animation *anim, bool finished, void *context) {
	//Free the memory used by the Animation
	property_animation_destroy((PropertyAnimation*) anim);
}

void animate_layer(Layer *layer, AnimationCurve curve, GRect *start, GRect *finish, int duration, int delay) {
	//Declare animation
	PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
	
	//Set characteristics
	animation_set_duration((Animation*) anim, duration);
	animation_set_delay((Animation*) anim, delay);
	animation_set_curve((Animation*) anim, curve);
	
	//Set stopped handler to free memory
	AnimationHandlers handlers = {
		//The reference to the stopped handler is the only one in the array
		.stopped = (AnimationStoppedHandler) on_animation_stopped
	};
	animation_set_handlers((Animation*) anim, handlers, NULL);
	
	//Start animation!
	animation_schedule((Animation*) anim);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	
	int seconds = tick_time->tm_sec;
	
	if (refresh_data || seconds == 29 || seconds == 59)
	{
		if (refresh_data)
		{
			GRect finish = text_label_rect;
			finish.origin.y = 144;
			animate_layer(text_layer_get_layer(text_label_layer), AnimationCurveEaseIn, &text_label_rect, &finish, 300, 500);
			
			finish_refresh = true;
			refresh_data = false;
		}
		
		GRect finish = text_pin_rect;
		finish.origin.y = 174;
		animate_layer(text_layer_get_layer(text_pin_layer), AnimationCurveEaseIn, &text_pin_rect, &finish, 300, 500);
	}
	else if (finish_refresh || seconds == 0 || seconds == 30)
	{
		if (finish_refresh)
		{
			if (otp_count)
				text_layer_set_text(text_label_layer, otp_labels[otp_selected]);
			else
				text_layer_set_text(text_label_layer, "NO");
			
			GRect start = text_label_rect;
			start.origin.x = 144;
			animate_layer(text_layer_get_layer(text_label_layer), AnimationCurveEaseOut, &start, &text_label_rect, 300, 0);
			finish_refresh = false;
		}

		if (otp_count)
			text_layer_set_text(text_pin_layer, generateCode(otp_keys[otp_selected], timezone_offset));
		else
			text_layer_set_text(text_pin_layer, "SECRETS");
			
		
		GRect start = text_pin_rect;
		start.origin.x = 144;
		animate_layer(text_layer_get_layer(text_pin_layer), AnimationCurveEaseOut, &start, &text_pin_rect, 300, 0);
	}
	
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	GRect start = layer_get_frame(text_layer_get_layer(countdown_layer));
	GRect finish = (GRect(0, bounds.size.h-10, bounds.size.w, 10));
	float boxsize = (30-(seconds%30))/((double)30);
	finish.size.w = finish.size.w * boxsize;
	
	if (seconds == 30 || seconds == 0)
		animate_layer(text_layer_get_layer(countdown_layer), AnimationCurveEaseInOut, &start, &finish, 900, 0);
	else
		animate_layer(text_layer_get_layer(countdown_layer), AnimationCurveLinear, &start, &finish, 900, 0);
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (otp_count) {
		if (otp_selected == (otp_count-1))
			otp_selected = 0;
		else
			otp_selected++;
		
		refresh_data = true;
	}
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (otp_count) {
		if (otp_selected == 0)
			otp_selected = (otp_count-1);
		else
			otp_selected--;
		
		refresh_data = true;
	}
}

void request_code(int code_id) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Requesting code: %d", code_id);
	Tuplet request_tuple = TupletInteger(JS_REQUEST_KEY, code_id);
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &request_tuple);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

static void key_menu_select_callback(int index, void *ctx) {
	otp_selected = index;
	refresh_data = true;
	window_stack_push(details_window, true /* Animated */);
}

static void select_window_load(Window *window) {

	int num_menu_items = 0;
	
	
	for(unsigned int i = 0; i < otp_count; i++) {
		key_menu_items[num_menu_items++] = (SimpleMenuItem) {
			.title = otp_labels[i],
			.callback = key_menu_select_callback,
			.subtitle = otp_keys[i],
		};
	}
	
	// Bind the menu items to the corresponding menu sections
	key_menu_sections[0] = (SimpleMenuSection){
		.num_items = num_menu_items,
		.items = key_menu_items,
	};

	Layer *select_window_layer = window_get_root_layer(select_window);
	GRect bounds = layer_get_frame(select_window_layer);
	
	// Initialize the simple menu layer
	key_menu_layer = simple_menu_layer_create(bounds, select_window, key_menu_sections, 1, NULL);
	
	// Add it to the window for display
	layer_add_child(select_window_layer, simple_menu_layer_get_layer(key_menu_layer));
}

void select_window_unload(Window *window) {
	simple_menu_layer_destroy(key_menu_layer);
}

void details_actionbar_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	otp_default = otp_selected;
	window_stack_remove(select_window, false);
	window_stack_pop(true);
}

void details_actionbar_down_click_handler(ClickRecognizerRef recognizer, void *context) {
}

void details_actionbar_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) details_actionbar_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) details_actionbar_down_click_handler);
}


static void details_window_load(Window *window) {
	
	Layer *details_window_layer = window_get_root_layer(details_window);
	GRect bounds = layer_get_frame(details_window_layer);
	
	GRect title_text_rect = GRect(0, 0, bounds.size.w, 125);	
	details_title_layer = text_layer_create(title_text_rect);
	text_layer_set_background_color(details_title_layer, GColorClear);
	text_layer_set_text_alignment(details_title_layer, GTextAlignmentLeft);
	text_layer_set_font(details_title_layer, font_ORBITRON_28);
	layer_add_child(window_get_root_layer(details_window), text_layer_get_layer(details_title_layer));
	
	GRect key_text_rect = GRect(0, 50, 115, 125);
	details_key_layer = text_layer_create(key_text_rect);
	text_layer_set_background_color(details_key_layer, GColorClear);
	text_layer_set_text_alignment(details_key_layer, GTextAlignmentLeft);

	font_UNISPACE_20 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_UNISPACE_20));
	text_layer_set_font(details_key_layer, font_UNISPACE_20);
	layer_add_child(window_get_root_layer(details_window), text_layer_get_layer(details_key_layer));
	
	window_set_background_color(details_window, bg_color);
	text_layer_set_text_color(details_title_layer, fg_color);
	text_layer_set_text_color(details_key_layer, fg_color);
	
	text_layer_set_text(details_title_layer, otp_labels[otp_selected]);
	text_layer_set_text(details_key_layer, otp_keys[otp_selected]);
	
	image_icon_yes = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_YES);
	image_icon_no = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_NO);
	
	// Initialize the action bar:
	details_action_bar_layer = action_bar_layer_create();
	action_bar_layer_set_background_color(details_action_bar_layer, fg_color);
	action_bar_layer_set_click_config_provider(details_action_bar_layer, details_actionbar_config_provider);
	action_bar_layer_set_icon(details_action_bar_layer, BUTTON_ID_UP, image_icon_yes);
	action_bar_layer_set_icon(details_action_bar_layer, BUTTON_ID_DOWN, image_icon_no);
	action_bar_layer_add_to_window(details_action_bar_layer, details_window);
}

void details_window_unload(Window *window) {
	action_bar_layer_destroy(details_action_bar_layer);
	gbitmap_destroy(image_icon_yes);
	gbitmap_destroy(image_icon_no);
	text_layer_destroy(details_title_layer);
	text_layer_destroy(details_key_layer);
	fonts_unload_custom_font(font_UNISPACE_20);
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {	
	window_stack_push(select_window, true /* Animated */);
}

void window_config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing Message Delivered");
	
	if (requesting_codes) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "REQUESTING ANOTHER!");
		request_code(otp_count+1);
	}
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing Message Failed");
}


void set_theme() { 
	if (theme) {
		bg_color = GColorWhite;
		fg_color = GColorBlack;
	} else {
		bg_color = GColorBlack;
		fg_color = GColorWhite;
	}
	
	window_set_background_color(window, bg_color);
	text_layer_set_background_color(countdown_layer, fg_color);
	text_layer_set_text_color(text_label_layer, fg_color);
	text_layer_set_text_color(text_pin_layer, fg_color);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	// Check for fields you expect to receive
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Recieved");
	
	Tuple *key_count_tuple = dict_find(iter, JS_KEY_COUNT);
	Tuple *key_tuple = dict_find(iter, JS_TRANSMIT_KEY);
	Tuple *timezone_tuple = dict_find(iter, JS_TIMEZONE);
	Tuple *theme_tuple = dict_find(iter, JS_THEME);
	
	// Act on the found fields received
	if (key_count_tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Key count from phone: %d", key_count_tuple->value->int16);
		phone_otp_count = key_count_tuple->value->int16;
		
		if (otp_count == 0 && phone_otp_count > 0) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "REQUESTING CODES!!!");
			requesting_codes = true;
			request_code(otp_count+1);
		}
	}
	if (key_tuple) {
		char key_value[40];
		memcpy(key_value, key_tuple->value->cstring, key_tuple->length);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Text: %s", key_value);
		expand_key(key_value);
	}
	
	if (timezone_tuple)
	{
		int tz_offset = timezone_tuple->value->int16;
		if (tz_offset != timezone_offset)
		{
			timezone_offset = tz_offset;
			refresh_data = true;
		}
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Timezone Offset: %d", timezone_offset);
	}
	
	if (theme_tuple)
	{
		theme = theme_tuple->value->int16;
		set_theme();
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Theme: %d", theme);
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {
	// incoming message dropped
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming Message Dropped");
}

void load_persistent_data() {
	timezone_offset = persist_exists(PS_TIMEZONE_KEY) ? persist_read_int(PS_TIMEZONE_KEY) : 0;
	theme = persist_exists(PS_THEME) ? persist_read_int(PS_THEME) : 0;
	otp_default = persist_exists(PS_DEFAULT_KEY) ? persist_read_int(PS_DEFAULT_KEY) : 0;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Default Key: %d", otp_default);
	if (persist_exists(PS_SECRET))
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "LOADING FROM WATCH");
		for(int i = 0; i < MAX_OTP; i++) {
			if (persist_exists(PS_SECRET+i)) {
				char keylabelpair[40];
				persist_read_string(PS_SECRET+i, keylabelpair, 40);
				expand_key(keylabelpair);
			}
			else
				break;
		}
	}
	
	if (otp_default < otp_count)
		otp_selected = otp_default;

	loading_complete = true;
}

void save_persistent_data() {
	persist_write_int(PS_TIMEZONE_KEY, timezone_offset);
	persist_write_int(PS_THEME, theme);
	persist_write_int(PS_DEFAULT_KEY, otp_default);
	
	char buff[40];
	for(int i = 0; i < MAX_OTP; i++) {
		buff[0] = '\0';
		if(strlen(otp_labels[i])) {
			strcat(buff,otp_labels[i]);
			strcat(buff,":");
			strcat(buff,otp_keys[i]);
			APP_LOG(APP_LOG_LEVEL_DEBUG, buff);
			persist_write_string(PS_SECRET+i, buff);
		}
	}
}

static void window_load(Window *window) {
	window_set_click_config_provider(window, (ClickConfigProvider) window_config_provider);
	
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	
	countdown_layer = text_layer_create(GRect(0, bounds.size.h-10, 0, 10));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(countdown_layer));
	
	text_label_rect = GRect(0, 30, bounds.size.w, 125);
	GRect text_label_start_rect  = text_label_rect;
	text_label_start_rect.origin.x = 144;
	text_label_layer = text_layer_create(text_label_start_rect);
	text_layer_set_background_color(text_label_layer, GColorClear);
	text_layer_set_text_alignment(text_label_layer, GTextAlignmentLeft);
	text_layer_set_font(text_label_layer, font_ORBITRON_28);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_label_layer));
	
	text_pin_rect = GRect(0, 60, bounds.size.w, 125);
	GRect text_pin_start_rect = text_pin_rect;
	text_pin_start_rect.origin.x = 144;
	text_pin_layer = text_layer_create(text_pin_start_rect);
	text_layer_set_background_color(text_pin_layer, GColorClear);
	text_layer_set_text_alignment(text_pin_layer, GTextAlignmentCenter);
	text_layer_set_font(text_pin_layer, font_BITWISE_32);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_pin_layer));
	
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
	
	set_theme();
	
	finish_refresh = true;
}

void window_unload(Window *window) {
	tick_timer_service_unsubscribe();
	text_layer_destroy(countdown_layer);
	text_layer_destroy(text_label_layer);
	text_layer_destroy(text_pin_layer);
}

void handle_init(void) {
	load_persistent_data();
	
	font_ORBITRON_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_28));
	font_BITWISE_32 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITWISE_32));
	
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	
	select_window = window_create();
	window_set_window_handlers(select_window, (WindowHandlers) {
		.load = select_window_load,
		.unload = select_window_unload,
	});
	
	details_window = window_create();
	window_set_window_handlers(details_window, (WindowHandlers) {
		.load = details_window_load,
		.unload = details_window_unload,
	});
	
	window_stack_push(window, true /* Animated */);
	
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	
	const uint32_t inbound_size = 128;
	const uint32_t outbound_size = 64;
	app_message_open(inbound_size, outbound_size);
}

void handle_deinit(void) {
	save_persistent_data();
	animation_unschedule_all();
	fonts_unload_custom_font(font_ORBITRON_28);
	fonts_unload_custom_font(font_BITWISE_32);
	window_destroy(details_window);
	window_destroy(select_window);
	window_destroy(window);
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}