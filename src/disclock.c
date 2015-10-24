/*

  Discordian Clock Watchface
  (c) 2015 by Lukas Zeller, plan44.ch

 */

#include <pebble.h>

// weekdays
const char * const dweekdayTexts[5] = {
  "Sweetmorn",
  "Boomtime",
  "Pungenday",
  "Prickle-Prickle",
  "Setting Orange"
};

// seasons
const char * const seasonTexts[5] = {
  "Chaos",
  "Discord",
  "Confusion",
  "Bureaucracy",
  #ifdef PBL_ROUND
  "Aftermath"
  #else
  "the Aftermath"
  #endif
};

// holydays
const char * const holydayTexts[5] = {
  "Mungday",
  "Mojoday",
  "Syaday",
  "Zaraday",
  "Maladay"
};
const char * const fluxdayTexts[5] = {
  "Chaoflux",
  "Discoflux",
  "Confuflux",
  "Bureflux",
  "Afflux"
};
const char *sttibsdayText = "St. Tib's Day";
const char *toweldayText = "Towel Day";

// Geometry constants
#ifdef PBL_ROUND
#define WEEKDAY_TOP_MARGIN 8
#define DATE_BOTTOM_MARGIN 12
#else
#define WEEKDAY_TOP_MARGIN -5
#define DATE_BOTTOM_MARGIN 0
#endif
#define WEEKDAY_HEIGHT 20
#define DATE_HEIGHT 20
#define YOLD_OFFSET -2
#define YOLD_WIDTH  38
#define YOLD_HEIGHT 20
#define MINUTE_DIAMETER 138
#define HOUR_DIAMETER 108
#define SECOND_DIAMETER 59

#ifdef PBL_COLOR
// Colors
// - colored weekday background
#define DEFAULT_BG_COLOR GColorBlack
#define SWEETMORN_BG_COLOR GColorBrilliantRose
#define BOOMTIME_BG_COLOR GColorRed
#define PUNGENDAY_BG_COLOR GColorIslamicGreen
#define PRICKLEPRICKLE_BG_COLOR GColorVividCerulean
#define SETTINGORANGE_BG_COLOR GColorOrange
#define STTIBSDAY_BG_COLOR GColorFashionMagenta
// - elements
#define DWDAY_TEXT_COLOR GColorWhite
#define MINUTE_P_COLOR GColorWhite
#define MINUTE_T_COLOR GColorBlack
#define HOUR_P_COLOR GColorDukeBlue
#define HOUR_T_COLOR GColorElectricBlue
#define SECOND_P_COLOR GColorChromeYellow
#define SECOND_TEXT_COLOR GColorBlack
#define DDATE_TEXT_COLOR GColorWhite
#else
// Monochrome
#define DEFAULT_BG_COLOR GColorBlack
#define DWDAY_TEXT_COLOR GColorWhite
#define MINUTE_P_COLOR GColorWhite
#define MINUTE_T_COLOR GColorBlack
#define HOUR_P_COLOR GColorBlack
#define HOUR_T_COLOR GColorWhite
#define SECOND_P_COLOR GColorWhite
#define SECOND_TEXT_COLOR GColorBlack
#define DDATE_TEXT_COLOR GColorWhite
#endif

// Variables

// - the window
Window *window = NULL;

// - text layers
TextLayer *dwday_textlayer = NULL;
TextLayer *ddate_textlayer = NULL;
TextLayer *yold_textlayer = NULL;
// - the texts
#define DWDAY_STR_BUFFER_BYTES 30
char dwday_str_buffer[DWDAY_STR_BUFFER_BYTES];
#define DDATE_STR_BUFFER_BYTES 40
char ddate_str_buffer[DDATE_STR_BUFFER_BYTES];
#define YOLD_STR_BUFFER_BYTES 6
char yold_str_buffer[YOLD_STR_BUFFER_BYTES];

// - graphic layers
Layer *hour_minute_layer = NULL;
Layer *second_layer = NULL;
// - paths
GPath *minutePentagon = NULL;
GPoint minutePentagonPoints[5];
GPath *minuteTriangle = NULL;
GPoint minuteTrianglePoints[3];
GPath *hourPentagon = NULL;
GPoint hourPentagonPoints[5];
GPath *hourTriangle = NULL;
GPoint hourTrianglePoints[3];
GPath *secondPentagon = NULL;
GPoint secondPentagonPoints[5];
// colors
#ifdef PBL_COLOR
GColor8 dweekdayColors[5];
#endif

// - internal variables
int currentSecHexAngle;
int currentMinHexAngle;
int currentHourHexAngle;
int dateValid;
int minuteValid;

// global path info temp var
GPathInfo pathInfo;

void calc_pentagon(int aDiameter, GPoint *pp)
{
  int r = aDiameter/2;
  // create points
  int hexAngle = 0;
  for (int i=0; i<5; i++) {
    pp[i].x = sin_lookup(hexAngle)*r/(long)TRIG_MAX_RATIO;
    pp[i].y = - cos_lookup(hexAngle)*r/(long)TRIG_MAX_RATIO;
    hexAngle += TRIG_MAX_ANGLE / 5;
  }
  pathInfo.num_points = 5;
  pathInfo.points = pp;
}

// morph already calculated pentagon to arrow
void morph_to_arrow(GPoint *pp, GPoint *tp)
{
  int halfbase = pp[1].x; // half base of sector triangle
  int pheight = pp[2].y-pp[0].y; // pentagon height
  int theight = pp[1].y-pp[0].y; // triangle height
  int ratio = (long)TRIG_MAX_RATIO*theight/pheight; // ratio to shrink
  // now adjust triangle points
  halfbase = halfbase*ratio/(long)TRIG_MAX_RATIO;
  tp[0] = pp[0];
  tp[1].y = pp[1].y;
  tp[1].x = +halfbase;
  tp[2].y = pp[4].y;
  tp[2].x = -halfbase;
  pathInfo.num_points = 3;
  pathInfo.points = tp;
}


void hour_minute_layer_callback(struct Layer *layer, GContext *ctx)
{
  // minute
  gpath_rotate_to(minutePentagon, currentMinHexAngle);
  graphics_context_set_fill_color(ctx, MINUTE_P_COLOR);
  gpath_draw_filled(ctx, minutePentagon);
  gpath_rotate_to(minuteTriangle, currentMinHexAngle);
  graphics_context_set_fill_color(ctx, MINUTE_T_COLOR);
  gpath_draw_filled(ctx, minuteTriangle);
  // hour
  gpath_rotate_to(hourPentagon, currentHourHexAngle);
  gpath_rotate_to(hourTriangle, currentHourHexAngle);
  graphics_context_set_fill_color(ctx, HOUR_P_COLOR);
  gpath_draw_filled(ctx, hourPentagon);
  graphics_context_set_fill_color(ctx, HOUR_T_COLOR);
  gpath_draw_filled(ctx, hourTriangle);
  #ifndef PBL_COLOR
  // - b&w: need outline of hour pentagon in minute pentagon color to visually separate from minute triangle
  graphics_context_set_stroke_color(ctx, MINUTE_P_COLOR);
  gpath_draw_outline(ctx, hourPentagon);
  #endif
}

void second_layer_callback(struct Layer *layer, GContext *ctx)
{
  // second
  gpath_rotate_to(secondPentagon, currentSecHexAngle);
  graphics_context_set_fill_color(ctx, SECOND_P_COLOR);
  gpath_draw_filled(ctx, secondPentagon);
  #ifndef PBL_COLOR
  // - b&w: need outline of second pentagon in hour pentagon color to visually separate from hour triangle
  graphics_context_set_stroke_color(ctx, HOUR_P_COLOR);
  gpath_draw_outline(ctx, secondPentagon);
  #endif
}


void update_hour_minute_angles(struct tm *tick_time)
{
  currentMinHexAngle = TRIG_MAX_ANGLE / 60 * tick_time->tm_min;
  currentHourHexAngle = TRIG_MAX_ANGLE / 60 * (tick_time->tm_hour*5 + tick_time->tm_min/12);
}

void update_second_angle(struct tm *tick_time)
{
  currentSecHexAngle = TRIG_MAX_ANGLE / 60 * tick_time->tm_sec;
}


// set this to 1 to quickly see all minute texts, one per second, to debug layout
// It also displays the LONGEST_HOUR_TEXT instead of the current hour
#define DEBUG_SECONDS_AS_MIN 0
#define DEBUG_SECONDS_AS_DAYS 0

// handle second tick
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed)
{
  #if DEBUG_SECONDS_AS_DAYS
  dateValid = 0;
  #else
  if (units_changed & DAY_UNIT) dateValid = 0; // new day, needs new text
  #endif
  if (!dateValid) {
    #if DEBUG_SECONDS_AS_DAYS
    int yearday = (tick_time->tm_sec+60*tick_time->tm_min) % 365; // seconds as days
    int tibsFlag = 0; // assume no leap
    #else
    int yearday = tick_time->tm_yday; // zero based 0..364 (365 in leap years)
    // leap year?
    int tibsFlag = (tick_time->tm_year%4==0) && (tick_time->tm_year!=2100-1900); // valid for 1600..2199 (2100 is not leap)
    #endif
    if (tibsFlag) {
      // leap year: could be St.Tibs itself
      if (yearday==59) {
        // Normally this would be 60 chaos, but in leap years it's St. Tib's day
      }
      else {
        tibsFlag=0; // is not St. Tib's
        // but days after
        if (yearday>59) yearday--; // compensate for inserted St.Tib's
      }
    }
    // St. Tib's day?
    if (tibsFlag) {
      // yes, weekday and date are both St. Tib's
      strcpy(dwday_str_buffer, sttibsdayText);
      strcpy(ddate_str_buffer, sttibsdayText);
      #ifdef PBL_COLOR
      window_set_background_color(window, STTIBSDAY_BG_COLOR);
      text_layer_set_background_color(dwday_textlayer, STTIBSDAY_BG_COLOR);
      text_layer_set_background_color(ddate_textlayer, STTIBSDAY_BG_COLOR);
      #endif
    }
    else {
      // regular day
      // - weekday
      int dwday = yearday % 5;
      strcpy(dwday_str_buffer, dweekdayTexts[dwday]);
      // - weekday color
      #ifdef PBL_COLOR
      window_set_background_color(window, dweekdayColors[dwday]);
      text_layer_set_background_color(dwday_textlayer, dweekdayColors[dwday]);
      text_layer_set_background_color(ddate_textlayer, dweekdayColors[dwday]);
      #endif
      // - date
      int season = yearday / 73;
      int daynum = yearday % 73 + 1;
      // - check holydays and flux days
      if (daynum==5) {
        // holy days
        strcpy(ddate_str_buffer, holydayTexts[season]);
      }
      else if (daynum==50) {
        // flux days
        strcpy(ddate_str_buffer, fluxdayTexts[season]);
      }
      else if (season==1 && daynum==72) {
        // towel day, Discord 72
        strcpy(ddate_str_buffer, toweldayText);
      }
      else {
        // completely regular day
        strcpy(ddate_str_buffer, seasonTexts[season]);
        int i = strlen(ddate_str_buffer);
        snprintf(ddate_str_buffer+i, DDATE_STR_BUFFER_BYTES-i," %d", daynum);
      }
    }
    text_layer_set_text(dwday_textlayer, dwday_str_buffer);
    text_layer_set_text(ddate_textlayer, ddate_str_buffer);
    // YOLD
    int yold = tick_time->tm_year+1900+1166; // 2015 AD == 3181 YOLD
    snprintf(yold_str_buffer, YOLD_STR_BUFFER_BYTES, "%04d", yold);
    text_layer_set_text(yold_textlayer, yold_str_buffer);
    // done
    dateValid = 1; // now valid
  }
  if (units_changed & MINUTE_UNIT) minuteValid = 0;
  if (!minuteValid) {
    // update minute and hour
    update_hour_minute_angles(tick_time);
    layer_mark_dirty(hour_minute_layer);
    // done
    minuteValid = 1; // now valid
  }
  // anyway, update second
  update_second_angle(tick_time);
  layer_mark_dirty(second_layer);
}



void init()
{
  #ifdef PBL_COLOR
  // the weekday colors
  dweekdayColors[0] = SWEETMORN_BG_COLOR;
  dweekdayColors[1] = BOOMTIME_BG_COLOR;
  dweekdayColors[2] = PUNGENDAY_BG_COLOR;
  dweekdayColors[3] = PRICKLEPRICKLE_BG_COLOR;
  dweekdayColors[4] = SETTINGORANGE_BG_COLOR;
  #endif

  // the window
  window = window_create();
  window_set_background_color(window, DEFAULT_BG_COLOR);
  window_stack_push(window, true /* Animated */);

  // init the engine
  dateValid = 0; // text not valid
  minuteValid = 0; // hands not valid
  struct tm *t;
  time_t now = time(NULL);
  t = localtime(&now);
  // start with correct hands positions
  update_second_angle(t);
  update_hour_minute_angles(t);

  // get the window frame
  GRect wf = layer_get_frame(window_get_root_layer(window));
  // the text layer for displaying the Weekday (top border)
  GRect tf = wf; // start calculation with window frame
  tf.origin.y += WEEKDAY_TOP_MARGIN;
  tf.size.h = WEEKDAY_HEIGHT;
  dwday_textlayer = text_layer_create(tf);
  text_layer_set_text_alignment(dwday_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(dwday_textlayer, DEFAULT_BG_COLOR); // like background
  text_layer_set_font(dwday_textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)); // font
  text_layer_set_text_color(dwday_textlayer, DWDAY_TEXT_COLOR); // text
  // the text layer for displaying the date (bottom border)
  tf = wf; // start calculation with window frame
  tf.origin.y += tf.size.h - DATE_HEIGHT - DATE_BOTTOM_MARGIN;
  tf.size.h = DATE_HEIGHT;
  ddate_textlayer = text_layer_create(tf);
  text_layer_set_text_alignment(ddate_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(ddate_textlayer, DEFAULT_BG_COLOR); // like background
  text_layer_set_font(ddate_textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)); // font
  text_layer_set_text_color(ddate_textlayer, DDATE_TEXT_COLOR); // text
  // the text layer for displaying the YOLD (center)
  tf = wf; // start calculation with window frame
  tf.origin.x += (tf.size.w-YOLD_WIDTH)/2;
  tf.size.w = YOLD_WIDTH;
  tf.origin.y += (tf.size.h-YOLD_HEIGHT)/2+YOLD_OFFSET;
  tf.size.h = YOLD_HEIGHT;
  yold_textlayer = text_layer_create(tf);
  text_layer_set_text_alignment(yold_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(yold_textlayer, SECOND_P_COLOR); // like second pentagon
  text_layer_set_font(yold_textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)); // font
  text_layer_set_text_color(yold_textlayer, SECOND_TEXT_COLOR); // black text
  // init text contents
  text_layer_set_text(dwday_textlayer, "dclock");
  text_layer_set_text(yold_textlayer, "");
  text_layer_set_text(dwday_textlayer, "dclock\n©2015\nplan44.ch");

  // graphic layer for hour and minute
  tf = wf; // start calculation with window frame
  tf.origin.x += (tf.size.w-MINUTE_DIAMETER)/2;
  tf.size.w = MINUTE_DIAMETER;
  tf.origin.y += (tf.size.h-MINUTE_DIAMETER)/2;
  tf.size.h = MINUTE_DIAMETER;
  hour_minute_layer = layer_create(tf);
  layer_set_update_proc(hour_minute_layer, &hour_minute_layer_callback);

  // - second
  tf = wf; // start calculation with window frame
  tf.origin.x += (tf.size.w-SECOND_DIAMETER)/2;
  tf.size.w = SECOND_DIAMETER;
  tf.origin.y += (tf.size.h-SECOND_DIAMETER)/2;
  tf.size.h = SECOND_DIAMETER;
  second_layer = layer_create(tf);
  layer_set_update_proc(second_layer, &second_layer_callback);


  // calculate the paths
  // - minute & hour
  GPoint mh_center = { x:MINUTE_DIAMETER/2, y:MINUTE_DIAMETER/2 };
  // - minute
  calc_pentagon(MINUTE_DIAMETER, minutePentagonPoints);
  minutePentagon = gpath_create(&pathInfo);
  morph_to_arrow(minutePentagonPoints, minuteTrianglePoints);
  minuteTriangle = gpath_create(&pathInfo);
  gpath_move_to(minutePentagon,mh_center);
  gpath_move_to(minuteTriangle,mh_center);
  // - hour
  calc_pentagon(HOUR_DIAMETER, hourPentagonPoints);
  hourPentagon = gpath_create(&pathInfo);
  morph_to_arrow(hourPentagonPoints, hourTrianglePoints);
  hourTriangle = gpath_create(&pathInfo);
  gpath_move_to(hourPentagon,mh_center);
  gpath_move_to(hourTriangle,mh_center);
  // - second
  GPoint s_center = { x:SECOND_DIAMETER/2, y:SECOND_DIAMETER/2 };
  calc_pentagon(SECOND_DIAMETER, secondPentagonPoints);
  secondPentagon = gpath_create(&pathInfo);
  gpath_move_to(secondPentagon,s_center);

  // add the layers
  // - top and bottom texts
  layer_add_child(window_get_root_layer(window), (Layer *)dwday_textlayer);
  layer_add_child(window_get_root_layer(window), (Layer *)ddate_textlayer);
  // - hour_minute
  layer_add_child(window_get_root_layer(window), hour_minute_layer);
  // - second
  layer_add_child(window_get_root_layer(window), second_layer);
  // - YOLD
  layer_add_child(window_get_root_layer(window), (Layer *)yold_textlayer);

  // trigger first update immediately
  layer_mark_dirty(hour_minute_layer);
  layer_mark_dirty(second_layer);

  // now subscribe to ticks
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
}


void deinit()
{
//   layer_destroy(second_display_layer);
  text_layer_destroy(yold_textlayer);
  text_layer_destroy(ddate_textlayer);
  text_layer_destroy(dwday_textlayer);
  layer_destroy(hour_minute_layer);
  layer_destroy(second_layer);
  gpath_destroy(minutePentagon);
  gpath_destroy(minuteTriangle);
  gpath_destroy(hourPentagon);
  gpath_destroy(hourTriangle);
  gpath_destroy(secondPentagon);
  window_destroy(window);
}



int main(void)
{
  init();
  app_event_loop();
  deinit();
  return 0;
}

