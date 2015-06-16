//
// Copyright 2015
// PebbAuth for the Pebble Smartwatch
// Author: Kevin Cooper
// https://github.com/JumpMaster/PebbleAuth
//

#include "pebble.h"
#include "select_window.h"
#include "main.h"
#include "dod_window.h"

Window *select_window;
static MenuLayer *select_menu_layer;
int selected_index = 0;
int moving_index = 0;
bool reorder_mode;

static GBitmap *shadow_top;
static GBitmap *shadow_bottom;

void close_select_window() {
	window_stack_remove(select_window, false);
}

void toggle_reorder_mode(int index) {
	reorder_mode = !reorder_mode;
	
	if (reorder_mode)
		moving_index = index;
	else
		move_key_position(moving_index, index);
	menu_layer_reload_data(select_menu_layer);
}

static uint16_t select_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
	return watch_otp_count;
}

static void select_menu_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
	GRect bounds = layer_get_bounds(cell_layer);
	
	#ifdef PBL_COLOR
	if (menu_cell_layer_is_highlighted(cell_layer)) {
		graphics_context_set_fill_color(ctx, fg_color);
		graphics_context_set_text_color(ctx, bg_color);
		graphics_fill_rect(ctx, bounds, 0, 0);
	}
	#endif
		
	if (reorder_mode) {
		// DRAW TEXT
		if (cell_index->row >= moving_index && cell_index->row < selected_index)
			menu_cell_basic_draw(ctx, cell_layer, otp_labels[(cell_index->row)+1], NULL, NULL);
		else if (cell_index->row <= moving_index && cell_index->row > selected_index)
			menu_cell_basic_draw(ctx, cell_layer, otp_labels[(cell_index->row)-1], NULL, NULL);
		else if (cell_index->row == selected_index)
			menu_cell_basic_draw(ctx, cell_layer, otp_labels[moving_index], NULL, NULL);
		else
			menu_cell_basic_draw(ctx, cell_layer, otp_labels[cell_index->row], NULL, NULL);

		#ifdef PBL_PLATFORM_APLITE
			graphics_context_set_compositing_mode(ctx, GCompOpClear);
		#elif PBL_PLATFORM_BASALT
			graphics_context_set_compositing_mode(ctx, GCompOpSet);
		#endif
		
		if (cell_index->row == (selected_index-1))
			graphics_draw_bitmap_in_rect(ctx, shadow_top, GRect(0, bounds.size.h-SHADOW_HEIGHT, 144, SHADOW_HEIGHT));
		else if (cell_index->row == (selected_index+1))
			graphics_draw_bitmap_in_rect(ctx, shadow_bottom, GRect(0, 0, 144, SHADOW_HEIGHT));
	}
	else
		menu_cell_basic_draw(ctx, cell_layer, otp_labels[cell_index->row], NULL, NULL);
}

static int16_t select_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	return SELECT_WINDOW_CELL_HEIGHT;
}

static void select_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	resetIdleTime();
	if (reorder_mode)
		toggle_reorder_mode(cell_index->row);
	else
		dod_window_push(cell_index->row);
}

static void select_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	toggle_reorder_mode(cell_index->row);
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

static void select_menu_selection_changed_callback(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) {
	resetIdleTime();
	selected_index = new_index.row;
}

static void select_window_load(Window *window) {
	reorder_mode = false;
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
		.select_long_click = (MenuLayerSelectCallback)select_menu_select_long_callback,
		.draw_header = (MenuLayerDrawHeaderCallback)select_menu_draw_header_callback,
		.get_header_height = (MenuLayerGetHeaderHeightCallback)select_menu_get_header_height_callback,
		.get_num_sections = (MenuLayerGetNumberOfSectionsCallback)select_menu_get_num_sections_callback,
		.selection_changed = (MenuLayerSelectionChangedCallback)select_menu_selection_changed_callback,
	});
	menu_layer_set_selected_index(select_menu_layer, MenuIndex(0, otp_selected), MenuRowAlignCenter, false);
	scroll_layer_set_shadow_hidden(menu_layer_get_scroll_layer(select_menu_layer), true);
	layer_add_child(window_layer, menu_layer_get_layer(select_menu_layer));
	
	#ifdef PBL_PLATFORM_APLITE
		shadow_top = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SIMPLE_FADE_TOP_BLACK);
		shadow_bottom = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SIMPLE_FADE_BOTTOM_BLACK);
	#elif PBL_PLATFORM_BASALT
		shadow_top = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SIMPLE_FADE_TOP);
		shadow_bottom = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SIMPLE_FADE_BOTTOM);
	#endif
}

void select_window_unload(Window *window) {
	menu_layer_destroy(select_menu_layer);
	gbitmap_destroy(shadow_top); 
	gbitmap_destroy(shadow_bottom); 
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