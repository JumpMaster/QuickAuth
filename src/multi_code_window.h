#pragma once
#include "pebble.h"
#define MULTI_CODE_CELL_HEIGHT 55

void multi_code_window_push(void);
void multi_code_window_remove(void);
void multi_code_window_second_tick(int seconds);