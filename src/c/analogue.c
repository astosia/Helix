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

static void on_tick_timer(struct tm* tick_time, TimeUnits units_changed);

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
//FFont* g_font_remote;
static GFont FontBTQTIcons;
struct tm g_local_time;
struct tm g_remote_time; //second timezone settings


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

#define ANGLE_OFFSET_DEGREES 12 // Shift by 15 degrees clockwise from the center line.


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
settings.tz_mode = 0;
settings.tz_id = 0;
settings.tz_offset = 0;
}

static int REMOTE_TIME_OFFSET_HOURS = 0;
static int REMOTE_TIME_OFFSET_MINUTES = 0;

void update_offset_vars(int32_t total_seconds) {
  
    int32_t abs_seconds = (total_seconds < 0) ? -total_seconds : total_seconds;

    REMOTE_TIME_OFFSET_HOURS = abs_seconds / 3600;
    REMOTE_TIME_OFFSET_MINUTES = (abs_seconds % 3600) / 60;
    
    // Keep signage for hours if negative:
    if (total_seconds < 0) {
        REMOTE_TIME_OFFSET_HOURS = -REMOTE_TIME_OFFSET_HOURS;
    }
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
    g_palette[  HOUR_TEXT_COLOR] = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite);
    g_palette[  BTQT_ICON_COLOR] = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite);

  }
  else{
    g_palette[      BEZEL_COLOR] = GColorBlack;
    g_palette[       FACE_COLOR] = GColorWhite;
    g_palette[MINUTE_TEXT_COLOR] = GColorBlack;
    g_palette[MINUTE_HAND_COLOR] = GColorBlack;
    g_palette[  HOUR_TEXT_COLOR] = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack);
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
  int text_size_remote = text_size - 12;
  fixed_t center_line_length = INT_TO_FIXED(6*ZOOM_FACTOR);
#elif defined(PBL_ROUND)
  int text_size = 22*ZOOM_FACTOR;
  int text_size_remote = text_size - 10;
  fixed_t center_line_length = INT_TO_FIXED(6*ZOOM_FACTOR);
#else
  int text_size = 18*ZOOM_FACTOR;
  int text_size_remote = text_size - 8;
  fixed_t center_line_length = INT_TO_FIXED(4*ZOOM_FACTOR);
#endif

//size of the rings & distance from the rings of the digits
    fixed_t minute_text_radius = safe_radius - INT_TO_FIXED(DIGITS_OFFSET);
    fixed_t remote_text_radius = safe_radius - INT_TO_FIXED(DIGITS_OFFSET - 2);
    fixed_t ring_outer_radius = safe_radius - INT_TO_FIXED(text_size + 0);
    fixed_t ring_outer_outer_radius = ring_outer_radius + INT_TO_FIXED(text_size + DIGITS_OFFSET) + INT_TO_FIXED(80);
    fixed_t ring_inner_radius = ring_outer_radius - INT_TO_FIXED(2*ZOOM_FACTOR);

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

    // 9 * TRIG_MAX_RATIO gives an angular distance of 9 "units" (where 1 minute is 1 unit)
    fixed_t max_visible_dist_mins = 9 * TRIG_MAX_RATIO ;
    fixed_t remote_minute_margin = 3 * TRIG_MAX_RATIO; // NEW: Margin for remote minute display (3 minutes)

    char minute_string[3];
    char hour_string[3];
    char remote_minute_string[3]; 

   fixed_t full_cycle_fixed_minutes = 60 * TRIG_MAX_RATIO;
   fixed_t current_time_fixed_minutes = minute_now * TRIG_MAX_RATIO;

    // Calculate fixed minute difference
    int minute_diff_raw = g_remote_time.tm_min - g_local_time.tm_min;
    int minute_diff = (minute_diff_raw % 60 + 60) % 60; 

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
///Draw minutes Text
/////////////////////////////////////
    fctx_set_text_em_height(&fctx,g_font, text_size);
    
    for (int m = 0; m < 60; m += 5) {
          
          fixed_t m_fixed = m * TRIG_MAX_RATIO;
          fixed_t diff = abs(m_fixed - current_time_fixed_minutes);
          fixed_t circular_dist = (diff < full_cycle_fixed_minutes / 2) ? diff : (full_cycle_fixed_minutes - diff);

          if (circular_dist <= max_visible_dist_mins) {
            
            // 1. Calculate the remote minute value at dial position 'm'
            int remote_minute_value_at_m = (m + minute_diff) % 60;
            snprintf(remote_minute_string, sizeof remote_minute_string, "%02d", remote_minute_value_at_m);

            // 2. Draw Local Minute
            snprintf(minute_string, sizeof minute_string, "%02d", m);

            int32_t minute_angle = (-minute_now + m + 15) * TRIG_MAX_ANGLE / 60;

            int32_t text_rotation;
            FTextAnchor text_anchor;
                text_rotation = minute_angle - TRIG_MAX_ANGLE / 4;
                text_anchor = FTextAnchorMiddle;

            // --- Draw Local Minute (P) ---
            fctx_begin_fill(&fctx);
            fctx_set_fill_color(&fctx, g_palette[MINUTE_TEXT_COLOR]); 
            fctx_set_text_em_height(&fctx,g_font, text_size); 
            FPoint p = clockToCartesian(center_minutes, minute_text_radius, minute_angle);
            fctx_set_rotation(&fctx, text_rotation);
            fctx_set_offset(&fctx, p);
            fctx_draw_string(&fctx, minute_string, g_font, GTextAlignmentLeft, text_anchor);
            fctx_end_fill(&fctx);

            // --- Draw Remote Minute (Remote P) - ONLY IF CLOSE TO CURRENT MINUTE ---
            if ((circular_dist <= remote_minute_margin) && settings.tz_mode && (REMOTE_TIME_OFFSET_MINUTES > 0)) { 
                fctx_begin_fill(&fctx);
                fctx_set_fill_color(&fctx, g_palette[HOUR_TEXT_COLOR]); 

                // Angular shift for remote minute (using ANGLE_OFFSET_DEGREES)
                int32_t remote_minute_angle = minute_angle + DEG_TO_TRIGANGLE(ANGLE_OFFSET_DEGREES); 
                FPoint remote_p = clockToCartesian(center_minutes, remote_text_radius, remote_minute_angle);

                fctx_set_rotation(&fctx, text_rotation);
                fctx_set_offset(&fctx, remote_p);
                fctx_set_text_em_height(&fctx,g_font, text_size_remote); // Smaller font for remote
                fctx_draw_string(&fctx, remote_minute_string, g_font, GTextAlignmentLeft, text_anchor);
                fctx_end_fill(&fctx);
            }
        }
    }
    
/////////////////////////////////////
///Draw minute Ticks 
/////////////////////////////////////

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

/* Draw the minor ticks. */
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
/////////////////////////////////////
///Draw Hours Text
/////////////////////////////////////

    for (int h = 0; h < 12; h += 1) {
        
        fixed_t h_fixed = h * TRIG_MAX_RATIO;
        fixed_t diff = abs(h_fixed - current_time_fixed_hours);
        fixed_t circular_dist = (diff < full_cycle_fixed_hours / 2) ? diff : (full_cycle_fixed_hours - diff);
        int32_t hour_angle;

//        fixed_t local_angle = DEG_TO_TRIGANGLE(360 - 30 * h);

        if(!settings.JumpHourOn){
          hour_angle = -(((-hour_now + h + 3) * TRIG_MAX_ANGLE / 12) - minute_offset/12);
        }
        else{
          hour_angle = -(((-hour_now + h + 3) * TRIG_MAX_ANGLE / 12));
        }

      int display_h;
      bool format_24h = clock_is_24h_style();

      char remote_num_buffer[3];
      int remote_h_display;

      int local_24h_value_at_h; 
      int remote_h_24_calc;     


      if (format_24h) {
          int hour_now_24h = g_local_time.tm_hour; 
          int hour_now_12h = hour_now_24h % 12; 

          int diff_12h_raw = h - hour_now_12h;

          int delta_24h = diff_12h_raw;

          if (diff_12h_raw > 6) {
              delta_24h -= 12;
          } else if (diff_12h_raw <= -6) {
              delta_24h += 12;
          }

          local_24h_value_at_h = (hour_now_24h + delta_24h + 24) % 24;
          display_h = local_24h_value_at_h; 
          
          
          // START OF NEW/FIXED REMOTE HOUR CALCULATION
          if (h == (int)hour_now) {
             // For the current hour, use the pre-calculated time from on_tick_timer
             remote_h_24_calc = g_remote_time.tm_hour; 
          } else {
             // For all other hour markers on the dial, use the static hour offset
             remote_h_24_calc = (local_24h_value_at_h + REMOTE_TIME_OFFSET_HOURS + 24) % 24;
          }
          // END OF NEW/FIXED REMOTE HOUR CALCULATION

          remote_h_display = remote_h_24_calc; 

      } else {
          int hour_now_24h = g_local_time.tm_hour; 
          int hour_now_12h = g_local_time.tm_hour % 12; // Use raw 12h value
          int diff_12h_raw = h - hour_now_12h;
          int delta_24h = diff_12h_raw;

          if (diff_12h_raw > 6) {
              delta_24h -= 12;
          } else if (diff_12h_raw <= -6) {
              delta_24h += 12;
          }
          
          local_24h_value_at_h = (hour_now_24h + delta_24h + 24) % 24;
          
          if (local_24h_value_at_h % 12 == 0) {
              display_h = 12;
          } else {
              display_h = local_24h_value_at_h % 12;
          }

          // START OF NEW/FIXED REMOTE HOUR CALCULATION
          if (h == (int)hour_now) {
             // For the current hour, use the pre-calculated time from on_tick_timer
             remote_h_24_calc = g_remote_time.tm_hour;
          } else {
             // For all other hour markers on the dial, use the static hour offset
             remote_h_24_calc = (local_24h_value_at_h + REMOTE_TIME_OFFSET_HOURS + 24) % 24;
          }
          // END OF NEW/FIXED REMOTE HOUR CALCULATION
          
          if (remote_h_24_calc % 12 == 0) {
                remote_h_display = 12;
            } else{
                remote_h_display = remote_h_24_calc % 12; 
            }
      }

      // 24h mode uses zero-padding ("%02d"), 12h mode does not ("%d").
      if (format_24h && !settings.RemoveZero24h) {
          snprintf(hour_string, sizeof hour_string, "%02d", display_h);
          snprintf(remote_num_buffer, sizeof remote_num_buffer, "%02d", remote_h_display);
      }
      else if (format_24h && settings.RemoveZero24h) {
          snprintf(hour_string, sizeof hour_string, "%d", display_h);
          snprintf(remote_num_buffer, sizeof remote_num_buffer, "%d", remote_h_display);
      }
      else if (!format_24h && settings.AddZero12h){
          snprintf(hour_string, sizeof hour_string, "%02d", display_h);
          snprintf(remote_num_buffer, sizeof remote_num_buffer, "%02d", remote_h_display);
      }
      else {
          snprintf(hour_string, sizeof hour_string, "%d", display_h);
          snprintf(remote_num_buffer, sizeof remote_num_buffer, "%d", remote_h_display);
      }

        int32_t text_rotation;
        FTextAnchor text_anchor;
        text_rotation = hour_angle + TRIG_MAX_ANGLE / 4;
        text_anchor = FTextAnchorMiddle;

        FPoint p = clockToCartesian(center_hour, minute_text_radius, hour_angle);

        // Apply angular shift for remote hour
        fixed_t remote_angle = hour_angle - DEG_TO_TRIGANGLE(ANGLE_OFFSET_DEGREES);
        FPoint remote_p = clockToCartesian(center_hour, remote_text_radius, remote_angle);
        
        // --- Condition for Drawing LOCAL Hour ---
        bool draw_local_hour = false;
        if (circular_dist <= max_visible_dist_hours && !settings.JumpHourOn) {
             draw_local_hour = true;
        } else if ( (h == (int)hour_now || h == ((int)hour_now + 11) % 12 || h == ((int)hour_now + 1) % 12) && settings.JumpHourOn){
            draw_local_hour = true;
        }

        if (draw_local_hour) {
            // Draw Local Hour
            fctx_begin_fill(&fctx);
            fctx_set_fill_color(&fctx, g_palette[MINUTE_TEXT_COLOR]); 
            fctx_set_text_em_height(&fctx,g_font, text_size);
            fctx_set_rotation(&fctx, text_rotation);
            fctx_set_offset(&fctx, p);
            fctx_draw_string(&fctx, hour_string, g_font, GTextAlignmentRight, text_anchor);
            fctx_end_fill(&fctx);
        }

        // --- Condition for Drawing REMOTE Hour (Only for the current hour marker) ---
        if ((h == (int)hour_now) && settings.tz_mode) {
            // Draw Remote Hour

            if(settings.tz_offset == -1){
              // Error state: display "99"
              snprintf(remote_num_buffer, sizeof remote_num_buffer, "99");
              }   

            fctx_begin_fill(&fctx);
            fctx_set_fill_color(&fctx, g_palette[HOUR_TEXT_COLOR]); 
            fctx_set_rotation(&fctx, text_rotation);
            fctx_set_offset(&fctx, remote_p);
            fctx_set_text_em_height(&fctx,g_font, text_size_remote); 
            fctx_draw_string(&fctx, remote_num_buffer, g_font, GTextAlignmentRight, text_anchor);
            fctx_end_fill(&fctx);
        }


        // Hour Tick Drawing Logic (Only current hour gets a tick)
        if (h == hour_now) {

                fctx_begin_fill(&fctx);
                fctx_set_fill_color(&fctx, g_palette[MINUTE_HAND_COLOR]);
                fctx_set_scale(&fctx, FPointOne, FPointOne);
                fctx_set_rotation(&fctx, hour_angle);
                fctx_set_offset(&fctx, center_hour);

                // Draw the rectangular tick (positioned along the Y-axis and then rotated)
                fctx_move_to (&fctx, FPoint(-major_half_width, -major_inner_edge));
                fctx_line_to (&fctx, FPoint( major_half_width, -major_inner_edge));
                fctx_line_to (&fctx, FPoint( major_half_width, -outer_edge));
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
Tuple *tzmode_t = dict_find(iter, MESSAGE_KEY_TZ_MODE);
Tuple *tzid_t = dict_find(iter, MESSAGE_KEY_TZ_ID);
Tuple *tzoffset_t = dict_find(iter, MESSAGE_KEY_TZ_OFFSET);
//Tuple *tzcode_t = dict_find(iter, MESSAGE_KEY_TZ_CODE);


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

if (tzmode_t) {
  settings.tz_mode = tzmode_t->value->int32 == 1;
  time_t now = time(NULL);
  on_tick_timer(localtime(&now), MINUTE_UNIT);
  layer_mark_dirty(g_layer);
}

if(tzid_t) {
  settings.tz_id = (int)tzid_t->value->int32;
  time_t now = time(NULL);
  on_tick_timer(localtime(&now), MINUTE_UNIT);
  // layer_mark_dirty(g_layer);
}

if(tzoffset_t) {
  settings.tz_offset = (int)tzoffset_t->value->int32;
    if (settings.tz_offset != -1) {
    update_offset_vars(settings.tz_offset);
    }
  //update_offset_vars(settings.tz_offset);
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  on_tick_timer(tick_time, MINUTE_UNIT);
  layer_mark_dirty(g_layer);
}

// if(tzcode_t) {
//   strcpy(settings.tz_code, tzcode_t->value->cstring);
//   layer_mark_dirty(g_layer);
// }


  prv_save_settings();
}



static void on_tick_timer(struct tm* tick_time, TimeUnits units_changed) {
    // Step 1: Update local time struct with the minute-aligned time
    g_local_time = *tick_time; 

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick at %02d:%02d:%02d | Units: %d ", 
          tick_time->tm_hour, tick_time->tm_min, tick_time->tm_sec, (int)units_changed);

    if(settings.tz_mode){
      time_t local_time_t = mktime(tick_time); 
      time_t remote_time_t = local_time_t + settings.tz_offset;
      g_remote_time = *gmtime(&remote_time_t); 
    }

    layer_mark_dirty(g_layer);
}

// --------------------------------------------------------------------------
// Initialization and teardown.
// --------------------------------------------------------------------------

static void init() {

    setlocale(LC_ALL, "");

    prv_load_settings();

    update_offset_vars(settings.tz_offset);

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

    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    on_tick_timer(tick_time, MINUTE_UNIT);

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