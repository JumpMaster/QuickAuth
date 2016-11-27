#include <pebble.h>
#include "main.h"
#include "multi_code_window.h"
#include "google-authenticator.h"
#include "select_window.h"
#include "display.h"

static GRect display_bounds;
static MenuLayer *multi_code_menu_layer;
static Layer *multi_code_graphics_layer;
static Window *multi_code_main_window;
int menu_cell_height = 0;
int pin_origin_y = 0;
bool multi_code_exiting = false;
AppTimer *multi_code_graphics_timer;

void multi_code_refresh_callback(void *data) {
  if (!multi_code_exiting)
  	layer_mark_dirty(multi_code_graphics_layer);
}

static void update_graphics(Layer *layer, GContext *ctx) {
  draw_countdown_graphic(&layer, &ctx, true);
  if (!multi_code_exiting)
    multi_code_graphics_timer = app_timer_register(30, (AppTimerCallback) multi_code_refresh_callback, NULL);
}

static uint16_t multi_code_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
	return watch_otp_count > 1 ? watch_otp_count : 1;
}

static void multi_code_menu_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
	GRect bounds = layer_get_bounds(cell_layer);
	graphics_context_set_fill_color(ctx, bg_color);
	graphics_context_set_text_color(ctx, fg_color);
	
	#ifdef PBL_COLOR
	if (menu_cell_layer_is_highlighted(cell_layer)) {
		graphics_context_set_fill_color(ctx, fg_color);
		graphics_context_set_text_color(ctx, bg_color);
		graphics_fill_rect(ctx, bounds, 0, 0);
	}
	#endif

	if (watch_otp_count >= 1) {
		graphics_draw_text(ctx, generateCode(otp_keys[cell_index->row], timezone_offset), font_pin.font, GRect(0, pin_origin_y, bounds.size.w, 30), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		graphics_draw_text(ctx, otp_labels[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, 30, bounds.size.w, 25), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	} else {
		graphics_draw_text(ctx, "123456", font_pin.font, GRect(0, pin_origin_y, bounds.size.w, 30), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		graphics_draw_text(ctx, "EMPTY", fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, 30, bounds.size.w, 25), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	}
}

static int16_t multi_code_menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	return MULTI_CODE_CELL_HEIGHT;
}

static void multi_code_menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	resetIdleTime();
	otp_selected = cell_index->row;
	push_select_window(otp_selected);
}

static void multi_code_menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
	resetIdleTime();
	switch_window_layout();
}

static void multi_code_menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
	menu_cell_basic_header_draw(ctx, cell_layer, NULL);
}

static int16_t multi_code_menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
	return 0;
}

static uint16_t multi_code_menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
	return 1;
}

static void multi_code_menu_selection_changed_callback(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) {
	if (!refresh_required)
		otp_selected = new_index.row;
	resetIdleTime();
}

void multi_code_set_fonts(void) {
	fonts_changed = false;
	
	if (font_pin.isCustom)
		fonts_unload_custom_font(font_pin.font);
	
	switch(font)
	{
		case 1 :
			font_pin.font = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
			font_pin.isCustom = false;
			pin_origin_y = 0;
			break;
		case 2 :
			font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_42));
			font_pin.isCustom = true;
			pin_origin_y = -8;
			break;
		case 3 :
			font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BD_CARTOON_30));
			font_pin.isCustom = true;
			pin_origin_y = 2;
			break;
		default :
			font = 0;
			font_pin.font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BITWISE_32));
			font_pin.isCustom = true;
			pin_origin_y = 0;
			break;
	}
}


void multi_code_apply_display_colors() {
	apply_new_colors();
	window_set_background_color(multi_code_main_window, bg_color);
// 	set_countdown_layer_color(fg_color);
	#ifdef PBL_COLOR
		menu_layer_set_normal_colors(multi_code_menu_layer, bg_color, fg_color);
	#endif
}

void multi_code_window_second_tick(int seconds) {
	if (refresh_required) {
		menu_layer_reload_data(multi_code_menu_layer);
		menu_layer_set_selected_index(multi_code_menu_layer, MenuIndex(0, otp_selected), MenuRowAlignCenter, true);
		if(fonts_changed)
			multi_code_set_fonts();
		if (colors_changed)
			multi_code_apply_display_colors();
		refresh_required = false;
	}
}

static void multi_code_window_load(Window *window) {
  multi_code_exiting = false;
	Layer *window_layer = window_get_root_layer(window);
	display_bounds = layer_get_frame(window_layer);
	multi_code_set_fonts();
	GRect menu_bounds = layer_get_bounds(window_layer);
	menu_bounds.size.h = (display_bounds.size.h - 10) - 2;
	multi_code_menu_layer = menu_layer_create(menu_bounds);
	multi_code_graphics_layer = layer_create(display_bounds);
	layer_set_update_proc(multi_code_graphics_layer, update_graphics);
	menu_layer_set_click_config_onto_window(multi_code_menu_layer, window);
	menu_layer_set_callbacks(multi_code_menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)multi_code_menu_get_num_rows_callback,
		.draw_row = (MenuLayerDrawRowCallback)multi_code_menu_draw_row_callback,
		.get_cell_height = (MenuLayerGetCellHeightCallback)multi_code_menu_get_cell_height_callback,
		.select_click = (MenuLayerSelectCallback)multi_code_menu_select_callback,
		.select_long_click = (MenuLayerSelectCallback)multi_code_menu_select_long_callback,
		.draw_header = (MenuLayerDrawHeaderCallback)multi_code_menu_draw_header_callback,
		.get_header_height = (MenuLayerGetHeaderHeightCallback)multi_code_menu_get_header_height_callback,
		.get_num_sections = (MenuLayerGetNumberOfSectionsCallback)multi_code_menu_get_num_sections_callback,
		.selection_changed = (MenuLayerSelectionChangedCallback)multi_code_menu_selection_changed_callback,
	});
	scroll_layer_set_shadow_hidden(menu_layer_get_scroll_layer(multi_code_menu_layer), true);
	layer_add_child(window_layer, menu_layer_get_layer(multi_code_menu_layer));
	layer_add_child(window_layer, multi_code_graphics_layer);
	menu_layer_set_selected_index(multi_code_menu_layer, MenuIndex(0, otp_selected), MenuRowAlignCenter, false);
	multi_code_apply_display_colors();
}

void multi_code_window_unload(Window *window) {
  multi_code_exiting = true;
  app_timer_cancel(multi_code_graphics_timer);
	menu_layer_destroy(multi_code_menu_layer);
  layer_destroy(multi_code_graphics_layer);
	window_destroy(multi_code_main_window);
	multi_code_main_window = NULL;
}

void multi_code_window_remove(void) {
	window_stack_remove(multi_code_main_window, false);
}

void multi_code_window_push(void) {
	if (!multi_code_main_window) {
		multi_code_main_window = window_create();
		window_set_window_handlers(multi_code_main_window, (WindowHandlers) {
			.load = multi_code_window_load,
			.unload = multi_code_window_unload,
		});
	}
	window_stack_push(multi_code_main_window, true);
}