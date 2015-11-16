#if defined(PBL_ROUND)

#include <pebble.h>
#include "graphics.h"

#define TIME_ANGLE(time) time * (TRIG_MAX_ANGLE / 600)

const int timer_delta = 50;
GRect single_text_label_rect;
GRect single_text_pin_rect;
int multi_code_cell_height = 60;
int single_text_label_alignment = GTextAlignmentCenter;

static GRect countdown_rect;
static Layer *countdown_layer;
static int thickness = 0;
AppTimer *timer;
GColor layer_color;
bool running = false;
bool hidden = false;
GRect display_bounds;

static void countdown_layer_update_proc(Layer *layer, GContext *ctx);
void timer_callback(void *data);

void initialise_veriables() {
  single_text_label_rect = GRect(0, 56, 180, 40);
  single_text_pin_rect = GRect(0, 86, 180, 50);
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
  int seconds = time % 30;
  int units = (seconds*20) + (ms / 50);
  
  graphics_context_set_fill_color(ctx, layer_color);
  
  /*
	#ifdef PBL_COLOR
  */
		if (seconds >= 24) {
			if (seconds % 2 == 0)
				graphics_context_set_fill_color(ctx, GColorRed);
		}
  /*
	#endif
  */
  if (hidden && thickness > 0)
    thickness--;
  else if (!hidden && thickness < 7)
    thickness++;
    /*
  if (!hidden)
    countdown_rect.size.w = ((24 * reverse_seconds) / 5) - (ms / 208);
  */
//  APP_LOG(APP_LOG_LEVEL_DEBUG, "S:%d MS:%d W:%d", reverse_seconds, ms, countdown_width);
  
  graphics_fill_radial(ctx, layer_get_bounds(countdown_layer), GOvalScaleModeFitCircle, thickness, 0, TIME_ANGLE(units));
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