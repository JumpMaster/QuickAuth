#if defined(PBL_ROUND)

#include <pebble.h>
#include "display.h"
#include "main.h"

#define TIME_ANGLE(time) time * (TRIG_MAX_ANGLE / 30000)

uint16_t thickness = 0;
uint16_t c_angle = 0;

void draw_countdown_graphic(Layer **layer, GContext **ctx, bool on_screen) {
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	int seconds = ((tick_time->tm_sec % 30)*1000) + time_ms(NULL, NULL);

	if (on_screen && thickness != 7)
		thickness++;
	else if (!on_screen && thickness > 0)
		thickness--;
	
	uint16_t target = (seconds*((TRIG_MAX_ANGLE*100)/30000))/100;
	
	if (target >= c_angle)
		c_angle += ((target-c_angle)/6)+1;
	else
		c_angle -= ((c_angle-target)/6)+1;
	
	graphics_context_set_fill_color(*ctx, fg_color);
	graphics_fill_radial(*ctx, layer_get_bounds(*layer), GOvalScaleModeFitCircle, thickness, 0, c_angle);
}

void create_single_code_screen_elements(Layer **layer, GRect *text_label_rect, GRect *text_pin_rect, TextLayer **text_label_layer) {
	GRect display_bounds = layer_get_frame(*layer);
	
	text_label_rect->origin.x = 0;
	text_label_rect->origin.y = 150;
	text_label_rect->size.w = display_bounds.size.w;
	text_label_rect->size.h = 40;

	text_pin_rect->origin.x = 0;
	text_pin_rect->origin.y = 180;
	text_pin_rect->size.w = display_bounds.size.w;
	text_pin_rect->size.h = 50;
	
	GRect text_label_start = *text_label_rect;
	text_label_start.origin.x = display_bounds.size.w;
	*text_label_layer = text_layer_create(text_label_start);
	text_layer_set_background_color(*text_label_layer, GColorClear);
	text_layer_set_overflow_mode(*text_label_layer, GTextOverflowModeWordWrap);	
	text_layer_set_text_alignment(*text_label_layer, GTextAlignmentCenter);
}

void set_textlayer_positions(int font, GRect *text_label_rect, GRect *text_pin_rect) {
	switch(font)
	{
		case 1 :
		text_label_rect->origin.y = 60;
		text_label_rect->size.h = 30;
		text_pin_rect->origin.y = 79;
		text_pin_rect->size.h = 50;
		break;
		case 2 :
		text_label_rect->origin.y = 57;
		text_label_rect->size.h = 40;
		text_pin_rect->origin.y = 79;
		text_pin_rect->size.h = 50;
		break;
		case 3 :
		text_label_rect->origin.y = 59;
		text_label_rect->size.h = 22;
		text_pin_rect->origin.y = 83;
		text_pin_rect->size.h = 50;
		break;
		default :
		text_label_rect->origin.y = 60;
		text_label_rect->size.h = 36;
		text_pin_rect->origin.y = 90;
		text_pin_rect->size.h = 85;
		break;
	}
}


#endif