#if defined(PBL_ROUND)

#include <pebble.h>
#include "display.h"
#include "main.h"

#define TIME_ANGLE(time) time * (TRIG_MAX_ANGLE / 60)

void draw_countdown_graphic(Layer **layer, GContext **ctx, int *countdown_size) {

  GRect display_bounds = layer_get_frame(*layer);
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int seconds = ((30 - (tick_time->tm_sec % 30))*1000) - time_ms(NULL, NULL);

  uint32_t end_angle = TIME_ANGLE(seconds);
  uint16_t thickness = 5;

  if (end_angle == 0) {
    graphics_fill_radial(*ctx,
                         display_bounds,
                         GOvalScaleModeFitCircle,
                         thickness,
                         0,
                         TRIG_MAX_ANGLE);
  } else {
    graphics_fill_radial(*ctx,
                         display_bounds,
                         GOvalScaleModeFitCircle,
                         thickness,
                         0,
                         end_angle);
  }

}

#endif