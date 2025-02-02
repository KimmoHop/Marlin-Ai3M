/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2016 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * status_screen_DOGM.h
 *
 * Standard Status Screen for Graphical Display
 */

#ifndef _STATUS_SCREEN_DOGM_H_
#define _STATUS_SCREEN_DOGM_H_

FORCE_INLINE void _draw_centered_temp(const int16_t temp, const uint8_t x, const uint8_t y) {
  const char * const str = itostr3(temp);
  u8g.setPrintPos(x - (str[0] != ' ' ? 0 : str[1] != ' ' ? 1 : 2) * DOG_CHAR_WIDTH / 2, y);
  lcd_print(str);
  lcd_printPGM(PSTR(LCD_STR_DEGREE " "));
}

#ifndef HEAT_INDICATOR_X
  #define HEAT_INDICATOR_X 8
#endif

FORCE_INLINE void _draw_heater_status(const uint8_t x, const int8_t heater, const bool blink) {
  #if !HEATER_IDLE_HANDLER
    UNUSED(blink);
  #endif

  #if HAS_HEATED_BED
    const bool isBed = heater < 0;
  #else
    constexpr bool isBed = false;
  #endif

  if (PAGE_UNDER(7)) {
    #if HEATER_IDLE_HANDLER
      const bool is_idle = (
        #if HAS_HEATED_BED
          isBed ? thermalManager.is_bed_idle() :
        #endif
        thermalManager.is_heater_idle(heater)
      );

      if (blink || !is_idle)
    #endif
        _draw_centered_temp(0.5f + (
            #if HAS_HEATED_BED
              isBed ? thermalManager.degTargetBed() :
            #endif
            thermalManager.degTargetHotend(heater)
          ), x, 7
        );
  }

  if (PAGE_CONTAINS(21, 28)) {
    _draw_centered_temp(0.5f + (
        #if HAS_HEATED_BED
          isBed ? thermalManager.degBed() :
        #endif
        thermalManager.degHotend(heater)
      ), x, 28
    );

    if (PAGE_CONTAINS(17, 20)) {
      const uint8_t h = isBed ? 7 : HEAT_INDICATOR_X,
                    y = isBed ? 18 : 17;
      if (
        #if HAS_HEATED_BED
          isBed ? thermalManager.isHeatingBed() :
        #endif
        thermalManager.isHeatingHotend(heater)
      ) {
        u8g.setColorIndex(0); // white on black
        u8g.drawBox(x + h, y, 2, 2);
        u8g.setColorIndex(1); // black on white
      }
      else
        u8g.drawBox(x + h, y, 2, 2);
    }
  }
}

//
// Before homing, blink '123' <-> '???'.
// Homed but unknown... '123' <-> '   '.
// Homed and known, display constantly.
//
FORCE_INLINE void _draw_axis_value(const AxisEnum axis, const char *value, const bool blink) {
  if (blink)
    lcd_print(value);
  else {
    if (!TEST(axis_homed, axis))
      while (const char c = *value++) lcd_print(c <= '.' ? c : '?');
    else {
      #if DISABLED(HOME_AFTER_DEACTIVATE) && DISABLED(DISABLE_REDUCED_ACCURACY_WARNING)
        if (!TEST(axis_known_position, axis))
          lcd_printPGM(axis == Z_AXIS ? PSTR("      ") : PSTR("    "));
        else
      #endif
          lcd_print(value);
    }
  }
}

inline void lcd_implementation_status_message(const bool blink) {
  #if ENABLED(STATUS_MESSAGE_SCROLLING)
    static bool last_blink = false;

    // Get the UTF8 character count of the string
    uint8_t slen = utf8_strlen(lcd_status_message);

    // If the string fits into the LCD, just print it and do not scroll it
    if (slen <= LCD_WIDTH) {

      // The string isn't scrolling and may not fill the screen
      lcd_print_utf(lcd_status_message);

      // Fill the rest with spaces
      while (slen < LCD_WIDTH) {
        u8g.print(' ');
        ++slen;
      }
    }
    else {
      // String is larger than the available space in screen.

      // Get a pointer to the next valid UTF8 character
      const char *stat = lcd_status_message + status_scroll_offset;

      // Get the string remaining length
      const uint8_t rlen = utf8_strlen(stat);

      // If we have enough characters to display
      if (rlen >= LCD_WIDTH) {
        // The remaining string fills the screen - Print it
        lcd_print_utf(stat, LCD_WIDTH);
      }
      else {
        // The remaining string does not completely fill the screen
        lcd_print_utf(stat, LCD_WIDTH);         // The string leaves space
        uint8_t chars = LCD_WIDTH - rlen;       // Amount of space left in characters

        u8g.print('.');                         // Always at 1+ spaces left, draw a dot
        if (--chars) {                          // Draw a second dot if there's space
          u8g.print('.');
          if (--chars)                          // Print a second copy of the message
            lcd_print_utf(lcd_status_message, LCD_WIDTH - (rlen + 2));
        }
      }
       if (last_blink != blink) {
         last_blink = blink;

        // Adjust by complete UTF8 characters
        if (status_scroll_offset < slen) {
          status_scroll_offset++;
          while (!START_OF_UTF8_CHAR(lcd_status_message[status_scroll_offset]))
            status_scroll_offset++;
        }
        else
          status_scroll_offset = 0;
      }
    }
  #else
    UNUSED(blink);

    // Get the UTF8 character count of the string
    uint8_t slen = utf8_strlen(lcd_status_message);

    // Just print the string to the LCD
    lcd_print_utf(lcd_status_message, LCD_WIDTH);

    // Fill the rest with spaces if there are missing spaces
    while (slen < LCD_WIDTH) {
      u8g.print(' ');
      ++slen;
    }
  #endif
}

#if ENABLED(FULL_M73_SUPPORT)
inline uint16_t print_time_to_change_remaining() {
  uint16_t print_t = print_time_to_change;
  if (feedrate_percentage != 0) print_t = 100ul * print_t / feedrate_percentage;
  return print_t;
}

inline uint16_t print_time_remaining() {
  uint16_t print_t = print_time_remaining_normal;
  if (feedrate_percentage != 0) print_t = 100ul * print_t / feedrate_percentage;
  return print_t;
}
#endif

static void lcd_implementation_status_screen() {

  const bool blink = lcd_blink();

  #if FAN_ANIM_FRAMES > 2
    static bool old_blink;
    static uint8_t fan_frame;
    if (old_blink != blink) {
      old_blink = blink;
      if (!fanSpeeds[0] || ++fan_frame >= FAN_ANIM_FRAMES) fan_frame = 0;
    }
  #endif

  // Status Menu Font
  lcd_setFont(FONT_STATUSMENU);

  //
  // Fan Animation
  //
  // Draws the whole heading image as a B/W bitmap rather than
  // drawing the elements separately.
  // This was done as an optimization, as it was slower to draw
  // multiple parts compared to a single bitmap.
  //
  // The bitmap:
  // - May be offset in X
  // - Includes all nozzle(s), bed(s), and the fan.
  //
  // TODO:
  //
  // - Only draw the whole header on the first
  //   entry to the status screen. Nozzle, bed, and
  //   fan outline bits don't change.
  //
  if (PAGE_UNDER(STATUS_SCREENHEIGHT + 1)) {

    u8g.drawBitmapP(
      STATUS_SCREEN_X, STATUS_SCREEN_Y,
      (STATUS_SCREENWIDTH + 7) / 8, STATUS_SCREENHEIGHT,
      #if HAS_FAN0
        #if FAN_ANIM_FRAMES > 2
          fan_frame == 1 ? status_screen1_bmp :
          fan_frame == 2 ? status_screen2_bmp :
          #if FAN_ANIM_FRAMES > 3
            fan_frame == 3 ? status_screen3_bmp :
          #endif
        #else
          blink && fanSpeeds[0] ? status_screen1_bmp :
        #endif
      #endif
      status_screen0_bmp
    );

  }

  //
  // Temperature Graphics and Info
  //

  if (PAGE_UNDER(28)) {
    // Extruders
    HOTEND_LOOP() _draw_heater_status(STATUS_SCREEN_HOTEND_TEXT_X(e), e, blink);

    // Heated bed
    #if HOTENDS < 4 && HAS_HEATED_BED
      _draw_heater_status(STATUS_SCREEN_BED_TEXT_X, -1, blink);
    #endif

    #if HAS_FAN0
      if (PAGE_CONTAINS(STATUS_SCREEN_FAN_TEXT_Y - 7, STATUS_SCREEN_FAN_TEXT_Y)) {
        // Fan
        const int16_t per = ((fanSpeeds[0] + 1) * 100) / 256;
        if (per) {
          u8g.setPrintPos(STATUS_SCREEN_FAN_TEXT_X, STATUS_SCREEN_FAN_TEXT_Y);
          lcd_print(itostr3(per));
          u8g.print('%');
        }
      }
    #endif
  }

  #if ENABLED(SDSUPPORT)
    //
    // SD Card Symbol
    //
    if (card.isFileOpen() && PAGE_CONTAINS(42 - (TALL_FONT_CORRECTION), 51 - (TALL_FONT_CORRECTION))) {
      // Upper box
      u8g.drawBox(42, 42 - (TALL_FONT_CORRECTION), 8, 7);     // 42-48 (or 41-47)
      // Right edge
      u8g.drawBox(50, 44 - (TALL_FONT_CORRECTION), 2, 5);     // 44-48 (or 43-47)
      // Bottom hollow box
      u8g.drawFrame(42, 49 - (TALL_FONT_CORRECTION), 10, 4);  // 49-52 (or 48-51)
      // Corner pixel
      u8g.drawPixel(50, 43 - (TALL_FONT_CORRECTION));         // 43 (or 42)
    }
  #endif // SDSUPPORT

  #if (ENABLED(SDSUPPORT) || ENABLED(LCD_SET_PROGRESS_MANUALLY)) && DISABLED(FULL_M73_SUPPORT)
    //
    // Progress bar frame
    //
    #define PROGRESS_BAR_X 54
    #define PROGRESS_BAR_WIDTH (LCD_PIXEL_WIDTH - PROGRESS_BAR_X)

    if (PAGE_CONTAINS(49, 52 - (TALL_FONT_CORRECTION)))       // 49-52 (or 49-51)
      u8g.drawFrame(
        PROGRESS_BAR_X, 49,
        PROGRESS_BAR_WIDTH, 4 - (TALL_FONT_CORRECTION)
      );

    #if DISABLED(LCD_SET_PROGRESS_MANUALLY)
      const uint8_t progress_bar_percent = card.percentDone();
    #endif

    if (progress_bar_percent > 1) {

      //
      // Progress bar solid part
      //

      if (PAGE_CONTAINS(50, 51 - (TALL_FONT_CORRECTION)))     // 50-51 (or just 50)
        u8g.drawBox(
          PROGRESS_BAR_X + 1, 50,
          (uint16_t)((PROGRESS_BAR_WIDTH - 2) * progress_bar_percent * 0.01), 2 - (TALL_FONT_CORRECTION)
        );

      //
      // SD Percent Complete
      //

      #if ENABLED(DOGM_SD_PERCENT)
        if (PAGE_CONTAINS(41, 48)) {
          // Percent complete
          u8g.setPrintPos(55, 48);
          u8g.print(itostr3(progress_bar_percent));
          u8g.print('%');
        }
      #endif
    }

    //
    // Elapsed Time
    //

    #if DISABLED(DOGM_SD_PERCENT)
      #define SD_DURATION_X (PROGRESS_BAR_X + (PROGRESS_BAR_WIDTH / 2) - len * (DOG_CHAR_WIDTH / 2))
    #else
      #define SD_DURATION_X (LCD_PIXEL_WIDTH - len * DOG_CHAR_WIDTH)
    #endif

    if (PAGE_CONTAINS(41, 48)) {
      char buffer[10];
      duration_t elapsed = print_job_timer.duration();
      bool has_days = (elapsed.value >= 60*60*24L);
      uint8_t len = elapsed.toDigital(buffer, has_days);
      u8g.setPrintPos(SD_DURATION_X, 48);
      lcd_print(buffer);
    }

  #endif // SDSUPPORT || LCD_SET_PROGRESS_MANUALLY
  
  #if ENABLED(FULL_M73_SUPPORT)
    #define PROGRESS_NUMBER_X 55
    #define SD_TIME_X 96

    if (PAGE_CONTAINS(42, 49)) {

      uint8_t progress = PRINT_PERCENT_DONE_INIT;
      if (print_percent_done_normal != PRINT_PERCENT_DONE_INIT) {
        progress = print_percent_done_normal;
      } else if (IS_SD_PRINTING()) {
        progress = card.percentDone();
      }
      if (progress != PRINT_PERCENT_DONE_INIT) {
        u8g.setPrintPos(PROGRESS_NUMBER_X, 49);
          u8g.print(itostr3(progress));
          u8g.print('%');
      }

       char buffer[10];
      duration_t elapsed = print_job_timer.duration();
      bool has_days = (elapsed.value >= 60*60*24L);
      uint8_t len = elapsed.toDigital(buffer, has_days);
      u8g.setPrintPos(SD_TIME_X, 49);
      lcd_print(buffer);
    }
  #endif // FULL_M73_SUPPORT

  //
  // XYZ Coordinates
  //

  #define XYZ_BASELINE (30 + INFO_FONT_HEIGHT)

  #define X_LABEL_POS  3
  #define X_VALUE_POS 11
  #define XYZ_SPACING 40

  #if ENABLED(XYZ_HOLLOW_FRAME)
    #define XYZ_FRAME_TOP 29
    #define XYZ_FRAME_HEIGHT INFO_FONT_HEIGHT + 3
  #else
    #define XYZ_FRAME_TOP 30
    #define XYZ_FRAME_HEIGHT INFO_FONT_HEIGHT + 1
  #endif

  static char xstring[5], ystring[5], zstring[7];
  #if ENABLED(FILAMENT_LCD_DISPLAY)
    static char wstring[5], mstring[4];
  #endif

  // At the first page, regenerate the XYZ strings
  if (page.page == 0) {
    strcpy(xstring, ftostr4sign(LOGICAL_X_POSITION(current_position[X_AXIS])));
    strcpy(ystring, ftostr4sign(LOGICAL_Y_POSITION(current_position[Y_AXIS])));
    strcpy(zstring, ftostr52sp(LOGICAL_Z_POSITION(current_position[Z_AXIS])));
    #if ENABLED(FILAMENT_LCD_DISPLAY)
      strcpy(wstring, ftostr12ns(filament_width_meas));
      strcpy(mstring, itostr3(100.0 * (
          parser.volumetric_enabled
            ? planner.volumetric_area_nominal / planner.volumetric_multiplier[FILAMENT_SENSOR_EXTRUDER_NUM]
            : planner.volumetric_multiplier[FILAMENT_SENSOR_EXTRUDER_NUM]
        )
      ));
    #endif
  }

  if (PAGE_CONTAINS(XYZ_FRAME_TOP, XYZ_FRAME_TOP + XYZ_FRAME_HEIGHT - 1)) {

    #if ENABLED(XYZ_HOLLOW_FRAME)
      u8g.drawFrame(0, XYZ_FRAME_TOP, LCD_PIXEL_WIDTH, XYZ_FRAME_HEIGHT); // 8: 29-40  7: 29-39
    #else
      u8g.drawBox(0, XYZ_FRAME_TOP, LCD_PIXEL_WIDTH, XYZ_FRAME_HEIGHT);   // 8: 30-39  7: 30-37
    #endif

    if (PAGE_CONTAINS(XYZ_BASELINE - (INFO_FONT_HEIGHT - 1), XYZ_BASELINE)) {

      #if DISABLED(XYZ_HOLLOW_FRAME)
        u8g.setColorIndex(0); // white on black
      #endif

      #if ENABLED(FULL_M73_SUPPORT)
        uint16_t print_t = PRINT_TIME_REMAINING_INIT;
        char symbol = LCD_STR_CLOCK[0];
        char suff_doubt = feedrate_percentage != 100 ? '?': ' ';
        if(print_time_to_change != PRINT_TIME_REMAINING_INIT) {
          print_t = print_time_to_change_remaining();
          symbol = LCD_STR_ARROW_RIGHT[0];
        } else if (print_time_remaining_normal != PRINT_TIME_REMAINING_INIT) {
          print_t = print_time_remaining();
        }
        
        if (IsRunning() && print_t != PRINT_TIME_REMAINING_INIT) {
          u8g.setPrintPos(0 * XYZ_SPACING + X_LABEL_POS, XYZ_BASELINE);

          char buffer[15];
          uint16_t hh = print_t / 60;
          uint8_t mm = print_t % 60;

          if (hh > 99) {
            sprintf_P(buffer, PSTR("%c    %3uh %c"), symbol, hh, suff_doubt); // max 11
          } else {
            sprintf_P(buffer, PSTR("%c %2uh %02um %c"), symbol, hh, mm, suff_doubt); // max 11
          }
          lcd_print(buffer);
        } else {

      #endif

        u8g.setPrintPos(0 * XYZ_SPACING + X_LABEL_POS, XYZ_BASELINE);
        lcd_printPGM(PSTR(MSG_X));
        u8g.setPrintPos(0 * XYZ_SPACING + X_VALUE_POS, XYZ_BASELINE);
        _draw_axis_value(X_AXIS, xstring, blink);

        u8g.setPrintPos(1 * XYZ_SPACING + X_LABEL_POS, XYZ_BASELINE);
        lcd_printPGM(PSTR(MSG_Y));
        u8g.setPrintPos(1 * XYZ_SPACING + X_VALUE_POS, XYZ_BASELINE);
        _draw_axis_value(Y_AXIS, ystring, blink);

      #if ENABLED(FULL_M73_SUPPORT)
        }
      #endif

      u8g.setPrintPos(2 * XYZ_SPACING + X_LABEL_POS, XYZ_BASELINE);
      lcd_printPGM(PSTR(MSG_Z));
      u8g.setPrintPos(2 * XYZ_SPACING + X_VALUE_POS, XYZ_BASELINE);
      _draw_axis_value(Z_AXIS, zstring, blink);

      #if DISABLED(XYZ_HOLLOW_FRAME)
        u8g.setColorIndex(1); // black on white
      #endif
    }
  }

  //
  // Feedrate
  //

  if (PAGE_CONTAINS(51 - INFO_FONT_HEIGHT, 49)) {
    lcd_setFont(FONT_MENU);
    u8g.setPrintPos(3, 50);
    lcd_print(LCD_STR_FEEDRATE[0]);

    lcd_setFont(FONT_STATUSMENU);
    u8g.setPrintPos(12, 50);
    lcd_print(itostr3(feedrate_percentage));
    u8g.print('%');

    //
    // Filament sensor display if SD is disabled
    //
    #if ENABLED(FILAMENT_LCD_DISPLAY) && DISABLED(SDSUPPORT)
      u8g.setPrintPos(56, 50);
      lcd_print(wstring);
      u8g.setPrintPos(102, 50);
      lcd_print(mstring);
      u8g.print('%');
      lcd_setFont(FONT_MENU);
      u8g.setPrintPos(47, 50);
      lcd_print(LCD_STR_FILAM_DIA);
      u8g.setPrintPos(93, 50);
      lcd_print(LCD_STR_FILAM_MUL);
    #endif
  }

  //
  // Status line
  //

  #define STATUS_BASELINE (55 + INFO_FONT_HEIGHT)

  if (PAGE_CONTAINS(STATUS_BASELINE - (INFO_FONT_HEIGHT - 1), STATUS_BASELINE)) {
    u8g.setPrintPos(0, STATUS_BASELINE);

    #if ENABLED(FILAMENT_LCD_DISPLAY) && ENABLED(SDSUPPORT)
      if (PENDING(millis(), previous_lcd_status_ms + 5000UL)) {  //Display both Status message line and Filament display on the last line
        lcd_implementation_status_message(blink);
      }
      else {
        lcd_printPGM(PSTR(LCD_STR_FILAM_DIA));
        u8g.print(':');
        lcd_print(wstring);
        lcd_printPGM(PSTR("  " LCD_STR_FILAM_MUL));
        u8g.print(':');
        lcd_print(mstring);
        u8g.print('%');
      }
    #else
      lcd_implementation_status_message(blink);
    #endif
  }
}

#endif // _STATUS_SCREEN_DOGM_H_
