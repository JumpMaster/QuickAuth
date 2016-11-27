#if defined(PBL_RECT) // && PBL_DISPLAY_WIDTH == 144

#include <pebble.h>
#include "display.h"
#include "main.h"

uint16_t thickness = 0;
uint16_t c_size = 0;

void draw_countdown_graphic(Layer **layer, GContext **ctx, bool on_screen) {
	GRect display_bounds = layer_get_frame(*layer);
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	int seconds = ((30 - (tick_time->tm_sec % 30))*1000) - time_ms(NULL, NULL);
	int target_width = (((display_bounds.size.w*100) / 30) * seconds) / 100000;

	if (target_width >= c_size)
		c_size += ((target_width-c_size)/6)+1;
	else
		c_size -= ((c_size-target_width)/6)+1;

	if (on_screen && thickness != 10)
		thickness++;
	else if (!on_screen && thickness > 0)
		thickness--;

	GRect countdown_rect = GRect(0, display_bounds.size.h-thickness, c_size, thickness);
	graphics_context_set_fill_color(*ctx, fg_color);
	graphics_fill_rect(*ctx, countdown_rect, 0, GCornerNone);
}

void create_single_code_screen_elements(Layer **layer, GRect *text_label_rect, GRect *text_pin_rect, TextLayer **text_label_layer) {
	GRect display_bounds = layer_get_frame(*layer);
	
	text_label_rect->origin.x = 2;
	text_label_rect->origin.y = 50;
	text_label_rect->size.w = display_bounds.size.w-2;
	text_label_rect->size.h = 40;

	text_pin_rect->origin.x = 0;
	text_pin_rect->origin.y = 80;
	text_pin_rect->size.w = display_bounds.size.w;
	text_pin_rect->size.h = 50;
	
	GRect text_label_start = *text_label_rect;
	text_label_start.origin.x = display_bounds.size.w;
	*text_label_layer = text_layer_create(text_label_start);
	text_layer_set_background_color(*text_label_layer, GColorClear);
	text_layer_set_overflow_mode(*text_label_layer, GTextOverflowModeWordWrap);	
	text_layer_set_text_alignment(*text_label_layer, GTextAlignmentLeft);
}

void set_textlayer_positions(int font, GRect *text_label_rect, GRect *text_pin_rect) {	
	switch(font)
	{
		case 1 :
		text_label_rect->origin.y = 55;
		text_label_rect->size.h = 30;
		text_pin_rect->origin.y = 76;
		text_pin_rect->size.h = 50;
		break;
		case 2 :
		text_label_rect->origin.y = 52;
		text_label_rect->size.h = 40;
		text_pin_rect->origin.y = 74;
		text_pin_rect->size.h = 50;
		break;
		case 3 :
		text_label_rect->origin.y = 54;
		text_label_rect->size.h = 22;
		text_pin_rect->origin.y = 78;
		text_pin_rect->size.h = 50;
		break;
		default :
		text_label_rect->origin.y = 50;
		text_label_rect->size.h = 36;
		text_pin_rect->origin.y = 80;
		text_pin_rect->size.h = 50;
		break;
	}
}


#endif