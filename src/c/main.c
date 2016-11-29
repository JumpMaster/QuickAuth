//
// Copyright 2015
// PebbAuth for the Pebble Smartwatch
// Author: Kevin Cooper
// https://github.com/JumpMaster/PebbleAuth
//

#include "pebble.h"
#include "main.h"
#include "single_code_window.h"
#include "multi_code_window.h"
#include "ctype.h"

// Colors
GColor bg_color;
GColor fg_color;
int bg_color_int;
int fg_color_int;

// Fonts
unsigned int font;
AppFont font_pin;
AppFont font_label;

bool fonts_changed;
bool colors_changed;
bool refresh_required;
bool loading_complete;

unsigned int js_message_retry_count = 0;
unsigned int js_message_max_retry_count = 5;

int timezone_offset = 0;

unsigned int phone_otp_count = 0;
unsigned int watch_otp_count = 0;
unsigned int idle_second_count = 0;
unsigned int idle_timeout = 300;
unsigned int otp_selected = 0;
unsigned int otp_default = 0;
unsigned int requesting_code = 0;
unsigned int otp_update_tick = 0;
unsigned int otp_updated_at_tick = 0;
unsigned int window_layout = 0;

char otp_labels[MAX_OTP][MAX_LABEL_LENGTH];
char otp_keys[MAX_OTP][MAX_KEY_LENGTH];

// Functions requiring early declaration
void request_key(int code_id);
void send_key(int requested_key);
void main_animate_second_counter(int seconds, bool off_screen);

void resetIdleTime() {
  idle_second_count = 0;
}

void refresh_screen(void) {
  if (loading_complete)
    refresh_required = true;
}

void update_screen_fonts(void) {
  fonts_changed = true;
  refresh_screen();
}

void notify_color_change(void) {
  colors_changed = true;
  refresh_screen();
}

void update_window_layout(void) {
  animation_unschedule_all();
  if (window_layout == 1) {
    multi_code_window_push();
    single_code_window_remove();
  } else {
    single_code_window_push();
    multi_code_window_remove();
  }
}

void switch_window_layout(void) {
  window_layout = window_layout == 0 ? 1 : 0;
  update_window_layout();
}

void write_key(char label[MAX_LABEL_LENGTH], char key[MAX_KEY_LENGTH], unsigned int location) {
  char combined_key[MAX_COMBINED_LENGTH];
  snprintf(combined_key, sizeof(combined_key), "%s:%s",label,key);
  persist_write_string(PS_SECRET+location, combined_key);
}

void move_key_position(unsigned int key_position, unsigned int new_position) {
  char label_buffer[MAX_LABEL_LENGTH];
  char key_buffer[MAX_KEY_LENGTH];

  strcpy(label_buffer, otp_labels[key_position]);
  strcpy(key_buffer, otp_keys[key_position]);

  if (key_position > new_position) {	
    for (unsigned int i = key_position; i > new_position; i--) {
      strcpy(otp_labels[i], otp_labels[i-1]);
      strcpy(otp_keys[i], otp_keys[i-1]);
      write_key(otp_labels[i], otp_keys[i], i);
    }
  } else if (new_position > key_position) {
    for (unsigned int i = key_position; i < new_position; i++) {
      strcpy(otp_labels[i], otp_labels[i+1]);
      strcpy(otp_keys[i], otp_keys[i+1]);
      write_key(otp_labels[i], otp_keys[i], i);
    }
  }

  strcpy(otp_labels[new_position], label_buffer);
  strcpy(otp_keys[new_position], key_buffer);
  write_key(otp_labels[new_position], otp_keys[new_position], new_position);

  if (otp_default == key_position)
    set_default_key(new_position, false);
  else if (otp_default <= new_position && otp_default >= key_position)
    set_default_key(otp_default-1, false);
    else if (otp_default >= new_position && otp_default <= key_position)
    set_default_key(otp_default+1, false);

    otp_selected = new_position;
    refresh_screen();
}

void on_animation_stopped(Animation *anim, bool finished, void *context) {
  //Free the memory used by the Animation
  property_animation_destroy((PropertyAnimation*) anim);
  anim = NULL;
}

void animate_layer(Layer *layer, AnimationCurve curve, GRect *start, GRect *finish, int duration, AnimationStoppedHandler callback) {
  //Declare animation
  PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);

  //Set characteristics
  animation_set_duration((Animation*) anim, duration);
  animation_set_curve((Animation*) anim, curve);


  if (callback) {
    AnimationHandlers handlers = {
      .stopped = (AnimationStoppedHandler) callback
    };
    animation_set_handlers((Animation*) anim, handlers, NULL);
  }

  #ifdef PBL_SDK_2
  if (!callback) {
    AnimationHandlers handlers = {
      .stopped = (AnimationStoppedHandler) on_animation_stopped
    };
    animation_set_handlers((Animation*) anim, handlers, NULL);
  }
  #endif

  //Start animation!	
  animation_schedule((Animation*) anim); 
}


void set_default_key(int key_id, bool force_refresh) {
  otp_default = key_id;
  persist_write_int(PS_DEFAULT_KEY, otp_default);

  if (otp_selected != otp_default) {
    otp_selected = otp_default;
    if (force_refresh)
      refresh_screen();
  }
}

void expand_key(char *inputString, bool new_code) {
  if (strstr(inputString, ":") == NULL) {
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: SUPER NULL input string, ignoring");
    return;
  }

  bool colonFound = false;
  int outputChar = 0;

  char otp_key[MAX_KEY_LENGTH];
  char otp_label[MAX_LABEL_LENGTH];

  for(unsigned int i = 0; i < strlen(inputString); i++) {
    if (inputString[i] == ':') {
      otp_label[outputChar] = '\0';
      colonFound = true;
      outputChar = 0;
    } else {
      if (colonFound) 
        otp_key[outputChar] = inputString[i]; 
      else
        otp_label[outputChar] = inputString[i];

      outputChar++;
    }
  }
  otp_key[outputChar] = '\0';

  // If the label or key are null ignore them
  if (strlen(otp_label) <= 0 || strlen(otp_key) <= 2) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: NULL key or label, ignoring");
    return;
  }

  bool updating_label = false;
  if (new_code) {
    for(unsigned int i = 0; i < watch_otp_count; i++) {
      if (strcmp(otp_key, otp_keys[i]) == 0) {
        updating_label = true;
        if (DEBUG) {
          APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Code exists. Relabeling");
          APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Saving to location: %d", PS_SECRET+i);
        }

        strcpy(otp_labels[i], otp_label);
        persist_write_string(PS_SECRET+i, inputString);
        if (otp_selected != i)
          otp_selected = i;

        refresh_screen();
      }
    }
  }

  if (!updating_label) {
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Adding Code");
    strcpy(otp_keys[watch_otp_count], otp_key);
    strcpy(otp_labels[watch_otp_count], otp_label);
    if (new_code) {
      if (DEBUG)
        APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Saving to location: %d", PS_SECRET+watch_otp_count);
      persist_write_string(PS_SECRET+watch_otp_count, inputString);
    }
    watch_otp_count++;
    otp_selected = watch_otp_count-1;
    refresh_screen();
  }
}

void check_load_status() {
  if ((phone_otp_count > 0 && phone_otp_count < requesting_code) || requesting_code > MAX_OTP) {
    requesting_code = 0;
    loading_complete = true;
    refresh_screen();
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: FINISHED REQUESTING");
  }
  else if (requesting_code > 0) {
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: REQUESTING ANOTHER!");
    request_key(requesting_code++);
  }
}

void sendJSMessage(Tuplet data_tuple) {
  DictionaryIterator *iter;
  int begin = app_message_outbox_begin(&iter);

  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Begin send JSMessage = %d", begin);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &data_tuple);
  dict_write_end(iter);

  int send = app_message_outbox_send();
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Send send JSMessage = %d", send);
}

void request_delete(int key_id) {
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Pebble Requesting delete: %s", otp_keys[key_id]);

  sendJSMessage(MyTupletCString(MESSAGE_KEY_delete_key, otp_keys[key_id]));
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered
  js_message_retry_count = 0;

  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Outgoing Message Delivered");
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Outgoing Message Failed");

  if (requesting_code > 0 && js_message_retry_count < js_message_max_retry_count) {
    js_message_retry_count++;

    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: RETRY:%d REQUESTING ANOTHER!", js_message_retry_count);

    request_key(requesting_code);
  }
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  // Check for fields you expect to receive
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Message Received");

  resetIdleTime();
  Tuple *key_count_tuple = dict_find(iter, MESSAGE_KEY_key_count);
  Tuple *key_tuple = dict_find(iter, MESSAGE_KEY_transmit_key);
  Tuple *timezone_tuple = dict_find(iter, MESSAGE_KEY_timezone);
  Tuple *key_delete_tuple = dict_find(iter, MESSAGE_KEY_delete_key);
  Tuple *font_tuple = dict_find(iter, MESSAGE_KEY_font);
  Tuple *idle_timeout_tuple = dict_find(iter, MESSAGE_KEY_idle_timeout);
  Tuple *key_request_tuple = dict_find(iter, MESSAGE_KEY_request_key);
  Tuple *window_layout_tuple = dict_find(iter, MESSAGE_KEY_window_layout);
  Tuple *foreground_color_tuple = dict_find(iter, MESSAGE_KEY_foreground_color);
  Tuple *background_color_tuple = dict_find(iter, MESSAGE_KEY_background_color);

  // Act on the found fields received
  if (key_count_tuple) {
    phone_otp_count = key_count_tuple->value->int16;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Key count from watch: %d", watch_otp_count);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Key count from phone: %d", phone_otp_count);

    if (watch_otp_count < phone_otp_count) {
      if (DEBUG)
        APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: REQUESTING CODES");
      loading_complete = false;
      requesting_code = 1;
      request_key(requesting_code++);
    }
  } // key_count_tuple

  if (key_tuple) {
    char key_value[MAX_COMBINED_LENGTH];
    memcpy(key_value, key_tuple->value->cstring, key_tuple->length);
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Text: %s", key_value);
    expand_key(key_value, true);
    check_load_status();
  } // key_tuple

  if (key_delete_tuple) {
    char key_value[MAX_COMBINED_LENGTH];
    memcpy(key_value, key_delete_tuple->value->cstring, key_delete_tuple->length);
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Deleting requested Key: %s", key_value);

    unsigned int key_found = MAX_OTP;
    for(unsigned int i = 0; i < watch_otp_count; i++) {
      if (strcmp(key_value, otp_keys[i]) == 0)
        key_found = i;
    }

    if(key_found < MAX_OTP) {
      for (unsigned int i = key_found; i < watch_otp_count; i++) {
        strcpy(otp_keys[i], otp_keys[i+1]);
        strcpy(otp_labels[i], otp_labels[i+1]);
        write_key(otp_labels[i], otp_keys[i], i);
      }
      watch_otp_count--;
      persist_delete(PS_SECRET+watch_otp_count);

      if (otp_selected >= key_found) {
        if (otp_selected == key_found)
          refresh_screen();
        otp_selected--;;
      }

      if (otp_default > 0 && otp_default >= key_found)
        otp_default--;
    }
  } // key_delete_tuple

  if (timezone_tuple) {
    int tz_offset = timezone_tuple->value->int16;
    if (tz_offset != timezone_offset) {
      timezone_offset = tz_offset;
      persist_write_int(PS_TIMEZONE_KEY, timezone_offset);
      #ifdef PBL_SDK_2
      refresh_screen();
      #endif
    }
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Timezone Offset: %d", timezone_offset);
  } // timezone_tuple

  int fg_color_value = fg_color_int;
  int bg_color_value = bg_color_int;
  
  if (foreground_color_tuple) {
    fg_color_value = foreground_color_tuple->value->int32;
  }

  if (background_color_tuple) {
    bg_color_value = background_color_tuple->value->int32;
  }

  if (fg_color_value != fg_color_int || bg_color_value != bg_color_int) {
    fg_color_int = fg_color_value;
    bg_color_int = bg_color_value;
    
    if (DEBUG) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: fg_color : %d", fg_color_int);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: bg_color : %d", bg_color_int);
    }
    persist_write_int(PS_FOREGROUND_COLOR, fg_color_int);
    persist_write_int(PS_BACKGROUND_COLOR, bg_color_int);
    notify_color_change();
  }

  if (font_tuple) {
    unsigned int font_value = font_tuple->value->int16;
    if (font != font_value) {
      font = font_value;
      persist_write_int(PS_FONT, font);
      update_screen_fonts();
    }
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Font : %d", font);
  } // font_tuple

  if (window_layout_tuple) {
    unsigned int window_layout_value = window_layout_tuple->value->int16;

    if (window_layout != window_layout_value) {
      window_layout = window_layout_value;
      persist_write_int(PS_WINDOW_LAYOUT, window_layout);
      update_window_layout();
    }
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Window Layout: %d", window_layout);
  } // window_layout_tuple

  if (idle_timeout_tuple) {
    if (idle_timeout_tuple->value->int16 >= 0) {
      unsigned int idle_timeout_value = idle_timeout_tuple->value->int16;
      if (idle_timeout != idle_timeout_value) {
        idle_timeout = idle_timeout_value;
        persist_write_int(PS_IDLE_TIMEOUT, idle_timeout);
      }
    }	
    else
      idle_timeout = 0;

    resetIdleTime();
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Idle Timeout: %d", idle_timeout);
  } // idle_timeout_tuple

  if (key_request_tuple) {
    int requested_key_value = key_request_tuple->value->int16;
    send_key(requested_key_value);
  } // key_request_tuple
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Incoming Message Dropped");
}

void request_key(int code_id) {
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Requesting code: %d", code_id);

  sendJSMessage(TupletInteger(MESSAGE_KEY_request_key, code_id));
}

void send_key(int requested_key) {
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Phone Requesting key: %d", requested_key);

  char keylabelpair[MAX_COMBINED_LENGTH];

  if (persist_exists(PS_SECRET+requested_key)) {
    if (DEBUG)
      APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: SENDING CODE FROM LOCATION %d", PS_SECRET+requested_key);

    persist_read_string(PS_SECRET+requested_key, keylabelpair, MAX_COMBINED_LENGTH);
  }
  else
    strcat(keylabelpair,"NULL");

  sendJSMessage(MyTupletCString(MESSAGE_KEY_transmit_key, keylabelpair));
}

void set_default_colors() {
  #ifdef PBL_COLOR
  fg_color_int = 16777215;
  bg_color_int = 255;
  #else
  fg_color_int = 16777215;
  bg_color_int = 0;
  #endif
}

void apply_new_colors() {
	if (fg_color_int < 0 || bg_color_int < 0)
		set_default_colors();
	fg_color = GColorFromHEX(fg_color_int);
	bg_color = GColorFromHEX(bg_color_int);
}

void load_persistent_data() {	
  timezone_offset = persist_exists(PS_TIMEZONE_KEY) ? persist_read_int(PS_TIMEZONE_KEY) : 0;

  fg_color_int = persist_exists(PS_FOREGROUND_COLOR) ? persist_read_int(PS_FOREGROUND_COLOR) : -1;
  bg_color_int = persist_exists(PS_BACKGROUND_COLOR) ? persist_read_int(PS_BACKGROUND_COLOR) : -1;

  if (fg_color_int < 0 || bg_color_int < 0)
    set_default_colors();
	
  apply_new_colors();

  otp_default = persist_exists(PS_DEFAULT_KEY) ? persist_read_int(PS_DEFAULT_KEY) : 0;
  font = persist_exists(PS_FONT) ? persist_read_int(PS_FONT) : 0;
  idle_timeout = persist_exists(PS_IDLE_TIMEOUT) ? persist_read_int(PS_IDLE_TIMEOUT) : 300;
  window_layout = persist_exists(PS_WINDOW_LAYOUT) ? persist_read_int(PS_WINDOW_LAYOUT) : 0;

  if (persist_exists(PS_SECRET)) {
    for(int i = 0; i < MAX_OTP; i++) {
      if (persist_exists(PS_SECRET+i)) {
        if (DEBUG)
          APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: LOADING CODE FROM LOCATION %d", PS_SECRET+i);

        char keylabelpair[MAX_COMBINED_LENGTH];
        persist_read_string(PS_SECRET+i, keylabelpair, MAX_COMBINED_LENGTH);

        if (DEBUG)
          APP_LOG(APP_LOG_LEVEL_DEBUG, "'%s'", keylabelpair);

        expand_key(keylabelpair, false);
      }
      else
        break;
    }
  } else
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: NO CODES ON WATCH!");

  if (otp_default >= watch_otp_count)
    otp_default = 0;

  otp_selected = otp_default;
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {

  if (idle_timeout > 0) {
    // If app is idle after X minutes then exit
    if (idle_second_count >= idle_timeout) {
      if (DEBUG)
        APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: Timer reached %d, exiting", idle_second_count);
      window_stack_pop_all(true);
      return;
    }
    else
      idle_second_count += 1;
  }

  if (window_layout == 1)
    multi_code_window_second_tick(tick_time->tm_sec);
  else
    single_code_window_second_tick(tick_time->tm_sec);
}

void handle_init(void) {
  load_persistent_data();
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);

	#if defined(PBL_PLATFORM_APLITE)
	int result = app_message_open(750, 750);
	#else
	int result = app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());    //Largest possible input and output buffer size
	#endif
	if (DEBUG)
		APP_LOG(APP_LOG_LEVEL_DEBUG, "APP_MESSAGE_OPEN: %d", result);


  if (window_layout == 1)
    multi_code_window_push();
  else
    single_code_window_push();

  loading_complete = true;
}

void handle_deinit(void) {
  if (DEBUG)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INFO: EXITING");

  tick_timer_service_unsubscribe();
  animation_unschedule_all();

  if (window_layout == 1)
    multi_code_window_remove();
  else
    single_code_window_remove();

  if (font_label.isCustom)
    fonts_unload_custom_font(font_label.font);
  if (font_pin.isCustom)
    fonts_unload_custom_font(font_pin.font);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}