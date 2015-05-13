#include "pebble.h"
#include "main.h"
#include "dod_window.h"


static Window *dord_main_window;
static TextLayer *dord_label_layer;
static BitmapLayer *dord_icon_layer;
static ActionBarLayer *dord_action_bar_layer;

static GBitmap *dord_icon_bitmap, *dord_fav_bitmap, *dord_del_bitmap;
static int s_key_id;

void dod_actionbar_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	resetIdleTime();

	set_default_key(s_key_id);

	close_select_window();
	window_stack_pop(true);
}

void dod_actionbar_down_click_handler(ClickRecognizerRef recognizer, void *context) {
	resetIdleTime();
	request_delete(s_key_id);

	close_select_window();
	window_stack_pop(true);
}

void dod_actionbar_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) dod_actionbar_up_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) dod_actionbar_down_click_handler);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	dord_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PADLOCK);
	GRect bitmap_bounds = gbitmap_get_bounds(dord_icon_bitmap);

	dord_icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - (bitmap_bounds.size.w / 2) - (ACTION_BAR_WIDTH / 2), 15, bitmap_bounds.size.w, bitmap_bounds.size.h));
	bitmap_layer_set_bitmap(dord_icon_layer, dord_icon_bitmap);
	bitmap_layer_set_compositing_mode(dord_icon_layer, GCompOpSet);
	layer_add_child(window_layer, bitmap_layer_get_layer(dord_icon_layer));

	dord_label_layer = text_layer_create(GRect(10, 15 + bitmap_bounds.size.h + 5, 124 - ACTION_BAR_WIDTH, bounds.size.h - (10 + bitmap_bounds.size.h + 15)));
	text_layer_set_text(dord_label_layer, DIALOG_CHOICE_WINDOW_MESSAGE);
	text_layer_set_text_color(dord_label_layer, fg_color);
	text_layer_set_background_color(dord_label_layer, GColorClear);
	text_layer_set_text_alignment(dord_label_layer, GTextAlignmentCenter);
	text_layer_set_font(dord_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(dord_label_layer));

	dord_fav_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PNG_IMAGE_ICON_STAR);
	dord_del_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PNG_IMAGE_ICON_TRASH);

	dord_action_bar_layer = action_bar_layer_create();
	action_bar_layer_set_icon(dord_action_bar_layer, BUTTON_ID_UP, dord_fav_bitmap);
	action_bar_layer_set_icon(dord_action_bar_layer, BUTTON_ID_DOWN, dord_del_bitmap);
	action_bar_layer_set_click_config_provider(dord_action_bar_layer, dod_actionbar_config_provider);
	action_bar_layer_add_to_window(dord_action_bar_layer, window);
}

static void window_unload(Window *window) {
	text_layer_destroy(dord_label_layer);
	action_bar_layer_destroy(dord_action_bar_layer);
	bitmap_layer_destroy(dord_icon_layer);

	gbitmap_destroy(dord_icon_bitmap); 
	gbitmap_destroy(dord_fav_bitmap);
	gbitmap_destroy(dord_del_bitmap);

	window_destroy(window);
	dord_main_window = NULL;
}

void dod_window_push(int key_id) {
	if(!dord_main_window) {
		s_key_id = key_id;
		dord_main_window = window_create();

		window_set_background_color(dord_main_window, bg_color);

		window_set_window_handlers(dord_main_window, (WindowHandlers) {
			.load = window_load,
			.unload = window_unload,
		});
	}
	window_stack_push(dord_main_window, true);
}
