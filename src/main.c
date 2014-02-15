#include "pebble.h"
#include "google-authenticator.c"
#define MAX_OTP 8
#define MAX_LABEL_LENGTH 6
#define MAX_KEY_LENGTH 32

Window *window;
static TextLayer *countdown_layer;
static TextLayer *text_pin_layer;
static TextLayer *text_label_layer;
static ActionBarLayer *action_bar_layer;

static GRect text_pin_rect;
static GRect text_label_rect;

static GFont font_BITWISE_32;
static GFont font_ORBITRON_28;

// Some various bitmaps for actionbar-buttons
static GBitmap *image_icon_yes;
static GBitmap *image_icon_no;

bool refresh_data = false;
bool finish_refresh = true;

int timezone_offset = 0;

enum {
	PS_TIMEZONE_KEY = 0x0,
	PS_SECRET = 0x1,
};

int otp_count = 0;
int otpselected = 0;
char otplabels[MAX_OTP][MAX_LABEL_LENGTH+1];
char otpkeys[MAX_OTP][MAX_KEY_LENGTH+1];

char label_override_text[MAX_LABEL_LENGTH+1] = "NO";
char pin_override_text[MAX_LABEL_LENGTH+1] = "SECRET";

enum {
	JS_KEY_COUNT = 0x0,
	JS_REQUEST_KEY = 0x1,
	JS_TRANSMIT_KEY = 0x2,
	JS_TIMEZONE = 0x3,
	JS_DISPLAY_MESSAGE = 0x4
};

void expand_key(char *inputString)
{
	bool colonFound = false;
	int outputChar = 0;
	
	for(unsigned int i = 0; i < strlen(inputString); i++) {
		if (inputString[i] == ':') {
			otplabels[otp_count][outputChar] = '\0';
			colonFound = true;
			outputChar = 0;
		} else {
			if (colonFound) 
				otpkeys[otp_count][outputChar] = inputString[i]; 
			else
				otplabels[otp_count][outputChar] = inputString[i];
			
			outputChar++;
		}
	}
	otpkeys[otp_count][outputChar] = '\0';
	
	pin_override_text[0] = '\0';
	label_override_text[0] = '\0';

	otp_count++;
	otpselected = otp_count-1;
	refresh_data = true;
}

void on_animation_stopped(Animation *anim, bool finished, void *context)
{
	//Free the memory used by the Animation
	property_animation_destroy((PropertyAnimation*) anim);
}

void animate_layer(Layer *layer, AnimationCurve curve, GRect *start, GRect *finish, int duration, int delay)
{
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
			if(strlen(label_override_text))
				text_layer_set_text(text_label_layer, label_override_text);
			else
				text_layer_set_text(text_label_layer, otplabels[otpselected]);
			GRect start = text_label_rect;
			start.origin.x = 144;
			animate_layer(text_layer_get_layer(text_label_layer), AnimationCurveEaseOut, &start, &text_label_rect, 300, 0);
			finish_refresh = false;
		}
		if(strlen(pin_override_text))
			text_layer_set_text(text_pin_layer, pin_override_text);
		else
			text_layer_set_text(text_pin_layer, generateCode(otpkeys[otpselected], timezone_offset));
			
		
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

	if (otpselected == (otp_count-1))
		otpselected = 0;
	else
		otpselected++;

	refresh_data = true;
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (otpselected == 0)
		otpselected = (otp_count-1);
	else
		otpselected--;

	refresh_data = true;
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	Tuplet request_tuple = TupletInteger(JS_REQUEST_KEY, otp_count);
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &request_tuple);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Adding ActionBar");
	action_bar_layer_add_to_window(action_bar_layer, window);
	strcpy(pin_override_text, "DROP");
	refresh_data = true;
}

void window_config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_SELECT, 750, select_long_click_handler, NULL);
}

void actionbar_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Removing ActionBar");
	pin_override_text[0] = '\0';
	action_bar_layer_remove_from_window(action_bar_layer);
	window_set_click_config_provider(window, (ClickConfigProvider) window_config_provider);
	refresh_data = true;
}

void actionbar_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_DOWN, actionbar_up_click_handler);
  //window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) my_previous_click_handler);
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing Message Delivered");
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// outgoing message failed
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing Message Failed");
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	// Check for fields you expect to receive
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Recieved");
	
	Tuple *key_tuple = dict_find(iter, JS_TRANSMIT_KEY);
	Tuple *timezone_tuple = dict_find(iter, JS_TIMEZONE);
	
	// Act on the found fields received
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
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Int: %d", timezone_offset);
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {
	// incoming message dropped
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming Message Dropped");
}

void load_persistent_data() {
	timezone_offset = persist_exists(PS_TIMEZONE_KEY) ? persist_read_int(PS_TIMEZONE_KEY) : 0;
	
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
}

void save_persistent_data() {
	persist_write_int(PS_TIMEZONE_KEY, timezone_offset);
	
	char buff[40];
	for(int i = 0; i < MAX_OTP; i++) {
		buff[0] = '\0';
		if(strlen(otplabels[i])) {
			strcat(buff,otplabels[i]);
			strcat(buff,":");
			strcat(buff,otpkeys[i]);
			APP_LOG(APP_LOG_LEVEL_DEBUG, buff);
			persist_write_string(PS_SECRET+i, buff);
		}
	}
}

void handle_init(void) {
	load_persistent_data();
	window = window_create();
	window_set_background_color(window, GColorBlack);
	
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);

	countdown_layer = text_layer_create(GRect(0, bounds.size.h-10, 0, 10));
	text_layer_set_background_color(countdown_layer, GColorWhite);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(countdown_layer));
	
	text_label_rect = GRect(0, 30, bounds.size.w, 125);
	GRect text_label_start_rect  = text_label_rect;
	text_label_start_rect.origin.x = 144;
	text_label_layer = text_layer_create(text_label_start_rect);
	text_layer_set_text_color(text_label_layer, GColorWhite);
	text_layer_set_background_color(text_label_layer, GColorClear);
	text_layer_set_text_alignment(text_label_layer, GTextAlignmentLeft);
	font_ORBITRON_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_28));
	text_layer_set_font(text_label_layer, font_ORBITRON_28);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_label_layer));
	
	text_pin_rect = GRect(0, 60, bounds.size.w, 125);
	GRect text_pin_start_rect = text_pin_rect;
	text_pin_start_rect.origin.x = 144;
	text_pin_layer = text_layer_create(text_pin_start_rect);
	text_layer_set_text_color(text_pin_layer, GColorWhite);
	text_layer_set_background_color(text_pin_layer, GColorClear);
	text_layer_set_text_alignment(text_pin_layer, GTextAlignmentCenter);
	font_BITWISE_32 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITWISE_32));
	text_layer_set_font(text_pin_layer, font_BITWISE_32);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_pin_layer));
	
	window_set_click_config_provider(window, (ClickConfigProvider) window_config_provider);
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
	
	window_stack_push(window, true /* Animated */);
	
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	
	const uint32_t inbound_size = 128;
	const uint32_t outbound_size = 64;
	app_message_open(inbound_size, outbound_size);
	
	image_icon_yes = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_YES);
	image_icon_no = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_NO);
	
	// Initialize the action bar:
	action_bar_layer = action_bar_layer_create();
	action_bar_layer_set_background_color(action_bar_layer, GColorWhite);
	action_bar_layer_set_click_config_provider(action_bar_layer, actionbar_config_provider);
	action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_UP, image_icon_yes);
    action_bar_layer_set_icon(action_bar_layer, BUTTON_ID_DOWN, image_icon_no);
}

void handle_deinit(void) {
	save_persistent_data();
	tick_timer_service_unsubscribe();
	animation_unschedule_all();
	fonts_unload_custom_font(font_ORBITRON_28);
	fonts_unload_custom_font(font_BITWISE_32);
	// Cleanup the image
  	gbitmap_destroy(image_icon_yes);
	gbitmap_destroy(image_icon_no);
	action_bar_layer_destroy(action_bar_layer);
	text_layer_destroy(countdown_layer);
	text_layer_destroy(text_label_layer);
	text_layer_destroy(text_pin_layer);
	window_destroy(window);
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}