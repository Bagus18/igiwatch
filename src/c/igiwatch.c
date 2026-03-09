#include "igiwatch.h"
#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>

#ifndef INT_TO_FIXED
#define INT_TO_FIXED(a) ((int32_t)(a) << 16)
#endif

// Clock state
static Layer *s_canvas_layer;
static FFont *s_time_font;
static Layer *s_status_layer;
static int s_battery_level;
static bool s_bt_connected;
static GColor s_bg_color;
static GColor s_text_color;
static int s_font_choice;
static int s_orientation_choice;
static char s_date_locale[8];

static AppTimer *s_fluid_rotation_timer;
static AppTimer *s_double_tap_timer = NULL;
static int s_tap_count = 0;
static int32_t s_current_angle = TRIG_MAX_ANGLE / 2;
static bool s_is_moving = false;

// Time display strings
static char s_hour_str[3];
static char s_minute_str[3];
static char s_time_str[6];
static char s_dow_str[8];
static char s_day_str[4];
static char s_mon_str[8];
static char s_full_date_str[20];

static const char* DAYS_IT[] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};
static const char* MONTHS_IT[] = {"Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug", "Ago", "Set", "Ott", "Nov", "Dic"};

static const char* DAYS_EN[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* MONTHS_EN[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static const char* DAYS_FR[] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};
static const char* MONTHS_FR[] = {"Jan", "Fév", "Mar", "Avr", "Mai", "Jui", "Jul", "Aoû", "Sep", "Oct", "Nov", "Déc"};

static const char* DAYS_DE[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
static const char* MONTHS_DE[] = {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

static const char* DAYS_ES[] = {"Dom", "Lun", "Mar", "Mié", "Jue", "Vie", "Sáb"};
static const char* MONTHS_ES[] = {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};

static const char* DAYS_ID[] = {"Min", "Sen", "Sel", "Rab", "Kam", "Jum", "Sab"};
static const char* MONTHS_ID[] = {"Jan", "Feb", "Mar", "Apr", "Mei", "Jun", "Jul", "Agu", "Sep", "Okt", "Nop", "Des"};


static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  if (clock_is_24h_style()) {
    strftime(s_hour_str, sizeof(s_hour_str), "%H", tick_time);
  } else {
    strftime(s_hour_str, sizeof(s_hour_str), "%I", tick_time);
  }
  strftime(s_minute_str, sizeof(s_minute_str), "%M", tick_time);

  snprintf(s_time_str, sizeof(s_time_str), "%s:%s", s_hour_str, s_minute_str);

  strftime(s_dow_str, sizeof(s_dow_str), "%a", tick_time);
  snprintf(s_day_str, sizeof(s_day_str), "%d", tick_time->tm_mday);
  strftime(s_mon_str, sizeof(s_mon_str), "%m", tick_time);
  
  const char** days = DAYS_IT;
  const char** months = MONTHS_IT;

  if (strncmp(s_date_locale, "en", 2) == 0) {
    days = DAYS_EN;
    months = MONTHS_EN;
  } else if (strncmp(s_date_locale, "fr", 2) == 0) {
    days = DAYS_FR;
    months = MONTHS_FR;
  } else if (strncmp(s_date_locale, "de", 2) == 0) {
    days = DAYS_DE;
    months = MONTHS_DE;
  } else if (strncmp(s_date_locale, "es", 2) == 0) {
    days = DAYS_ES;
    months = MONTHS_ES;
  } else if (strncmp(s_date_locale, "id", 2) == 0) {
    days = DAYS_ID;
    months = MONTHS_ID;
  }


  snprintf(s_full_date_str, sizeof(s_full_date_str), "%s %d %s", days[tick_time->tm_wday], tick_time->tm_mday, months[tick_time->tm_mon]);

  layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void fluid_rotation_timer_callback(void *data) {
  s_fluid_rotation_timer = NULL;

  // Stop listening to accelerometer data
  accel_data_service_unsubscribe();

  s_is_moving = false;
  layer_mark_dirty(s_canvas_layer);
  layer_mark_dirty(s_status_layer);

}

static void double_tap_timer_callback(void *data) {
  s_double_tap_timer = NULL;
  s_tap_count = 0;
}

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  // We are only interested in the latest sample
  AccelData *latest = &data[num_samples - 1];

  // Check for a significant tilt to the left or right
  if (latest->x > 700) { // Tilted right
    s_current_angle = TRIG_MAX_ANGLE / 4; // 90 degrees
  } else if (latest->x < -700) { // Tilted left
    s_current_angle = TRIG_MAX_ANGLE * 3 / 4; // 270 degrees
  } else {
    // Not tilted enough, or tilted forward/backward, stay at bottom
    s_current_angle = TRIG_MAX_ANGLE / 2;
  }

  // Redraw the layer
  layer_mark_dirty(s_canvas_layer);
  layer_mark_dirty(s_status_layer);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_orientation_choice != 0) {
    return; // Fixed orientation, ignore taps
  }

  s_tap_count++;
  if (s_tap_count == 1) {
    s_double_tap_timer = app_timer_register(1000, double_tap_timer_callback, NULL);
    return;
  }

  if (s_double_tap_timer) {
    app_timer_cancel(s_double_tap_timer);
    s_double_tap_timer = NULL;
  }
  s_tap_count = 0;

  s_is_moving = true;
  layer_mark_dirty(s_canvas_layer);
  layer_mark_dirty(s_status_layer);

  if (s_fluid_rotation_timer) {
    app_timer_reschedule(s_fluid_rotation_timer, 5000); // 5 seconds
    return;
  }

  accel_data_service_subscribe(1, accel_data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  s_fluid_rotation_timer = app_timer_register(5000, fluid_rotation_timer_callback, NULL);
}

// Draw the clock on screen
static void layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Draw a background rectangle
  graphics_context_set_fill_color(ctx, s_bg_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GPoint center = grect_center_point(&bounds);

  graphics_context_set_fill_color(ctx, s_text_color);
  graphics_context_set_stroke_color(ctx, s_text_color);

  // Draw a small ball attached to the big one
  // The big circle has a radius of 40, the small one 10.
  // A y-offset of -50 makes them touch.
  if (s_is_moving) {
    int32_t sin_a = sin_lookup(s_current_angle);
    int32_t cos_a = cos_lookup(s_current_angle);
    int radius = 50;
    int small_ball_x = center.x + (radius * sin_a) / TRIG_MAX_RATIO;
    int small_ball_y = center.y - (radius * cos_a) / TRIG_MAX_RATIO;

    graphics_fill_circle(ctx, GPoint(small_ball_x, small_ball_y), 10);
  }

  if (!s_is_moving) {
    int32_t text_rotation = 0;
    FPoint text_pos;
    int font_size;

    if (s_current_angle == TRIG_MAX_ANGLE / 4) { // 90 deg
        text_rotation = TRIG_MAX_ANGLE * 3 / 4; // 270 deg
        text_pos.x = INT_TO_FIXED(0);
        text_pos.y = INT_TO_FIXED(bounds.size.h + 2);
        font_size = 96;
    } else if (s_current_angle == TRIG_MAX_ANGLE * 3 / 4) { // 270 deg
        text_rotation = TRIG_MAX_ANGLE / 4; // 90 deg
        text_pos.x = INT_TO_FIXED(bounds.size.w);
        text_pos.y = INT_TO_FIXED(2);
        font_size = 96;
    } else {
        text_rotation = 0;
        text_pos.x = INT_TO_FIXED(0);
        text_pos.y = INT_TO_FIXED(0);
        font_size = 96;
    }

    if (!s_time_font) {
        uint32_t font_res_id;
        switch (s_font_choice) {
          case 1:
            font_res_id = RESOURCE_ID_LECO1976_REGULAR;
            break;
          case 2:
            font_res_id = RESOURCE_ID_AVENIR_NEXT_REGULAR;
            break;
          default: // case 0
            font_res_id = RESOURCE_ID_AVENIR_NEXT_DEMI_BOLD;
            break;
        }
        s_time_font = ffont_create_from_resource(font_res_id);
    }

    FContext fctx;
    fctx_init_context(&fctx, ctx);
    fctx_set_fill_color(&fctx, s_text_color);

    fctx_set_rotation(&fctx, text_rotation);
    fctx_set_offset(&fctx, text_pos);

    fctx_set_text_em_height(&fctx, s_time_font, font_size);

    fctx_begin_fill(&fctx);
    fctx_draw_string(&fctx, s_hour_str, s_time_font, GTextAlignmentLeft, FTextAnchorTop);
    fctx_end_fill(&fctx);

    // Reduce the line height to bring minutes closer to the hour
    int line_height = font_size * 3 / 4;
    if (s_current_angle != TRIG_MAX_ANGLE / 2) {
      line_height += 2;
    }

    int32_t dy = (cos_lookup(text_rotation) * line_height) / TRIG_MAX_RATIO;
    int32_t dx = (-sin_lookup(text_rotation) * line_height) / TRIG_MAX_RATIO;
    text_pos.x += INT_TO_FIXED(dx);
    text_pos.y += INT_TO_FIXED(dy);
    fctx_set_offset(&fctx, text_pos);

    fctx_begin_fill(&fctx);
    fctx_draw_string(&fctx, s_minute_str, s_time_font, GTextAlignmentLeft, FTextAnchorTop);
    fctx_end_fill(&fctx);

    // --- Draw Date using the chosen font ---
    int date_font_size;
    if (s_current_angle == TRIG_MAX_ANGLE / 2) { // Portrait
        // Use system font for portrait
        graphics_context_set_text_color(ctx, s_text_color);
        GFont sys_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
        GRect date_bounds = GRect(0, bounds.size.h - 30, bounds.size.w - 5, 30);
        graphics_draw_text(ctx, s_full_date_str, sys_font, date_bounds, GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    } else { // Landscape
        date_font_size = 32;
        FPoint date_pos;
        if (s_current_angle == TRIG_MAX_ANGLE / 4) { // Right Landscape
            // Visual bottom-right is physical top-right
            date_pos.x = INT_TO_FIXED(bounds.size.w - 40);
            date_pos.y = INT_TO_FIXED(40);
        } else { // Left Landscape
            // Visual bottom-right is physical bottom-left
            date_pos.x = INT_TO_FIXED(40);
            date_pos.y = INT_TO_FIXED(bounds.size.h - 40);
        }
        fctx_set_text_em_height(&fctx, s_time_font, date_font_size);

        // Calculate offset for stacking day and month
        int date_line_height = date_font_size;
        int32_t date_dy = (cos_lookup(text_rotation) * date_line_height) / TRIG_MAX_RATIO;
        int32_t date_dx = (-sin_lookup(text_rotation) * date_line_height) / TRIG_MAX_RATIO;

        FPoint line_pos = date_pos;
        line_pos.x -= INT_TO_FIXED(date_dx) / 2;
        line_pos.y -= INT_TO_FIXED(date_dy) / 2;

        fctx_set_offset(&fctx, line_pos);
        fctx_begin_fill(&fctx);
        fctx_draw_string(&fctx, s_day_str, s_time_font, GTextAlignmentCenter, FTextAnchorMiddle);
        fctx_end_fill(&fctx);

        line_pos.x += INT_TO_FIXED(date_dx);
        line_pos.y += INT_TO_FIXED(date_dy);

        fctx_set_offset(&fctx, line_pos);
        fctx_begin_fill(&fctx);
        fctx_draw_string(&fctx, s_mon_str, s_time_font, GTextAlignmentCenter, FTextAnchorMiddle);
        fctx_end_fill(&fctx);
    }

    fctx_deinit_context(&fctx);
  }
}

static void status_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Don't draw status icons while the orientation is being changed
  if (s_is_moving) {
    return;
  }

  // --- Battery Bar ---
  int bar_len_h = (int)(((float)s_battery_level / 100.0F) * bounds.size.w);
  int bar_len_v = (int)(((float)s_battery_level / 100.0F) * bounds.size.h);

  GColor bar_color;
  if (s_battery_level > 50) {
    bar_color = GColorJaegerGreen;
  } else if (s_battery_level > 20) {
    bar_color = GColorChromeYellow;
  } else {
    bar_color = GColorRed;
  }

  graphics_context_set_fill_color(ctx, bar_color);

  GRect battery_bar_rect;
  GRect battery_border_rect;

  // --- Bluetooth Icon & Bar Positioning ---
  graphics_context_set_text_color(ctx, s_text_color); // Set text color for BT icon

  if (s_current_angle == TRIG_MAX_ANGLE * 3 / 4) { // Left landscape
    // Visual Right is Physical Bottom.
    // Bar at Physical Bottom.
    battery_bar_rect = GRect(0, bounds.size.h - 6, bar_len_h, 6);
    battery_border_rect = GRect(0, bounds.size.h - 6, bounds.size.w, 6);
    if (!s_bt_connected) {
      graphics_draw_text(ctx, "BT", fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0, bounds.size.h - 20, 20, 14), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  } else if (s_current_angle == TRIG_MAX_ANGLE / 4) { // Right landscape
    // Visual Right is Physical Top.
    // Bar at Physical Top.
    battery_bar_rect = GRect(bounds.size.w - bar_len_h, 0, bar_len_h, 6);
    battery_border_rect = GRect(0, 0, bounds.size.w, 6);
    if (!s_bt_connected) {
      graphics_draw_text(ctx, "BT", fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(bounds.size.w - 20, 6, 20, 14), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    }
  } else { // Portrait
    // Visual Right is Physical Right.
    battery_bar_rect = GRect(bounds.size.w - 6, bounds.size.h - bar_len_v, 6, bar_len_v);
    battery_border_rect = GRect(bounds.size.w - 6, 0, 6, bounds.size.h);
    if (!s_bt_connected) {
      graphics_draw_text(ctx, "BT", fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(bounds.size.w - 25, 0, 20, 14), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    }
  }

  // Draw the battery bar
  graphics_context_set_fill_color(ctx, bar_color);
  graphics_fill_rect(ctx, battery_bar_rect, 0, GCornerNone);
  // Add a black border for better visibility
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, battery_border_rect);
}

static void battery_callback(BatteryChargeState charge) {
  s_battery_level = charge.charge_percent;
  layer_mark_dirty(s_status_layer);
}

static void bluetooth_callback(bool connected) {
  s_bt_connected = connected;
  layer_mark_dirty(s_status_layer);

  if(!connected) {
    // Vibrate for disconnection
    vibes_double_pulse();
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) {
    s_bg_color = GColorFromHEX(bg_color_t->value->int32);
    persist_write_int(MESSAGE_KEY_BackgroundColor, bg_color_t->value->int32);
  }

  Tuple *text_color_t = dict_find(iter, MESSAGE_KEY_TextColor);
  if (text_color_t) {
    s_text_color = GColorFromHEX(text_color_t->value->int32);
    persist_write_int(MESSAGE_KEY_TextColor, text_color_t->value->int32);
  }

  Tuple *font_choice_t = dict_find(iter, MESSAGE_KEY_FontChoice);
  if (font_choice_t) {
    s_font_choice = atoi(font_choice_t->value->cstring);
    persist_write_int(MESSAGE_KEY_FontChoice, s_font_choice);

    // Unload old font, new one will be loaded in update_proc
    if (s_time_font) {
      ffont_destroy(s_time_font);
      s_time_font = NULL;
    }
  }

  Tuple *orientation_t = dict_find(iter, MESSAGE_KEY_ScreenOrientation);
  if (orientation_t) {
    s_orientation_choice = atoi(orientation_t->value->cstring);
    persist_write_int(MESSAGE_KEY_ScreenOrientation, s_orientation_choice);

    // Apply the new orientation immediately, disable tap handler if fixed
    if (s_orientation_choice == 1) { // Portrait
      s_current_angle = TRIG_MAX_ANGLE / 2;
    } else if (s_orientation_choice == 2) { // Landscape Left
      s_current_angle = TRIG_MAX_ANGLE * 3 / 4;
    } else if (s_orientation_choice == 3) { // Landscape Right
      s_current_angle = TRIG_MAX_ANGLE / 4;
    }
    // If automatic (0), we don't change the current angle until a tap

    layer_mark_dirty(s_status_layer);
  }

  Tuple *language_t = dict_find(iter, MESSAGE_KEY_DateLanguage);
  if (language_t) {
    strncpy(s_date_locale, language_t->value->cstring, sizeof(s_date_locale));
    persist_write_string(MESSAGE_KEY_DateLanguage, s_date_locale);
    update_time();
  }

  layer_mark_dirty(s_canvas_layer);
}

// Initialize clock
void Clock_init(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the clock display layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, layer_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Create status layer
  s_status_layer = layer_create(bounds);
  layer_set_update_proc(s_status_layer, status_update_proc);
  layer_add_child(window_layer, s_status_layer);

  // Subscribe to tick timer service
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(tap_handler);

  // Subscribe to services
  battery_state_service_subscribe(battery_callback);
  bluetooth_connection_service_subscribe(bluetooth_callback);

  // Get initial state
  battery_callback(battery_state_service_peek());
  bluetooth_callback(connection_service_peek_pebble_app_connection());

  update_time();
}

// Deinitialize clock
void Clock_deinit(void) {
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();

  if (s_fluid_rotation_timer) {
    app_timer_cancel(s_fluid_rotation_timer);
    s_fluid_rotation_timer = NULL;
    accel_data_service_unsubscribe();
  }

  if (s_double_tap_timer) {
    app_timer_cancel(s_double_tap_timer);
    s_double_tap_timer = NULL;
  }

  if (s_time_font) {
    ffont_destroy(s_time_font);
    s_time_font = NULL;
  }

  if (s_canvas_layer) {
    layer_destroy(s_canvas_layer);
  }

  if (s_status_layer) {
    layer_destroy(s_status_layer);
    s_status_layer = NULL;
  }
}

static Window *s_main_window;

static void main_window_load(Window *window) { Clock_init(window); }

static void main_window_unload(Window *window) { Clock_deinit(); }

static void init(void) {
  // Load colors or set defaults
  if (persist_exists(MESSAGE_KEY_BackgroundColor)) {
    s_bg_color = GColorFromHEX(persist_read_int(MESSAGE_KEY_BackgroundColor));
  } else {
    s_bg_color = GColorBlack;
  }

  if (persist_exists(MESSAGE_KEY_TextColor)) {
    s_text_color = GColorFromHEX(persist_read_int(MESSAGE_KEY_TextColor));
  } else {
    s_text_color = GColorWhite;
  }

  // Load font choice or set default
  if (persist_exists(MESSAGE_KEY_FontChoice)) {
    s_font_choice = persist_read_int(MESSAGE_KEY_FontChoice);
  } else {
    s_font_choice = 0; // 0: Avenir, 1: Leco
  }

  // Load orientation choice or set default
  if (persist_exists(MESSAGE_KEY_ScreenOrientation)) {
    s_orientation_choice = persist_read_int(MESSAGE_KEY_ScreenOrientation);
  } else {
    s_orientation_choice = 0; // 0: Automatic
  }

  // Load date language or set default
  if (persist_exists(MESSAGE_KEY_DateLanguage)) {
    persist_read_string(MESSAGE_KEY_DateLanguage, s_date_locale, sizeof(s_date_locale));
  } else {
    strcpy(s_date_locale, "it_IT");
  }

  if (s_orientation_choice == 1) { // Portrait
    s_current_angle = TRIG_MAX_ANGLE / 2;
  } else if (s_orientation_choice == 2) { // Landscape Left
    s_current_angle = TRIG_MAX_ANGLE * 3 / 4;
  } else if (s_orientation_choice == 3) { // Landscape Right
    s_current_angle = TRIG_MAX_ANGLE / 4;
  }

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 128);

  s_main_window = window_create();
  window_set_window_handlers(
      s_main_window,
      (WindowHandlers){.load = main_window_load, .unload = main_window_unload});
  window_set_background_color(s_main_window, GColorBlack);
  window_stack_push(s_main_window, true);
}

static void deinit(void) { window_destroy(s_main_window); }

int main(void) {
  init();
  app_event_loop();
  deinit();
}
