#pragma once
#include "pebble.h"

extern GRect single_text_label_rect;
extern GRect single_text_pin_rect;
extern int single_text_label_alignment;
extern int multi_code_cell_height;

void initialise_veriables();
void start_managing_countdown_layer(Layer * countdown_layer);
void stop_managing_countdown_layer();
void hide_countdown_layer();
void show_countdown_layer();
void set_countdown_layer_color(GColor color);