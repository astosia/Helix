#include <pebble.h>
#include "analogue.h"
#include "utils/hourname.h"
#include "utils/MathUtils.h"
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/fpath.h>
#include <pebble-fctx/ffont.h>

// --------------------------------------------------------------------------
// Types and global variables.
// --------------------------------------------------------------------------

enum Palette {
    BEZEL_COLOR,
    FACE_COLOR,
    MINUTE_RING_COLOR_BATTERY,
    MINUTE_TEXT_COLOR,
    MINUTE_HAND_COLOR,
    HOUR_TEXT_COLOR,
    PALETTE_SIZE
};

Window* g_window;
Layer* g_layer;
//Layer* g_layer_hr;
FFont* g_font;
struct tm g_local_time;
GColor g_palette[PALETTE_SIZE];


#if defined(PBL_ROUND)
#define BEZEL_INSET 4
#else
#define BEZEL_INSET 0
#endif

#define ZOOM_FACTOR 1.33

#ifdef PBL_PLATFORM_EMERY
  #define DIGITS_OFFSET 18*ZOOM_FACTOR
#else
  #define DIGITS_OFFSET 12*ZOOM_FACTOR
#endif


// --------------------------------------------------------------------------
// Utility functions.
// --------------------------------------------------------------------------
ClaySettings settings;
static void prv_default_settings(){
settings.ShowHourNumbers = false;
}


static inline FPoint clockToCartesian(FPoint center, fixed_t radius, int32_t angle) {
    FPoint pt;
    int32_t c = cos_lookup(angle);
    int32_t s = sin_lookup(angle);
    pt.x = center.x + s * radius / TRIG_MAX_RATIO;
    pt.y = center.y - c * radius / TRIG_MAX_RATIO;
    return pt;
}

// --------------------------------------------------------------------------
// The main drawing function.
// --------------------------------------------------------------------------

void on_layer_update(Layer* layer, GContext* ctx) {
/*draw the minutes*/
    GRect bounds = layer_get_bounds(layer);
    FPoint center_minutes = FPointI(bounds.size.w * 0.2/ZOOM_FACTOR, bounds.size.h / 2);
    FPoint center_hour = FPointI(bounds.size.w -(bounds.size.w * 0.2/ZOOM_FACTOR), bounds.size.h / 2);
    FPoint center_line = FPointI(bounds.size.w/2, bounds.size.h / 2);
    fixed_t safe_radius = INT_TO_FIXED((bounds.size.w / 2 - BEZEL_INSET)*ZOOM_FACTOR);

#ifdef PBL_PLATFORM_EMERY
  int text_size = 26*ZOOM_FACTOR;
  fixed_t center_line_length = INT_TO_FIXED(6*ZOOM_FACTOR);
//  int text_size = 28*ZOOM_FACTOR;
#elif defined(PBL_ROUND)
  int text_size = 18*ZOOM_FACTOR;
  fixed_t center_line_length = INT_TO_FIXED(6*ZOOM_FACTOR);
#else
  int text_size = 18*ZOOM_FACTOR;
  fixed_t center_line_length = INT_TO_FIXED(4*ZOOM_FACTOR);
#endif

    fixed_t minute_text_radius = safe_radius - INT_TO_FIXED(DIGITS_OFFSET);
    fixed_t ring_outer_radius = safe_radius - INT_TO_FIXED(text_size + 0);
    fixed_t ring_outer_outer_radius = ring_outer_radius + INT_TO_FIXED(text_size + DIGITS_OFFSET) + INT_TO_FIXED(80);
    fixed_t ring_inner_radius = ring_outer_radius - INT_TO_FIXED(2*ZOOM_FACTOR);
    //fixed_t minute_hand_radius = ring_inner_radius - INT_TO_FIXED(8);


    fixed_t tick_size = INT_TO_FIXED(6*ZOOM_FACTOR); // Length of the tick mark (e.g., 4 pixels long)
    fixed_t tick_width = INT_TO_FIXED(2*ZOOM_FACTOR); // Width of the tick mark (e.g., 2 pixels wide)
    fixed_t half_width = tick_width / 2;
    fixed_t outer_edge = ring_inner_radius; // Assuming ring_inner_radius is the distance to the edge
    fixed_t inner_edge = outer_edge - tick_size;
    fixed_t major_tick_size = INT_TO_FIXED(9*ZOOM_FACTOR);
    fixed_t major_tick_width = INT_TO_FIXED(2*ZOOM_FACTOR);
    fixed_t major_half_width = major_tick_width / 2;
    fixed_t major_inner_edge = outer_edge - major_tick_size;

    fixed_t hour_now = g_local_time.tm_hour %12;
    fixed_t minute_now = g_local_time.tm_min;
    fixed_t minute_offset = minute_now * TRIG_MAX_ANGLE / 60; // Total minute angle for smooth sweep

    fixed_t minute_fraction_fixed_hour = minute_now * (TRIG_MAX_RATIO / 60);
    fixed_t current_time_fixed_hours = hour_now * TRIG_MAX_RATIO + minute_fraction_fixed_hour;
    fixed_t full_cycle_fixed_hours = 12 * TRIG_MAX_RATIO;
    // 100 is the visible wedge angle for the hours digits
    fixed_t max_visible_dist_hours = 100 * TRIG_MAX_RATIO / 60; //first number is  the visbible angle
    fixed_t max_visible_dist_ticks_hours = 30 * TRIG_MAX_RATIO / 60; //first number is  the visbible angle
    // 9 * TRIG_MAX_RATIO gives an angular distance of 9 "units" (where 1 minute is 1 unit)
    fixed_t max_visible_dist_mins = 9 * TRIG_MAX_RATIO ;

    char minute_string[3];
    char hour_string[3];

   fixed_t full_cycle_fixed_minutes = 60 * TRIG_MAX_RATIO;
   fixed_t current_time_fixed_minutes = minute_now * TRIG_MAX_RATIO;



    FContext fctx;
    fctx_init_context(&fctx, ctx);
    fctx_set_color_bias(&fctx, 0);



    /* Draw a thin hour ring. */
    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, g_palette[MINUTE_RING_COLOR_BATTERY]);
    fctx_plot_circle(&fctx, &center_hour, ring_outer_radius);
    fctx_plot_circle(&fctx, &center_hour, ring_inner_radius);
    fctx_end_fill(&fctx);

    /* Draw a thin minute ring. */
    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, g_palette[MINUTE_RING_COLOR_BATTERY]);
    fctx_plot_circle(&fctx, &center_minutes, ring_outer_radius);
    fctx_plot_circle(&fctx, &center_minutes, ring_inner_radius);
    fctx_end_fill(&fctx);

    fctx_set_color_bias(&fctx, -3);

    //draw thick outer ring
    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, g_palette[FACE_COLOR]);
    fctx_plot_circle(&fctx, &center_minutes, minute_text_radius - INT_TO_FIXED(1));
    fctx_plot_circle(&fctx, &center_minutes, ring_outer_outer_radius);
    fctx_end_fill(&fctx);

    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, g_palette[FACE_COLOR]);
    fctx_plot_circle(&fctx, &center_hour, minute_text_radius - INT_TO_FIXED(1));
    fctx_plot_circle(&fctx, &center_hour, ring_outer_outer_radius);
    fctx_end_fill(&fctx);

    fctx_set_color_bias(&fctx, 0);
/////////////////////////////////////
///Draw minutes
/////////////////////////////////////
    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, g_palette[MINUTE_TEXT_COLOR]);
    fctx_set_text_em_height(&fctx,g_font, text_size);
    for (int m = 0; m < 60; m += 5) {
          snprintf(minute_string, sizeof minute_string, "%02d", m);

          fixed_t m_fixed = m * TRIG_MAX_RATIO;
          fixed_t diff = abs(m_fixed - current_time_fixed_minutes);
          fixed_t circular_dist = (diff < full_cycle_fixed_minutes / 2) ? diff : (full_cycle_fixed_minutes - diff);

          if (circular_dist <= max_visible_dist_mins) {

        int32_t minute_angle = (-minute_now + m + 15) * TRIG_MAX_ANGLE / 60;


        int32_t text_rotation;
        FTextAnchor text_anchor;
            text_rotation = minute_angle - TRIG_MAX_ANGLE / 4;
            text_anchor = FTextAnchorMiddle;

        FPoint p = clockToCartesian(center_minutes, minute_text_radius, minute_angle);
        fctx_set_rotation(&fctx, text_rotation);
        fctx_set_offset(&fctx, p);
        fctx_draw_string(&fctx, minute_string, g_font, GTextAlignmentLeft, text_anchor);
    }
  }
    fctx_end_fill(&fctx);

/* Draw the major ticks. */
    for (int m = 0; m < 60; m += 5) {
      snprintf(minute_string, sizeof minute_string, "%02d", m);

      int32_t circular_dist = abs(m - (minute_now % 60));

      if (circular_dist < 5) {
      int32_t minute_angle = (-minute_now + m + 15) * TRIG_MAX_ANGLE / 60;

    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, g_palette[MINUTE_HAND_COLOR]);
    fctx_set_scale(&fctx, FPointOne, FPointOne);
    fctx_set_rotation(&fctx, minute_angle);
    fctx_set_offset(&fctx, center_minutes);

    fctx_move_to (&fctx, FPoint(-major_half_width, -major_inner_edge));
    fctx_line_to (&fctx, FPoint( major_half_width, -major_inner_edge));
    fctx_line_to (&fctx, FPoint( major_half_width, -outer_edge));
    fctx_line_to (&fctx, FPoint(-major_half_width, -outer_edge));
    fctx_close_path(&fctx);
    fctx_end_fill(&fctx);
    }
  }

    for (int m = 0; m < 60; m++) {
    //     // ignore every 5th minute mark, which is usually a larger hour tick
      snprintf(minute_string, sizeof minute_string, "%02d", m);
         if (m % 5 == 0) continue;

      int32_t circular_dist = abs(m - (minute_now % 60));

      if (circular_dist < 3) {
      int32_t minute_angle = (-minute_now + m + 15) * TRIG_MAX_ANGLE / 60;
      fctx_begin_fill(&fctx);
      fctx_set_fill_color(&fctx, g_palette[MINUTE_RING_COLOR_BATTERY]);
      fctx_set_scale(&fctx, FPointOne, FPointOne);
      fctx_set_rotation(&fctx, minute_angle);
      fctx_set_offset(&fctx, center_minutes);


          fctx_move_to (&fctx, FPoint(-half_width, -inner_edge));
          fctx_line_to (&fctx, FPoint( half_width, -inner_edge));
          fctx_line_to (&fctx, FPoint( half_width, -outer_edge));
          fctx_line_to (&fctx, FPoint(-half_width, -outer_edge));
          fctx_close_path(&fctx);
          fctx_end_fill(&fctx);
        }
    }

    for (int h = 0; h < 12; h += 1) {
        fctx_begin_fill(&fctx);
        fctx_set_fill_color(&fctx, g_palette[MINUTE_TEXT_COLOR]);
        fctx_set_text_em_height(&fctx,g_font, text_size);

        fixed_t h_fixed = h * TRIG_MAX_RATIO;
        fixed_t diff = abs(h_fixed - current_time_fixed_hours);
        fixed_t circular_dist = (diff < full_cycle_fixed_hours / 2) ? diff : (full_cycle_fixed_hours - diff);

        int32_t hour_angle = -(((-hour_now + h + 3) * TRIG_MAX_ANGLE / 12) - minute_offset/12);

        snprintf(hour_string, sizeof hour_string, "%d", h);

        int32_t text_rotation;
        FTextAnchor text_anchor;
        text_rotation = hour_angle + TRIG_MAX_ANGLE / 4;
        text_anchor = FTextAnchorMiddle;

        FPoint p = clockToCartesian(center_hour, minute_text_radius, hour_angle);

        if (circular_dist <= max_visible_dist_hours) {
            fctx_set_rotation(&fctx, text_rotation);
            fctx_set_offset(&fctx, p);
            fctx_draw_string(&fctx, hour_string, g_font, GTextAlignmentRight, text_anchor);
            fctx_end_fill(&fctx);

        }

        //if (circular_dist <= max_visible_dist_ticks) {
        if (h == hour_now) {

                fctx_begin_fill(&fctx);
                fctx_set_fill_color(&fctx, g_palette[MINUTE_HAND_COLOR]);
                fctx_set_scale(&fctx, FPointOne, FPointOne);
                fctx_set_rotation(&fctx, hour_angle);
                fctx_set_offset(&fctx, center_hour);

                // Draw the rectangular tick (positioned along the Y-axis and then rotated)
                // 1. Start path: Top-Left corner (Inner edge of the tick)
                fctx_move_to (&fctx, FPoint(-major_half_width, -major_inner_edge));

                // 2. Top-Right corner
                fctx_line_to (&fctx, FPoint( major_half_width, -major_inner_edge));

                // 3. Bottom-Right corner (Outer edge of the tick)
                fctx_line_to (&fctx, FPoint( major_half_width, -outer_edge));

                // 4. Bottom-Left corner
                fctx_line_to (&fctx, FPoint(-major_half_width, -outer_edge));
                fctx_close_path(&fctx);
                fctx_end_fill(&fctx);
        }
    }


//draw short line in centre
    fctx_begin_fill(&fctx);
    fctx_set_fill_color(&fctx, g_palette[MINUTE_TEXT_COLOR]);
    fctx_set_scale(&fctx, FPointOne, FPointOne);
    fctx_set_rotation(&fctx, TRIG_MAX_ANGLE/4);
    fctx_set_offset(&fctx, center_line);


    fctx_move_to (&fctx, FPoint(-major_half_width, -center_line_length));
    fctx_line_to (&fctx, FPoint( major_half_width, -center_line_length));
    fctx_line_to (&fctx, FPoint( major_half_width, center_line_length));
    fctx_line_to (&fctx, FPoint(-major_half_width, center_line_length));
    fctx_close_path(&fctx);
    fctx_end_fill(&fctx);

    fctx_deinit_context(&fctx);

}

// void on_layer_update_hr(Layer* layer, GContext* ctx) {
// /*draw the hour*/
//   GRect bounds = layer_get_bounds(layer);
//     FPoint center_minutes = FPointI(bounds.size.w * 0.25, bounds.size.h / 2);
//   FPoint center_hour = FPointI(bounds.size.w * 0.75, bounds.size.h / 2);
//   fixed_t safe_radius = INT_TO_FIXED(bounds.size.w / 2 - BEZEL_INSET);
//
// #ifdef PBL_PLATFORM_EMERY
//   int text_size = 28;
// #else
//   int text_size = 18;
// #endif
//
//   fixed_t minute_text_radius = safe_radius - INT_TO_FIXED(DIGITS_OFFSET);
//   fixed_t ring_outer_radius = safe_radius - INT_TO_FIXED(text_size + 0);
//   fixed_t ring_outer_outer_radius = ring_outer_radius + INT_TO_FIXED(text_size + DIGITS_OFFSET);
//   fixed_t ring_inner_radius = ring_outer_radius - INT_TO_FIXED(2);
//   fixed_t minute_hand_radius = ring_inner_radius - INT_TO_FIXED(8);
//
//   fixed_t tick_size = INT_TO_FIXED(6); // Length of the tick mark (e.g., 4 pixels long)
//   fixed_t tick_width = INT_TO_FIXED(2); // Width of the tick mark (e.g., 2 pixels wide)
//   fixed_t half_width = tick_width / 2;
//   fixed_t outer_edge = ring_inner_radius; // Assuming ring_inner_radius is the distance to the edge
//   fixed_t inner_edge = outer_edge - tick_size;
//   fixed_t major_tick_size = INT_TO_FIXED(9);
//   fixed_t major_tick_width = INT_TO_FIXED(2);
//   fixed_t major_half_width = major_tick_width / 2;
//   fixed_t major_inner_edge = outer_edge - major_tick_size;
//
//     fixed_t hour_now = g_local_time.tm_hour %12;
//     fixed_t minute_now = g_local_time.tm_min;
//     fixed_t minute_offset = minute_now * TRIG_MAX_ANGLE / 60; // Total minute angle for smooth sweep
//
//     fixed_t minute_fraction_fixed_hour = minute_now * (TRIG_MAX_RATIO / 60);
//     fixed_t current_time_fixed = hour_now * TRIG_MAX_RATIO + minute_fraction_fixed_hour;
//     fixed_t full_cycle_fixed = 12 * TRIG_MAX_RATIO;
//     fixed_t max_visible_dist_hours = 100 * TRIG_MAX_RATIO / 60; //first number is  the visbible angle
//     fixed_t max_visible_dist_ticks_hours = 30 * TRIG_MAX_RATIO / 60; //first number is  the visbible angle
//     // ----------------------------------------------------------------------
//
//     char minute_string[3];
//     char hour_string[3];
//
//     FContext fctx;
//     fctx_init_context(&fctx, ctx);
//     fctx_set_color_bias(&fctx, 0);
//
//
//     /* Draw the hour digits. */
//
//
//
//
//
//
//     fctx_deinit_context(&fctx);
// }
// --------------------------------------------------------------------------
// System event handlers.
// --------------------------------------------------------------------------
static void prv_load_settings(){
  // Load the default settings
  prv_default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, & settings, sizeof(settings));
}
// Save the settings to persistent storage
static void prv_save_settings(){
  persist_write_data(SETTINGS_KEY, & settings, sizeof(settings));

}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
#ifdef LOG
  APP_LOG(APP_LOG_LEVEL_INFO, "Received message");
#endif
Tuple *hournumberson_t = dict_find(iter, MESSAGE_KEY_ShowHourNumbers);
if (hournumberson_t){
  if (hournumberson_t -> value -> int32 == 0){
    settings.ShowHourNumbers = false;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Vibe off");
  } else {
    settings.ShowHourNumbers = true;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Vibe on");
  }
  layer_mark_dirty(g_layer);
  //layer_mark_dirty(g_layer_hr);
}
  prv_save_settings();
}

void on_battery_state(BatteryChargeState charge) {

    if (charge.is_charging) {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite);
    } else if (charge.charge_percent <= 20) {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorOrange, GColorDarkGray);
    } else if (charge.charge_percent <= 50) {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray);
    } else {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorScreaminGreen, GColorWhite);
    }

    layer_mark_dirty(g_layer);
  //  layer_mark_dirty(g_layer_hr);
}

void on_tick_timer(struct tm* tick_time, TimeUnits units_changed) {
    g_local_time = *tick_time;
    layer_mark_dirty(g_layer);
  //  layer_mark_dirty(g_layer_hr);
}

// --------------------------------------------------------------------------
// Initialization and teardown.
// --------------------------------------------------------------------------

static void init() {

    setlocale(LC_ALL, "");

    prv_load_settings();

    // Open AppMessage and set the message handler
    app_message_open(256, 256);
    app_message_register_inbox_received(prv_inbox_received_handler);

    g_palette[      BEZEL_COLOR] = GColorWhite;
    g_palette[       FACE_COLOR] = GColorBlack;
    g_palette[MINUTE_TEXT_COLOR] = GColorWhite;
    g_palette[MINUTE_HAND_COLOR] = GColorWhite;
    g_palette[  HOUR_TEXT_COLOR] = PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack);

    g_font = ffont_create_from_resource(RESOURCE_ID_DIN_CONDENSED_FFONT);
    ffont_debug_log(g_font, APP_LOG_LEVEL_DEBUG);

    g_window = window_create();
    window_set_background_color(g_window, g_palette[FACE_COLOR]);
    window_stack_push(g_window, true);
    Layer* window_layer = window_get_root_layer(g_window);
    GRect window_frame = layer_get_frame(window_layer);



    g_layer = layer_create(window_frame);
    layer_set_update_proc(g_layer, &on_layer_update);
    layer_add_child(window_layer, g_layer);

    // g_layer_hr = layer_create(window_frame);
    // layer_set_update_proc(g_layer_hr, &on_layer_update_hr);
    // layer_add_child(window_layer, g_layer_hr);

    time_t now = time(NULL);
    g_local_time = *localtime(&now);
    on_battery_state(battery_state_service_peek());

    tick_timer_service_subscribe(MINUTE_UNIT, &on_tick_timer);

    battery_state_service_subscribe(&on_battery_state);
}

static void deinit() {
    battery_state_service_unsubscribe();
    tick_timer_service_unsubscribe();
    window_destroy(g_window);
    layer_destroy(g_layer);
  //  layer_destroy(g_layer_hr);
	ffont_destroy(g_font);
}

// --------------------------------------------------------------------------
// The main event loop.
// --------------------------------------------------------------------------

int main() {
    init();
    app_event_loop();
    deinit();
}
