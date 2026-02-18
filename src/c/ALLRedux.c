#include <pebble.h>

// Bump key when ClaySettings struct layout changes so old persisted data is ignored.
#define SETTINGS_KEY 2

// ─── Quadrant slot indices ──────────────────────────────────────────────────

#define SLOT_TL 0
#define SLOT_TR 1
#define SLOT_BL 2
#define SLOT_BR 3

// ─── What a quadrant can display ───────────────────────────────────────────

typedef enum {
  QUAD_NONE        = 0,
  QUAD_BATTERY     = 1,
  QUAD_TEMP        = 2,
  QUAD_DAY         = 3,
  QUAD_DATE        = 4,
  QUAD_CONDITIONS  = 5,
  QUAD_STEPS       = 6,
  QUAD_DISTANCE    = 7,
  QUAD_ACTIVE_MINS = 8,
  QUAD_HEART_RATE  = 9,
  QUAD_SECONDS     = 10,
} QuadrantContent;

// ─── Persistent settings ────────────────────────────────────────────────────

typedef struct ClaySettings {
  bool    use_celsius;
  bool    use_metric;
  char    apikey[64];
  uint8_t quad[4];   // QuadrantContent per slot: TL, TR, BL, BR
} ClaySettings;

static ClaySettings settings;

// ─── UI ─────────────────────────────────────────────────────────────────────

static Window    *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_quad_layer[4];
static char       s_quad_buf[4][32];

// ─── Data store ─────────────────────────────────────────────────────────────

static int  s_batt_pct      = 0;
static int  s_temp_c        = 0;
static int  s_temp_f        = 0;
static char s_conditions[32] = "--";
static char s_day_str[16]    = "";
static char s_date_str[16]   = "";
static char s_seconds_str[4] = ":00";

#if defined(PBL_HEALTH)
static int  s_steps       = 0;
static int  s_distance_m  = 0;
static int  s_active_min  = 0;
static int  s_heart_rate  = 0;
#endif

// ─── Settings ───────────────────────────────────────────────────────────────

static void load_settings(void) {
  if (persist_exists(SETTINGS_KEY)) {
    persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
  } else {
    settings.use_celsius    = false;
    settings.use_metric     = false;
    settings.apikey[0]      = '\0';
    settings.quad[SLOT_TL]  = QUAD_BATTERY;
    settings.quad[SLOT_TR]  = QUAD_TEMP;
    settings.quad[SLOT_BL]  = QUAD_DAY;
    settings.quad[SLOT_BR]  = QUAD_DATE;
  }
}

static void save_settings(void) {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// ─── Quadrant rendering ─────────────────────────────────────────────────────

static void render_quadrant(int idx) {
  char *buf = s_quad_buf[idx];

  switch ((QuadrantContent)settings.quad[idx]) {
    case QUAD_BATTERY:
      snprintf(buf, 32, "%d%%", s_batt_pct);
      break;

    case QUAD_TEMP:
      if (settings.use_celsius) snprintf(buf, 32, "%d°C", s_temp_c);
      else                       snprintf(buf, 32, "%d°F", s_temp_f);
      break;

    case QUAD_DAY:
      snprintf(buf, 32, "%s", s_day_str);
      break;

    case QUAD_DATE:
      snprintf(buf, 32, "%s", s_date_str);
      break;

    case QUAD_CONDITIONS:
      snprintf(buf, 32, "%s", s_conditions);
      break;

#if defined(PBL_HEALTH)
    case QUAD_STEPS:
      snprintf(buf, 32, "%d", s_steps);
      break;

    case QUAD_DISTANCE: {
      // Integer arithmetic to avoid floating point on watch
      if (settings.use_metric) {
        int km_int = s_distance_m / 1000;
        int km_dec = (s_distance_m % 1000) / 100;
        snprintf(buf, 32, "%d.%dkm", km_int, km_dec);
      } else {
        // 1609 meters per mile; multiply first to avoid truncation
        int mi_x10 = (int)((long)s_distance_m * 10 / 1609);
        snprintf(buf, 32, "%d.%dmi", mi_x10 / 10, mi_x10 % 10);
      }
      break;
    }

    case QUAD_ACTIVE_MINS:
      snprintf(buf, 32, "%dmin", s_active_min);
      break;

    case QUAD_HEART_RATE:
      if (s_heart_rate > 0) snprintf(buf, 32, "%dbpm", s_heart_rate);
      else                   snprintf(buf, 32, "--bpm");
      break;
#endif

    case QUAD_SECONDS:
      snprintf(buf, 32, "%s", s_seconds_str);
      break;

    case QUAD_NONE:
    default:
      buf[0] = '\0';
      break;
  }

  text_layer_set_text(s_quad_layer[idx], buf);
}

static void update_all_quadrants(void) {
  for (int i = 0; i < 4; i++) render_quadrant(i);
}

// ─── Tick subscription ──────────────────────────────────────────────────────

// Forward declaration — defined below after update_time_display.
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

static bool any_quad_shows_seconds(void) {
  for (int i = 0; i < 4; i++) {
    if ((QuadrantContent)settings.quad[i] == QUAD_SECONDS) return true;
  }
  return false;
}

// Call this at init and whenever quadrant settings change to keep the
// subscription granularity in sync with what the face actually needs.
static void resubscribe_tick(void) {
  tick_timer_service_unsubscribe();
  TimeUnits unit = any_quad_shows_seconds() ? SECOND_UNIT : MINUTE_UNIT;
  tick_timer_service_subscribe(unit, tick_handler);
}

// ─── Time ───────────────────────────────────────────────────────────────────

static void update_time_display(struct tm *tick_time) {
  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  // Strip leading zero in 12-hour mode ("08:30" → "8:30")
  if (!clock_is_24h_style() && s_time_buffer[0] == '0') {
    memmove(s_time_buffer, s_time_buffer + 1, strlen(s_time_buffer));
  }
  text_layer_set_text(s_time_layer, s_time_buffer);

  strftime(s_day_str,  sizeof(s_day_str),  "%A",    tick_time);
  strftime(s_date_str, sizeof(s_date_str), "%b %e", tick_time);
}

// Used for initial population at window load — reads the current wall clock.
static void init_time_display(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  update_time_display(t);
  snprintf(s_seconds_str, sizeof(s_seconds_str), ":%02d", t->tm_sec);
}

// ─── Tick handler ───────────────────────────────────────────────────────────

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    update_time_display(tick_time);
  }

  // Always refresh the seconds string; it's only displayed when a quadrant
  // is configured to show it, but keeping it current is cheap.
  snprintf(s_seconds_str, sizeof(s_seconds_str), ":%02d", tick_time->tm_sec);

  update_all_quadrants();

  // Trigger a weather refresh every 30 minutes.
  // Guard with tm_sec == 0 so this fires exactly once even when subscribed
  // to SECOND_UNIT (without the guard it would fire 60 times per interval).
  if (tick_time->tm_min % 30 == 0 && tick_time->tm_sec == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, 0, 0);
    app_message_outbox_send();
  }
}

// ─── Battery ────────────────────────────────────────────────────────────────

static void update_battery(BatteryChargeState state) {
  s_batt_pct = state.charge_percent;
  update_all_quadrants();
}

// ─── Health ─────────────────────────────────────────────────────────────────

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  time_t start = time_start_of_today();
  time_t end   = time(NULL);

  if (health_service_metric_accessible(HealthMetricStepCount, start, end)
      & HealthServiceAccessibilityMaskAvailable) {
    s_steps = (int)health_service_sum_today(HealthMetricStepCount);
  }

  if (health_service_metric_accessible(HealthMetricWalkedDistanceMeters, start, end)
      & HealthServiceAccessibilityMaskAvailable) {
    s_distance_m = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
  }

  if (health_service_metric_accessible(HealthMetricActiveSeconds, start, end)
      & HealthServiceAccessibilityMaskAvailable) {
    s_active_min = (int)health_service_sum_today(HealthMetricActiveSeconds) / 60;
  }

  // Heart rate is a point-in-time value, not a daily sum
  if (health_service_metric_accessible(HealthMetricHeartRateBPM, end, end)
      & HealthServiceAccessibilityMaskAvailable) {
    s_heart_rate = (int)health_service_peek_current_value(HealthMetricHeartRateBPM);
  }

  update_all_quadrants();
}
#endif

// ─── AppMessage ─────────────────────────────────────────────────────────────

// Clay sends select option values as JS strings, so Pebble.sendAppMessage
// serializes them as TUPLE_CSTRING even when they look like numbers.
// Toggles arrive as TUPLE_INT. This helper handles both safely.
static int tuple_as_int(Tuple *t) {
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  return (int)t->value->int32;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Weather data (sent by JS on each fetch)
  Tuple *tempc_tuple      = dict_find(iterator, MESSAGE_KEY_TEMPERATUREC);
  Tuple *tempf_tuple      = dict_find(iterator, MESSAGE_KEY_TEMPERATUREF);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  // Clay settings (sent when the user saves config)
  Tuple *use_celsius_tuple = dict_find(iterator, MESSAGE_KEY_USECELSIUS);
  Tuple *apikey_tuple      = dict_find(iterator, MESSAGE_KEY_APIKEY);
  Tuple *use_metric_tuple  = dict_find(iterator, MESSAGE_KEY_USEMETRIC);
  Tuple *quad_tl_tuple     = dict_find(iterator, MESSAGE_KEY_QUAD_TL);
  Tuple *quad_tr_tuple     = dict_find(iterator, MESSAGE_KEY_QUAD_TR);
  Tuple *quad_bl_tuple     = dict_find(iterator, MESSAGE_KEY_QUAD_BL);
  Tuple *quad_br_tuple     = dict_find(iterator, MESSAGE_KEY_QUAD_BR);

  bool settings_changed   = false;
  bool tick_needs_resubscribe = false;

  if (use_celsius_tuple) {
    settings.use_celsius = (use_celsius_tuple->value->int32 != 0);
    settings_changed = true;
  }

  if (apikey_tuple) {
    strncpy(settings.apikey, apikey_tuple->value->cstring,
            sizeof(settings.apikey) - 1);
    settings.apikey[sizeof(settings.apikey) - 1] = '\0';
    settings_changed = true;
  }

  if (use_metric_tuple) {
    settings.use_metric = (use_metric_tuple->value->int32 != 0);
    settings_changed = true;
  }

  // For each quadrant slot: update the content type and note if the seconds
  // subscription state may have changed (so we can re-evaluate the tick rate).
  #define UPDATE_QUAD_SLOT(tuple, slot_idx) \
    if (tuple) { \
      bool was_seconds = ((QuadrantContent)settings.quad[slot_idx] == QUAD_SECONDS); \
      settings.quad[slot_idx] = (uint8_t)tuple_as_int(tuple); \
      bool is_seconds  = ((QuadrantContent)settings.quad[slot_idx] == QUAD_SECONDS); \
      if (was_seconds != is_seconds) tick_needs_resubscribe = true; \
      settings_changed = true; \
    }

  UPDATE_QUAD_SLOT(quad_tl_tuple, SLOT_TL)
  UPDATE_QUAD_SLOT(quad_tr_tuple, SLOT_TR)
  UPDATE_QUAD_SLOT(quad_bl_tuple, SLOT_BL)
  UPDATE_QUAD_SLOT(quad_br_tuple, SLOT_BR)

  #undef UPDATE_QUAD_SLOT

  if (settings_changed) {
    save_settings();
    if (tick_needs_resubscribe) resubscribe_tick();
    update_all_quadrants();
  }

  // Weather data
  if (tempc_tuple && tempf_tuple) {
    s_temp_c = (int)tempc_tuple->value->int32;
    s_temp_f = (int)tempf_tuple->value->int32;

    if (conditions_tuple) {
      strncpy(s_conditions, conditions_tuple->value->cstring,
              sizeof(s_conditions) - 1);
      s_conditions[sizeof(s_conditions) - 1] = '\0';
    }

    update_all_quadrants();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
                                   AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send success");
}

// ─── Window ─────────────────────────────────────────────────────────────────

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorBlack);

  // Center time band ────────────────────────────────────────────────────────
  const int band_height  = 48;
  const int band_frame_h = band_height + 4;
  int band_y = (bounds.size.h - band_height) / 2;

  s_time_layer = text_layer_create(
      GRect(0, band_y, bounds.size.w, band_frame_h));
  text_layer_set_background_color(s_time_layer, GColorWhite);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer,
      fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Info row positions ──────────────────────────────────────────────────────
  const int info_h = 30;
  int top_y, bottom_y, h_inset;

  // Hug the band on all platforms — preserves the original tight aesthetic
  // and scales automatically to any screen height.
  // On round, h_inset pulls text inward to clear the circle edge at this y.
  top_y    = band_y - info_h;
  bottom_y = band_y + band_frame_h - 4;
  h_inset  = PBL_IF_ROUND_ELSE(22, 0);

  int half_w = bounds.size.w / 2;

  // Quadrant rects: TL and TR are each a half-width column.
  // BL spans the full width (left-aligned) so long strings like "Wednesday"
  // are not truncated; BR sits on top in the right half (right-aligned).
  GRect quad_rects[4] = {
    GRect(h_inset, top_y,    half_w - h_inset,             info_h), // TL
    GRect(half_w,  top_y,    half_w - h_inset,             info_h), // TR
    GRect(h_inset, bottom_y, bounds.size.w - h_inset * 2,  info_h), // BL
    GRect(half_w,  bottom_y, half_w - h_inset,             info_h), // BR
  };
  GTextAlignment quad_aligns[4] = {
    GTextAlignmentLeft,  GTextAlignmentRight,
    GTextAlignmentLeft,  GTextAlignmentRight,
  };

  for (int i = 0; i < 4; i++) {
    s_quad_layer[i] = text_layer_create(quad_rects[i]);
    text_layer_set_background_color(s_quad_layer[i], GColorClear);
    text_layer_set_text_color(s_quad_layer[i], GColorWhite);
    text_layer_set_text_alignment(s_quad_layer[i], quad_aligns[i]);
    text_layer_set_font(s_quad_layer[i],
        fonts_get_system_font(FONT_KEY_GOTHIC_24));
    layer_add_child(window_layer, text_layer_get_layer(s_quad_layer[i]));
  }

  // Initial content — window_stack_push is synchronous so these are the
  // only calls needed; the duplicate calls that were previously in init()
  // have been removed.
  init_time_display();
  update_battery(battery_state_service_peek());
  update_all_quadrants();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  for (int i = 0; i < 4; i++) text_layer_destroy(s_quad_layer[i]);
}

// ─── Init / Deinit ──────────────────────────────────────────────────────────

static void init(void) {
  load_settings();

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(256, 64);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load   = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true); // calls main_window_load() synchronously

  // Subscribe at the correct granularity based on persisted settings
  resubscribe_tick();

  battery_state_service_subscribe(update_battery);

#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  // Populate health values immediately rather than waiting for first event
  health_handler(HealthEventSignificantUpdate, NULL);
#endif
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
