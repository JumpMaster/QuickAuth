#if defined(PBL_RECT)

#include <pebble.h>
#include "graphics.h"

const int timer_delta = 104;
GRect single_text_label_rect;
GRect single_text_pin_rect;
int multi_code_cell_height = 55;
int single_text_label_alignment = GTextAlignmentLeft;

static GRect countdown_rect;
static Layer *countdown_layer;
AppTimer *timer;
GColor layer_color;
bool running = false;
bool hidden = false;
GRect display_bounds;

static void countdown_layer_update_proc(Layer *layer, GContext *ctx);
void timer_callback(void *data);

void initialise_veriables() {
  single_text_label_rect = GRect(2, 50, 142, 40);
  single_text_pin_rect = GRect(0, 80, 144, 50);
}

void start_managing_countdown_layer(Layer * layer) {
  countdown_layer = layer;
  display_bounds = layer_get_frame(countdown_layer);
  layer_set_update_proc(countdown_layer, countdown_layer_update_proc);
  countdown_rect = (GRect(0, display_bounds.size.h, display_bounds.size.w, 10));
  
  if (timer) {
    app_timer_cancel(timer);
    timer = NULL;
  }
  
  running = true;
  timer = app_timer_register(timer_delta, (AppTimerCallback) timer_callback, NULL);
}

void stop_managing_countdown_layer() {
  running = false;
  app_timer_cancel(timer);
  timer = NULL;
}

void set_countdown_layer_color(GColor color) {
  layer_color = color;
}

static void countdown_layer_update_proc(Layer *layer, GContext *ctx) {
  uint16_t ms;
  time_t time;
  time_ms(&time, &ms);
  int reverse_seconds = 30-(time%30);
  
  graphics_context_set_fill_color(ctx, layer_color);
  
	#ifdef PBL_COLOR
		if (reverse_seconds <= 6) {
			if (reverse_seconds % 2 == 0)
				graphics_context_set_fill_color(ctx, GColorRed);
		}
	#endif
  
  if (hidden && countdown_rect.origin.y < display_bounds.size.h)
    countdown_rect.origin.y++;
  else if (!hidden && countdown_rect.origin.y > (display_bounds.size.h-10))
    countdown_rect.origin.y--;
    
  if (!hidden)
    countdown_rect.size.w = ((24 * reverse_seconds) / 5) - (ms / 208);
  
//  APP_LOG(APP_LOG_LEVEL_DEBUG, "S:%d MS:%d W:%d", reverse_seconds, ms, countdown_width);
  
  graphics_fill_rect(ctx, countdown_rect, 0, GCornerNone);
}

void timer_callback(void *data) {
  layer_mark_dirty(countdown_layer);
  
  if (running)
    timer = app_timer_register(timer_delta, (AppTimerCallback) timer_callback, NULL);
}

void hide_countdown_layer() {
  hidden = true;
}

void show_countdown_layer() {
  hidden = false;
}

#endif