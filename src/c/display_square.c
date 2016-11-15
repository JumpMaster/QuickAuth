#if defined(PBL_RECT) && PBL_DISPLAY_WIDTH == 144

#include <pebble.h>
#include "display.h"
#include "main.h"

void draw_countdown_graphic(Layer **layer, GContext **ctx, int *countdown_size) {
  GRect display_bounds = layer_get_frame(*layer);
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int seconds = ((30 - (tick_time->tm_sec % 30))*1000) - time_ms(NULL, NULL);
  int target_width = (((display_bounds.size.w*100) / 30) * seconds) / 100000;
  
  if (target_width >= *countdown_size)
    *countdown_size += ((target_width-*countdown_size)/6)+1;
  else
    *countdown_size -= ((*countdown_size-target_width)/6)+1;
  
  GRect countdown_rect = GRect(0, display_bounds.size.h-10, *countdown_size, 10);
  graphics_context_set_fill_color(*ctx, fg_color);
  graphics_fill_rect(*ctx, countdown_rect, 0, GCornerNone);
}

#endif