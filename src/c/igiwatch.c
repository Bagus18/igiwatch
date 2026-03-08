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
static GColor s_bg_color;
static GColor s_text_color;

static AppTimer *s_fluid_rotation_timer;
static AppTimer *s_double_tap_timer = NULL;
static int s_tap_count = 0;
static int32_t s_current_angle = TRIG_MAX_ANGLE / 2;
static bool s_is_moving = false;

// Time display strings
static char s_hour_str[3];
static char s_minute_str[3];
static char s_time_str[6];

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
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
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
        text_pos.y = INT_TO_FIXED(bounds.size.h);
        font_size = 96;
    } else if (s_current_angle == TRIG_MAX_ANGLE * 3 / 4) { // 270 deg
        text_rotation = TRIG_MAX_ANGLE / 4; // 90 deg
        text_pos.x = INT_TO_FIXED(bounds.size.w);
        text_pos.y = INT_TO_FIXED(0);
        font_size = 96;
    } else {
        text_rotation = 0;
        text_pos.x = INT_TO_FIXED(0);
        text_pos.y = INT_TO_FIXED(0);
        font_size = 114;
    }

    if (!s_time_font) {
        s_time_font = ffont_create_from_resource(RESOURCE_ID_AVENIR_NEXT_DEMI_BOLD);
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
    int32_t dy = (cos_lookup(text_rotation) * line_height) / TRIG_MAX_RATIO;
    int32_t dx = (-sin_lookup(text_rotation) * line_height) / TRIG_MAX_RATIO;
    text_pos.x += INT_TO_FIXED(dx);
    text_pos.y += INT_TO_FIXED(dy);
    fctx_set_offset(&fctx, text_pos);

    fctx_begin_fill(&fctx);
    fctx_draw_string(&fctx, s_minute_str, s_time_font, GTextAlignmentLeft, FTextAnchorTop);
    fctx_end_fill(&fctx);

    fctx_deinit_context(&fctx);
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

  // Subscribe to tick timer service
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(tap_handler);
  update_time();
}

// Deinitialize clock
void Clock_deinit(void) {
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
