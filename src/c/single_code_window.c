#include "pebble.h"
#include "single_code_window.h"
#include "main.h"
#include "select_window.h"
#include "google-authenticator.h"
#include "display.h"

// Main Window
static Window *single_code_main_window;
static TextLayer *text_pin_layer;
static TextLayer *text_label_layer;
static TextLayer *swipe_layer;
static Layer *single_code_graphics_layer;

static GRect text_pin_rect;
static GRect text_label_rect;
static GRect display_bounds;

unsigned int animation_direction = LEFT;
unsigned int animation_state = 0;
unsigned int animation_count = 0;

char label_text[MAX_LABEL_LENGTH];
char pin_text[MAX_KEY_LENGTH];

AppTimer *single_code_graphics_timer;
bool single_code_exiting = false;

int single_code_countdown_size = 0;
bool countdown_layer_onscreen = false;

void single_code_refresh_callback(void *data) {
	if (!single_code_exiting)
		layer_mark_dirty(single_code_graphics_layer);
}

static void update_graphics(Layer *layer, GContext *ctx) {
	draw_countdown_graphic(&layer, &ctx, &single_code_countdown_size, countdown_layer_onscreen);                               
	if (!single_code_exiting)
		single_code_graphics_timer = app_timer_register(30, (AppTimerCallback) single_code_refresh_callback, NULL);
}


// Functions requiring early declaration
void animation_control(void);
void set_fonts(void);
void apply_display_colors();

void refresh_screen_data(int direction) {
	if (!loading_complete)
		return;

	animation_direction = direction;
	animation_state = 40;
	animation_control();
}

void on_animation_stopped_callback(Animation *anim, bool finished, void *context) {
	// 	#ifdef PBL_SDK_2
	// 		property_animation_destroy((PropertyAnimation*) anim);
	// 		anim = NULL;
	// 	#endif
	animation_control();
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
	animate_layer(text_layer_get_layer(text_label_layer), AnimationCurveEaseOut, &start, &text_label_rect, 300, on_animation_stopped_callback);
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
	animate_layer(text_layer_get_layer(text_label_layer), AnimationCurveEaseIn, &text_label_rect, &finish, 300, on_animation_stopped_callback);
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
	animate_layer(text_layer_get_layer(text_pin_layer), AnimationCurveEaseOut, &start, &text_pin_rect, 300, on_animation_stopped_callback);
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
	animate_layer(text_layer_get_layer(text_pin_layer), AnimationCurveEaseIn, &text_pin_rect, &finish, 300, on_animation_stopped_callback);
}

void animation_control(void) {	
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
		countdown_layer_onscreen = true;
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
		animation_count = 1;
		animation_direction = RIGHT;
		countdown_layer_onscreen = false;
		animate_code_off();
		animate_label_off();
		break;
		case 60: // animate the swipe layer on screen using the background color
		animation_state = 70;
		GRect start = GRect(0, display_bounds.size.h*-1, display_bounds.size.w, display_bounds.size.h);
		swipe_layer = text_layer_create(start);
		text_layer_set_background_color(swipe_layer, bg_color);
		Layer *window_layer = window_get_root_layer(single_code_main_window);
		layer_add_child(window_layer, text_layer_get_layer(swipe_layer));
		animate_layer(text_layer_get_layer(swipe_layer), AnimationCurveEaseInOut, &start, &display_bounds, 500, on_animation_stopped_callback);
		break;
		case 70: // set the new colors and clear the swipe layer
		apply_display_colors();
		text_layer_destroy(swipe_layer);
		animation_state = 0;
		animation_control();
		break;
	}
}

void single_code_window_second_tick(int seconds) {

	if (seconds % 30 == 0)
		otp_update_tick++;

	if	(otp_updated_at_tick != otp_update_tick) {
		animation_state = 20;
		animation_control();
	}

	if (refresh_required) {
		refresh_required = false;

		if (colors_changed) {
			colors_changed = false;
			animation_state = 50;
			animation_control();
		}
		else
			refresh_screen_data(DOWN);	
	}
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

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	resetIdleTime();
	if (watch_otp_count)
		push_select_window(otp_selected);
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	resetIdleTime();
	switch_window_layout();
}

void window_config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
}

void apply_display_colors() {
	window_set_background_color(single_code_main_window, bg_color);
	// 	set_countdown_layer_color(fg_color);
	text_layer_set_text_color(text_label_layer, fg_color);
	text_layer_set_text_color(text_pin_layer, fg_color);
}

void set_fonts(void) {
	if (font_label.isCustom)
		fonts_unload_custom_font(font_label.font);

	if (font_pin.isCustom)
		fonts_unload_custom_font(font_pin.font);

	switch(font)
	{
		case 1 :
		font_label.font = fonts_get_system_font(FONT_KEY_GOTHIC_24);
		font_label.isCustom = false;
		font_pin.font = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
		font_pin.isCustom = false;
		text_label_rect.origin.y = 55;
		text_label_rect.size.h = 30;
		text_pin_rect.size.h = 50;
		text_pin_rect.origin.y = 76;
		break;
		case 2 :
		font_label.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_30));
		font_label.isCustom = true;
		font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_42));
		font_pin.isCustom = true;
		text_label_rect.origin.y = 52;
		text_label_rect.size.h = 40;
		text_pin_rect.size.h = 50;
		text_pin_rect.origin.y = 74;
		break;
		case 3 :
		font_label.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BD_CARTOON_20));
		font_label.isCustom = true;
		font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BD_CARTOON_30));
		font_pin.isCustom = true;
		text_label_rect.origin.y = 54;
		text_label_rect.size.h = 22;
		text_pin_rect.size.h = 50;
		text_pin_rect.origin.y = 78;
		break;
		default :
		font = 0;
		font_label.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORBITRON_28));
		font_label.isCustom = true;
		font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITWISE_32));
		font_pin.isCustom = true;
		text_label_rect.origin.y = 50;
		text_label_rect.size.h = 36;
		text_pin_rect.size.h = 50;
		text_pin_rect.origin.y = 80;
		break;
	}
	text_layer_set_font(text_label_layer, font_label.font);
	text_layer_set_font(text_pin_layer, font_pin.font);
	fonts_changed = false;
}

static void single_code_window_load(Window *window) {
	single_code_exiting = false;
	window_set_click_config_provider(single_code_main_window, (ClickConfigProvider) window_config_provider);

	Layer *window_layer = window_get_root_layer(single_code_main_window);
	display_bounds = layer_get_frame(window_layer);

	text_label_rect = GRect(2, 50, display_bounds.size.w-2, 40);
	GRect text_label_start = text_label_rect;
	text_label_start.origin.x = display_bounds.size.w;
	text_label_layer = text_layer_create(text_label_start);
	text_layer_set_background_color(text_label_layer, GColorClear);
	text_layer_set_text_alignment(text_label_layer, GTextAlignmentLeft);
	text_layer_set_overflow_mode(text_label_layer, GTextOverflowModeWordWrap);
	text_layer_set_text(text_label_layer, label_text);
	layer_add_child(window_layer, text_layer_get_layer(text_label_layer));

	text_pin_rect = GRect(0, 80, display_bounds.size.w, 50);
	GRect text_pin_start = text_pin_rect;
	text_pin_start.origin.x = display_bounds.size.w;
	text_pin_layer = text_layer_create(text_pin_start);
	text_layer_set_background_color(text_pin_layer, GColorClear);
	text_layer_set_text_alignment(text_pin_layer, GTextAlignmentCenter);
	text_layer_set_text(text_pin_layer, pin_text);
	layer_add_child(window_layer, text_layer_get_layer(text_pin_layer));

	single_code_graphics_layer = layer_create(display_bounds);
	layer_set_update_proc(single_code_graphics_layer, update_graphics);
	layer_add_child(window_layer, single_code_graphics_layer);

	apply_display_colors();
	set_fonts();
	loading_complete = true;
}

static void single_code_window_appear(Window *window) {
	if (!refresh_required) {
		animation_direction = LEFT;
		animation_state = 0;
		animation_control();
	}
}

void single_code_window_unload(Window *window) {
	single_code_exiting = true;
	app_timer_cancel(single_code_graphics_timer);
	text_layer_destroy(text_label_layer);
	text_layer_destroy(text_pin_layer);
	layer_destroy(single_code_graphics_layer);
	window_destroy(single_code_main_window);
	single_code_main_window = NULL;
}

void single_code_window_remove(void) {
	window_stack_remove(single_code_main_window, false);
}

void single_code_window_push(void) {
	if (!single_code_main_window) {
		single_code_main_window = window_create();
		window_set_window_handlers(single_code_main_window, (WindowHandlers) {
			.load = single_code_window_load,
			.unload = single_code_window_unload,
			.appear = single_code_window_appear,
		});
	}

	window_stack_push(single_code_main_window, false);
}