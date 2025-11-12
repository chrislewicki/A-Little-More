#include <pebble.h>

static Window     *s_main_window;
static TextLayer  *s_time_layer;
static TextLayer  *s_battery_layer;
static TextLayer  *s_weather_layer;
static TextLayer  *s_day_layer;
static TextLayer  *s_date_layer;

static void update_time() {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Time
    static char s_time_buffer[8];  // "00:00" + safety
    strftime(s_time_buffer, sizeof(s_time_buffer),
            clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    // Strip leading zero for 12h if present
    if(!clock_is_24h_style() && s_time_buffer[0] == '0') {
        memmove(s_time_buffer, s_time_buffer + 1, strlen(s_time_buffer));
    }
    text_layer_set_text(s_time_layer, s_time_buffer);

    // Day
    static char s_day_buffer[16];  // "Wednesday"
    strftime(s_day_buffer, sizeof(s_day_buffer), "%A", tick_time);
    text_layer_set_text(s_day_layer, s_day_buffer);

    // Date (e.g., "Jun 2")
    static char s_date_buffer[16];
    strftime(s_date_buffer, sizeof(s_date_buffer), "%b %e", tick_time);
    text_layer_set_text(s_date_layer, s_date_buffer);
}

static void update_battery(BatteryChargeState state) {
    static char s_battery_buffer[8];  // "100%"
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", state.charge_percent);
    text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    if(units_changed & MINUTE_UNIT) {
        update_time();
    }
    
    // Update weather automatically
    if (tick_time->tm_min % 30 == 0) {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);

        dict_write_uint8(iter, 0, 0);

        app_message_outbox_send();
    }
}

// BEGIN APPMESSAGE STUFF
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    static char temperaturec_buffer[8];
    static char temperaturef_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[32];

    Tuple *tempc_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATUREC);
    Tuple *tempf_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATUREF);
    Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

    if (tempc_tuple && tempf_tuple && conditions_tuple) {
        snprintf(temperaturec_buffer, sizeof(temperaturec_buffer), "%d°C", (int)tempc_tuple->value->int32);
        snprintf(temperaturef_buffer, sizeof(temperaturef_buffer), "%d°F", (int)tempf_tuple->value->int32);
        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    }

    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperaturef_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send success!");
}
// END APPMESSAGE STUFF


static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    window_set_background_color(window, GColorBlack);

    // Central white band for time
    const int band_height = 48;
    GRect time_frame = GRect(0,
                            (bounds.size.h - band_height) / 2,
                            bounds.size.w,
                            band_height + 4);

    s_time_layer = text_layer_create(time_frame);
    text_layer_set_background_color(s_time_layer, GColorWhite);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    text_layer_set_font(s_time_layer,
                        fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    // Top-left: battery "70%"
    s_battery_layer = text_layer_create(GRect(0, 32, bounds.size.w / 2, 30));
    text_layer_set_background_color(s_battery_layer, GColorClear);
    text_layer_set_text_color(s_battery_layer, GColorWhite);
    text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
    text_layer_set_font(s_battery_layer,
                        fonts_get_system_font(FONT_KEY_GOTHIC_24));
    layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

    // Top-right: temperature "30°C"
    s_weather_layer = text_layer_create(GRect(bounds.size.w / 2, 32,
                                                bounds.size.w / 2, 30));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorWhite);
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
    text_layer_set_font(s_weather_layer,
                        fonts_get_system_font(FONT_KEY_GOTHIC_24));
    text_layer_set_text(s_weather_layer, "   ");  // initial placeholder
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));

    // Bottom-left: weekday "Tuesday"
    s_day_layer = text_layer_create(GRect(0, bounds.size.h - 60,
                                            bounds.size.w, 30));
    text_layer_set_background_color(s_day_layer, GColorClear);
    text_layer_set_text_color(s_day_layer, GColorWhite);
    text_layer_set_text_alignment(s_day_layer, GTextAlignmentLeft);
    text_layer_set_font(s_day_layer,
                        fonts_get_system_font(FONT_KEY_GOTHIC_24));
    layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

    // Bottom-right: date "Jun 2"
    s_date_layer = text_layer_create(GRect(bounds.size.w / 2, bounds.size.h - 60,
                                            bounds.size.w / 2, 30));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
    text_layer_set_font(s_date_layer,
                        fonts_get_system_font(FONT_KEY_GOTHIC_24));
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Initial population
    update_time();
    update_battery(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_battery_layer);
    text_layer_destroy(s_weather_layer);
    text_layer_destroy(s_day_layer);
    text_layer_destroy(s_date_layer);
}


static void init(void) {
    //blow me
    app_message_register_inbox_received(inbox_received_callback);
    const int inbox_size = 256;
    const int outbox_size = 128;
    app_message_open(inbox_size, outbox_size);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    window_stack_push(s_main_window, true);

    // Time updates
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    update_time();

    // Battery updates
    battery_state_service_subscribe(update_battery);
    update_battery(battery_state_service_peek());
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
