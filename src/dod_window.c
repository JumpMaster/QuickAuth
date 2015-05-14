#include "pebble.h"
#include "main.h"
#include "select_window.h"
#include "dod_window.h"


static Window *dod_main_window;
static TextLayer *dod_label_layer;
static BitmapLayer *dod_icon_layer;
static ActionBarLayer *dod_action_bar_layer;

static GBitmap *dod_icon_bitmap, *dod_fav_bitmap, *dod_del_bitmap;

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

	dod_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PADLOCK);
	GRect bitmap_bounds = gbitmap_get_bounds(dod_icon_bitmap);
	dod_icon_layer = bitmap_layer_create(GRect((bounds.size.w / 2) - (bitmap_bounds.size.w / 2) - (ACTION_BAR_WIDTH / 2), 15, bitmap_bounds.size.w, bitmap_bounds.size.h));
	
	
	bitmap_layer_set_bitmap(dod_icon_layer, dod_icon_bitmap);
	#ifdef PBL_PLATFORM_APLITE
		bitmap_layer_set_compositing_mode(dod_icon_layer, GCompOpAssign);
	#elif PBL_PLATFORM_BASALT
		bitmap_layer_set_compositing_mode(dod_icon_layer, GCompOpSet);
	#endif
	
	layer_add_child(window_layer, bitmap_layer_get_layer(dod_icon_layer));

	dod_label_layer = text_layer_create(GRect(10, 15 + bitmap_bounds.size.h + 5, 124 - ACTION_BAR_WIDTH, bounds.size.h - (10 + bitmap_bounds.size.h + 15)));
	text_layer_set_text(dod_label_layer, DIALOG_CHOICE_WINDOW_MESSAGE);
	
	#ifdef PBL_COLOR
		text_layer_set_text_color(dod_label_layer, fg_color);
	#else
		text_layer_set_text_color(dod_label_layer, GColorBlack);
	#endif
	
	text_layer_set_background_color(dod_label_layer, GColorClear);
	text_layer_set_text_alignment(dod_label_layer, GTextAlignmentCenter);
	text_layer_set_font(dod_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(dod_label_layer));

	dod_fav_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PNG_IMAGE_ICON_STAR);
	dod_del_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PNG_IMAGE_ICON_TRASH);

	dod_action_bar_layer = action_bar_layer_create();
	action_bar_layer_set_icon(dod_action_bar_layer, BUTTON_ID_UP, dod_fav_bitmap);
	action_bar_layer_set_icon(dod_action_bar_layer, BUTTON_ID_DOWN, dod_del_bitmap);
	action_bar_layer_set_click_config_provider(dod_action_bar_layer, dod_actionbar_config_provider);
	action_bar_layer_add_to_window(dod_action_bar_layer, window);
}

static void window_unload(Window *window) {
	text_layer_destroy(dod_label_layer);
	action_bar_layer_destroy(dod_action_bar_layer);
	bitmap_layer_destroy(dod_icon_layer);

	gbitmap_destroy(dod_icon_bitmap); 
	gbitmap_destroy(dod_fav_bitmap);
	gbitmap_destroy(dod_del_bitmap);

	window_destroy(window);
	dod_main_window = NULL;
}

void dod_window_push(int key_id) {
	if(!dod_main_window) {
		s_key_id = key_id;
		dod_main_window = window_create();

		#ifdef PBL_SDK_2
			window_set_fullscreen(dod_main_window, true);
		#endif
		
		#ifdef PBL_COLOR
			window_set_background_color(dod_main_window, bg_color);
		#else
			window_set_background_color(dod_main_window, GColorWhite);
		#endif

		window_set_window_handlers(dod_main_window, (WindowHandlers) {
			.load = window_load,
			.unload = window_unload,
		});
	}
	window_stack_push(dod_main_window, true);
}