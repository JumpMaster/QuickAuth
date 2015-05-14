//
// Copyright 2015
// PebbleAuth for the Pebble Smartwatch
// Author: Kevin Cooper
// https://github.com/JumpMaster/PebbleAuth
//

#include "pebble.h"
#include "main.h"
#include "dod_window.h"

Window *select_window;
static MenuLayer *select_menu_layer;
int selected_index = 0;

void close_select_window() {
	window_stack_remove(select_window, false);
}

static uint16_t select_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
	return watch_otp_count;
}

static void select_menu_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
		menu_cell_basic_draw(ctx, cell_layer, otp_labels[cell_index->row], NULL, NULL);
}

static int16_t select_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	return SELECT_WINDOW_CELL_HEIGHT;
}

static void select_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	resetIdleTime();
	dod_window_push(cell_index->row);
}

static void select_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
	menu_cell_basic_header_draw(ctx, cell_layer, NULL);
}

static int16_t select_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static uint16_t select_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
	return 1;
}

static void select_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	select_menu_layer = menu_layer_create(bounds);
	
	#ifdef PBL_COLOR
		menu_layer_set_normal_colors(select_menu_layer, bg_color, fg_color);
	#endif
		
	menu_layer_set_click_config_onto_window(select_menu_layer, window);
	menu_layer_set_callbacks(select_menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)select_menu_get_num_rows_callback,
		.draw_row = (MenuLayerDrawRowCallback)select_menu_draw_row_callback,
		.get_cell_height = (MenuLayerGetCellHeightCallback)select_menu_get_cell_height_callback,
		.select_click = (MenuLayerSelectCallback)select_menu_select_callback,
		.draw_header = (MenuLayerDrawHeaderCallback)select_menu_draw_header_callback,
		.get_header_height = (MenuLayerGetHeaderHeightCallback)select_menu_get_header_height_callback,
		.get_num_sections = (MenuLayerGetNumberOfSectionsCallback)select_menu_get_num_sections_callback,
	});
	menu_layer_set_selected_index(select_menu_layer, MenuIndex(0, selected_index), MenuRowAlignCenter, false);
	layer_add_child(window_layer, menu_layer_get_layer(select_menu_layer));
}

void select_window_unload(Window *window) {
	menu_layer_destroy(select_menu_layer);
	window_destroy(select_window);
}

void push_select_window(int key_id) {
	selected_index = key_id;
	select_window = window_create();

	#ifdef PBL_SDK_2
		window_set_fullscreen(select_window, true);
	#endif

	window_set_window_handlers(select_window, (WindowHandlers) {
		.load = select_window_load,
		.unload = select_window_unload,
	});

	window_stack_push(select_window, true /* Animated */);	
}