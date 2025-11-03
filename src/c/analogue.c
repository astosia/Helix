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
    BTQT_ICON_COLOR,
    PALETTE_SIZE
};

Window* g_window;
Layer* g_layer;
static Layer *s_canvas_bt_icon;
static Layer *s_canvas_qt_icon;
//Layer* g_layer_hr;
FFont* g_font;
static GFont FontBTQTIcons;
struct tm g_local_time;
//static ClaySettings settings;

GColor g_palette[PALETTE_SIZE];
bool connected = true;



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
settings.JumpHourOn = false;
settings.InvertScreen = false;
settings.AddZero12h = false;
settings.RemoveZero24h = false;
settings.BTVibeOn = true;
settings.DigitsColor = GColorWhite;
settings.DialColorMinutes = GColorWhite;
settings.DialColorHours = GColorWhite;
settings.ColorMinorTick = GColorWhite;
settings.ColorMajorTick = GColorWhite;
}


static inline FPoint clockToCartesian(FPoint center, fixed_t radius, int32_t angle) {
    FPoint pt;
    int32_t c = cos_lookup(angle);
    int32_t s = sin_lookup(angle);
    pt.x = center.x + s * radius / TRIG_MAX_RATIO;
    pt.y = center.y - c * radius / TRIG_MAX_RATIO;
    return pt;
}

static void update_palette() {
  if(!settings.InvertScreen){
    g_palette[      BEZEL_COLOR] = GColorWhite;
    g_palette[       FACE_COLOR] = GColorBlack;
    g_palette[MINUTE_TEXT_COLOR] = GColorWhite;
    g_palette[MINUTE_HAND_COLOR] = GColorWhite;
    g_palette[  HOUR_TEXT_COLOR] = PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack);
    g_palette[  BTQT_ICON_COLOR] = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite);

  }
  else{
    g_palette[      BEZEL_COLOR] = GColorBlack;
    g_palette[       FACE_COLOR] = GColorWhite;
    g_palette[MINUTE_TEXT_COLOR] = GColorBlack;
    g_palette[MINUTE_HAND_COLOR] = GColorBlack;
    g_palette[  HOUR_TEXT_COLOR] = PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite);
    g_palette[  BTQT_ICON_COLOR] = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack);
  }
}

static void quiet_time_icon () {
    layer_set_hidden(s_canvas_qt_icon, !quiet_time_is_active());
    layer_mark_dirty(s_canvas_qt_icon);
}

static void bluetooth_vibe_icon (bool connected) {
   layer_set_hidden(s_canvas_bt_icon, connected);

   layer_mark_dirty(s_canvas_bt_icon);

  if((!connected && !quiet_time_is_active() && settings.BTVibeOn)) {
    vibes_double_pulse();
  }
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

//size of the rings & distance from the rings of the digits
    fixed_t minute_text_radius = safe_radius - INT_TO_FIXED(DIGITS_OFFSET);
    fixed_t ring_outer_radius = safe_radius - INT_TO_FIXED(text_size + 0);
    fixed_t ring_outer_outer_radius = ring_outer_radius + INT_TO_FIXED(text_size + DIGITS_OFFSET) + INT_TO_FIXED(80);
    fixed_t ring_inner_radius = ring_outer_radius - INT_TO_FIXED(2*ZOOM_FACTOR);
    //fixed_t minute_hand_radius = ring_inner_radius - INT_TO_FIXED(8);

//sizes of the tick marks
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

      int32_t diff = abs(m - (int32_t)minute_now);
      int32_t circular_dist = (diff < 30) ? diff : (60 - diff);

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

        int32_t diff = abs(m - (int32_t)minute_now);
        int32_t circular_dist = (diff < 30) ? diff : (60 - diff);

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
        int32_t hour_angle;

        if(!settings.JumpHourOn){
          hour_angle = -(((-hour_now + h + 3) * TRIG_MAX_ANGLE / 12) - minute_offset/12);
        }
        else{
          hour_angle = -(((-hour_now + h + 3) * TRIG_MAX_ANGLE / 12));
        }

      int display_h = h;
      bool format_24h = clock_is_24h_style();

      // if (format_24h) {
      //     //Get the base 12-hour number (1-12) for the dial position 'h'
      //     if (h == 0) {
      //         display_h = 12; // Dial position 0 is 12 o'clock
      //     } else {
      //         display_h = h;
      //     }

      //     //Convert to 24-hour value based on the current time's AM/PM cycle
      //     if (g_local_time.tm_hour >= 12) {
      //         // PM cycle (12:00 - 23:59). We want 13 for 1, 14 for 2, etc.
      //         // 12 o'clock position (display_h=12) stays 12 (12PM).
      //         if (display_h != 12) {
      //             display_h += 12; // 1 -> 13, 11 -> 23
      //         }
      //     } else {
      //         // 12 o'clock position (display_h=12) becomes 0 (12AM -> 00:xx).
      //         if (display_h == 12) {
      //             display_h = 0;
      //         }
      //       }
      // } else {
      //     // 12-hour mode (e.g., 1, 12)
      //     // If h=0, display 12. Else display h.
      //     if (h == 0) {
      //         display_h = 12;
      //     } else {
      //         display_h = h;
      //     }
      // }

if (format_24h) {
          int hour_now_24h = g_local_time.tm_hour; // Current hour (0-23)
          int hour_now_12h = hour_now_24h % 12; // Current 12h position (0-11)

          // 1. Calculate the raw difference in 12-hour positions (range -11 to 11)
          int diff_12h_raw = h - hour_now_12h;

          // 2. Normalize the difference to the shortest angular distance
          // This converts the 12h dial difference (e.g., 11 or -11) into a 24h offset (-1 or +1).
          int delta_24h = diff_12h_raw;
          if (diff_12h_raw > 6) {
              delta_24h -= 12; // Example: 00:xx (0) to 11 (h) is 11 forward, corrected to -1 backward
          } else if (diff_12h_raw <= -6) {
              delta_24h += 12; // Example: 23:xx (11) to 0 (h) is -11 backward, corrected to +1 forward
          }

          // 3. Calculate the target 24-hour value and normalize
          // Add 24 before modulo to ensure positive result for negative delta_24h
          display_h = (hour_now_24h + delta_24h + 24) % 24;

      } else {
          // Keep existing 12-hour mode logic
          // 12-hour mode (e.g., 1, 12)
          // If h=0, display 12. Else display h.
          if (h == 0) {
              display_h = 12;
          } else {
              display_h = h;
          }
      }

      // 24h mode uses zero-padding ("%02d"), 12h mode does not ("%d").
      if (format_24h && !settings.RemoveZero24h) {
          snprintf(hour_string, sizeof hour_string, "%02d", display_h);
      }
      else if (format_24h && settings.RemoveZero24h) {
          snprintf(hour_string, sizeof hour_string, "%d", display_h);
      }
      else if (!format_24h && settings.AddZero12h){
          snprintf(hour_string, sizeof hour_string, "%02d", display_h);
      }
      else {
          snprintf(hour_string, sizeof hour_string, "%d", display_h);
      }

        int32_t text_rotation;
        FTextAnchor text_anchor;
        text_rotation = hour_angle + TRIG_MAX_ANGLE / 4;
        text_anchor = FTextAnchorMiddle;

        FPoint p = clockToCartesian(center_hour, minute_text_radius, hour_angle);

        if (circular_dist <= max_visible_dist_hours && !settings.JumpHourOn) {
            fctx_set_rotation(&fctx, text_rotation);
            fctx_set_offset(&fctx, p);
            fctx_draw_string(&fctx, hour_string, g_font, GTextAlignmentRight, text_anchor);
            fctx_end_fill(&fctx);

        }
        else if ( (h==hour_now || h == ((hour_now + 11) % 12) || h == ((hour_now + 1) % 12)) && settings.JumpHourOn){
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

static void layer_update_proc_bt(Layer * layer, GContext * ctx){
    GRect bounds = layer_get_bounds(layer);

    GRect BTIconRect =
      GRect(bounds.size.w/2 - 9 , bounds.size.h/2 - 17 ,18,18);

      graphics_context_set_text_color(ctx, g_palette[BTQT_ICON_COLOR]);
      graphics_context_set_antialiased(ctx, true);
      graphics_draw_text(ctx, "z", FontBTQTIcons, BTIconRect, GTextOverflowModeFill,GTextAlignmentCenter, NULL);

}

static void layer_update_proc_qt(Layer * layer, GContext * ctx){
  GRect bounds = layer_get_bounds(layer);

  GRect QTIconRect =
    GRect(bounds.size.w/2 - 9 ,bounds.size.h/2 + 3 ,18,18);

  graphics_context_set_text_color(ctx, g_palette[BTQT_ICON_COLOR]);
  graphics_context_set_antialiased(ctx, true);
  graphics_draw_text(ctx, "\U0000E061", FontBTQTIcons, QTIconRect, GTextOverflowModeFill,GTextAlignmentCenter, NULL);

}

void on_battery_state(BatteryChargeState charge) {

if(!settings.InvertScreen){

    if (charge.is_charging) {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite);
    } else if (charge.charge_percent <= 20) {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorOrange, GColorDarkGray);
    } else if (charge.charge_percent <= 50) {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray);
    } else {
        g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorMalachite, GColorWhite);
    }
  }
else{
  if (charge.is_charging) {
      g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack);
  } else if (charge.charge_percent <= 20) {
      g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorOrange, GColorLightGray);
  } else if (charge.charge_percent <= 50) {
      g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorYellow, GColorDarkGray);
  } else {
      g_palette[MINUTE_RING_COLOR_BATTERY] = PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorBlack);
  }
}

    layer_mark_dirty(g_layer);
  //  layer_mark_dirty(g_layer_hr);
}

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

Tuple *jumphour_t = dict_find(iter, MESSAGE_KEY_JumpHourOn);
Tuple *invert_t = dict_find(iter, MESSAGE_KEY_InvertScreen);
Tuple * addzero12_t = dict_find(iter, MESSAGE_KEY_AddZero12h);
Tuple * remzero24_t = dict_find(iter, MESSAGE_KEY_RemoveZero24h);
Tuple * vibe_t = dict_find(iter, MESSAGE_KEY_BTVibeOn);

if (jumphour_t) {
  settings.JumpHourOn = jumphour_t->value->int32 == 1;
  layer_mark_dirty(g_layer);
}

if (invert_t) {
  settings.InvertScreen = invert_t->value->int32 == 1;
  update_palette();
  window_set_background_color(g_window, g_palette[FACE_COLOR]);
  on_battery_state(battery_state_service_peek());
  layer_mark_dirty(g_layer);
}

if (addzero12_t) {
  settings.AddZero12h = addzero12_t->value->int32 == 1;
  layer_mark_dirty(g_layer);
}

if (remzero24_t) {
  settings.RemoveZero24h = remzero24_t->value->int32 == 1;
  layer_mark_dirty(g_layer);
}

if (vibe_t) {
  settings.BTVibeOn = vibe_t->value->int32 == 1;
  layer_mark_dirty(s_canvas_bt_icon);
}

  prv_save_settings();
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

    update_palette();

    g_font = ffont_create_from_resource(RESOURCE_ID_DIN_CONDENSED_FFONT);
    ffont_debug_log(g_font, APP_LOG_LEVEL_DEBUG);

    FontBTQTIcons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DRIPICONS_12));

    connection_service_subscribe((ConnectionHandlers){
      .pebble_app_connection_handler = bluetooth_vibe_icon
    });


    g_window = window_create();
    window_set_background_color(g_window, g_palette[FACE_COLOR]);
    window_stack_push(g_window, true);
    Layer* window_layer = window_get_root_layer(g_window);
    GRect window_frame = layer_get_frame(window_layer);



    g_layer = layer_create(window_frame);
    layer_set_update_proc(g_layer, &on_layer_update);
    layer_add_child(window_layer, g_layer);

    s_canvas_qt_icon = layer_create(window_frame);
       quiet_time_icon();
       layer_add_child(window_layer, s_canvas_qt_icon);
       layer_set_update_proc(s_canvas_qt_icon, layer_update_proc_qt);

    s_canvas_bt_icon = layer_create(window_frame);
      connected = connection_service_peek_pebble_app_connection();
      layer_set_hidden(s_canvas_bt_icon, connected);
      layer_add_child(window_layer, s_canvas_bt_icon);
      layer_set_update_proc(s_canvas_bt_icon, layer_update_proc_bt);

      bluetooth_vibe_icon(connection_service_peek_pebble_app_connection());

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
    connection_service_unsubscribe();
    window_destroy(g_window);
    layer_destroy(g_layer);
    layer_destroy(s_canvas_bt_icon);
    layer_destroy(s_canvas_qt_icon);
  //  layer_destroy(g_layer_hr);
	ffont_destroy(g_font);
  fonts_unload_custom_font(FontBTQTIcons);
}

// --------------------------------------------------------------------------
// The main event loop.
// --------------------------------------------------------------------------

int main() {
    init();
    app_event_loop();
    deinit();
}
