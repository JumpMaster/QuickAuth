#include <pebble.h>
void draw_countdown_graphic(Layer **layer, GContext **ctx, bool on_screen);
void create_single_code_screen_elements(Layer **layer, GRect *text_label_rect, GRect *text_pin_rect, TextLayer **text_label_layer);
void set_textlayer_positions(int font, GRect *text_label_rect, GRect *text_pin_rect);