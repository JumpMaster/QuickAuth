#if defined(PBL_ROUND)

#include <pebble.h>
#include "display.h"
#include "main.h"

#define TIME_ANGLE(time) time * (TRIG_MAX_ANGLE / 30000)

uint16_t thickness = 0;

void draw_countdown_graphic(Layer **layer, GContext **ctx, int *countdown_size, bool on_screen) {
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	int seconds = ((tick_time->tm_sec % 30)*1000) + time_ms(NULL, NULL);

	if (on_screen && thickness != 7)
		thickness++;
	else if (!on_screen && thickness > 0)
		thickness--;
	
	graphics_context_set_fill_color(*ctx, fg_color);
	graphics_fill_radial(*ctx, layer_get_bounds(*layer), GOvalScaleModeFitCircle, thickness, 0, TIME_ANGLE(seconds));
}

#endif