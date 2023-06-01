/*!
 * @file /ui.c
 * @brief Simpio User Interface
 * @details
 * Sets up windows, handles keystrokes, and provides timeline dialog & window, 
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

  
#include "ui.h"
#include "editor.h"

static FILE* pio_pgm;
static editor_t editor;

static ui_user_functions_t * ui_user_functions;

static char buff[100];

WINDOW *src_win, *regs_win, *status_win;

#define ERROR_MSG_MAX 100
static  char error_msg[ERROR_MSG_MAX];
#define bailout(...)  {snprintf(error_msg, ERROR_MSG_MAX, __VA_ARGS__); goto _bailout;}

typedef enum { debug_mode, timeline_parameter_mode, timeline_window_mode } mode_e;

static mode_e current_mode = debug_mode;

void ui_enter_run_break_mode() { nodelay(editor.window, TRUE); }
void ui_exit_run_break_mode()  { nodelay(editor.window, FALSE); }
bool ui_break_check() {
    char ch = wgetch(editor.window);
    if (ch == 'b' || ch == 'B') return true;
    else return false;
}


/**********************************************************************************
 * 
 **********************************************************************************/

void ui_process_char(editor_t *ed, int ch);

void ui_run(ui_user_functions_t * user_functions) {
  int max_x, max_y;
  int ch;
  
  ui_user_functions = user_functions;
  
  slk_init(0);
  initscr();
  //slk_set(1, "F1=help", 0);
  slk_set(1, "F2=save", 0);
  slk_set(2, "F3=quit", 0);
  slk_set(3, "F4=build", 0);
  slk_set(4, "F5=run", 0);
  slk_set(5, "F6=step", 0);
  slk_set(6, "F7=brpt", 0);
  slk_set(7, "F8=setTL", 0);
  slk_set(8, "F9=TL", 0);
  slk_refresh();
  refresh();

  if (!has_colors() || (start_color() != OK)) bailout("Need terminal with colors\n");
  init_pair(1, COLOR_WHITE, COLOR_BLUE);
  init_pair(2, COLOR_WHITE, COLOR_GREEN);
  init_pair(3, COLOR_WHITE, COLOR_BLACK);
  init_pair(4, COLOR_GREEN, COLOR_BLACK);
    
                    
  getmaxyx(stdscr, max_y, max_x);
  
  src_win = newwin(SRC_LINES, SRC_COLS, SRC_START_Y, SRC_START_X);
  regs_win = newwin(REGS_LINES, REGS_COLS, REGS_START_Y, REGS_START_X);
  status_win = newwin(STATUS_LINES, STATUS_COLS, STATUS_START_Y, STATUS_START_X);
  scrollok(status_win, TRUE);
    
  if (!src_win || !regs_win || !status_win) bailout("failed to create window\n");
  
  if (scrollok(src_win, TRUE) == ERR) bailout("unable to enable scrolling");
                  
  wbkgd(src_win, COLOR_PAIR(1));
  wbkgd(regs_win, COLOR_PAIR(2));
  wbkgd(status_win, COLOR_PAIR(3));

  //mvwaddstr(src_win, 0, 0, "src window\n"); 
  mvwaddstr(regs_win, 0, 0, "regs window\n"); 
  mvwaddstr(status_win, 0, 0, "status window\n");
  
  //sprintf(buff, "lines=%d, cols=%d, status_win, start_y=%d, start_x=%d\n", SRC_LINES, SRC_COLS, SRC_START_Y, SRC_START_X);
  //mvwaddstr(src_win, 1, 0, buff);
  sprintf(buff, "lines=%d, cols=%d, start_y=%d, start_x=%d\n", REGS_LINES, REGS_COLS, REGS_START_Y, REGS_START_X);
  mvwaddstr(regs_win, 1, 0, buff);
  sprintf(buff, "lines=%d, cols=%d, start_y=%d, start_x=%d\n", STATUS_LINES, STATUS_COLS, STATUS_START_Y, STATUS_START_X);
  mvwaddstr(status_win, 1, 0, buff);
                    
  wrefresh(src_win);
  wrefresh(regs_win);
  wrefresh(status_win);
  
  pio_pgm = fopen(user_functions->filename, "r");
  if (!ed_init(&editor, pio_pgm, src_win, COLOR_PAIR(4))) {
    printf("unable to open file %s", user_functions->filename);
    exit(-2);
  }
    
  //TODO: revisit this always save when first loaded behavior when no longer just saving to a temp file
  int rc;
  rc = (*user_functions->save_function)(editor.buffer);
  if (rc < 0) {status_msg("auto-save successful; ready to build\n");}
  else {
    status_msg("auto-save failed; build won't be on this program! (%d)\n", rc);
  }
   
  wrefresh(src_win);
  noecho();
  keypad(stdscr, TRUE);
  ed_display_refresh(&editor);

  ch = getch();
  while (ch != 'q' && ch != 'Q') {
    ui_process_char(&editor, ch);
    ed_display_refresh(&editor);
    ch = getch();
  }
  endwin();
  return;

_bailout:
  endwin();
  printf("ABORT: %s", error_msg);
  return;

}
  
/**********************************************************************************
 * command processing
 **********************************************************************************/

void ui_process_char(editor_t *ed, int ch) {
  int rc;
  switch (ch) {
    case KEY_UP:
      ed_up(ed);
      break;
    case KEY_DOWN:
      ed_down(ed);
      break;
    case KEY_LEFT:
      ed_left(ed);
      break;
    case KEY_RIGHT:
      ed_right(ed);
      break;
    case KEY_HOME:
      ed_home(ed);
      break;
    case KEY_DC:
      ed_del_char(ed);
      break;
    case KEY_BACKSPACE:
      ed_left(ed);
      ed_del_char(ed);
      break;
    case KEY_NPAGE:
      ed_page_down(ed);
      break;
    case KEY_PPAGE:
      ed_left(ed);
      ed_page_up(ed);
      break;
    case KEY_F(1):
      status_msg("F1\n");
      break;
    case KEY_F(2):
      rc = (*(ui_user_functions->save_function))(ed->buffer);
      if (rc < 0) {status_msg("save successful\n");}
      else {
        status_msg("save failed (%d)\n", rc);
      }
      break;
    case KEY_F(3):
      status_msg("F3\n");
      break;    
    case KEY_F(4):
      rc = (*(ui_user_functions->build_compile_function))(ed->buffer);
      if (rc < 0) {status_msg("build successful\n");}
      else {
        status_msg("error at line %d\n", rc);
        ed_goto_line(ed, rc);
      }
      break;
    case KEY_F(5):
      rc = (*(ui_user_functions->run_function))();
      ed_goto_line(ed, rc);
      break;
    case KEY_F(6):
      rc = (*(ui_user_functions->step_function))();
      ed_goto_line(ed, rc);
      break;
    case KEY_F(7):
      rc = (*ui_user_functions->toggle_function)(ed->current_line+1); //TODO: reconcile editor starts lines at 0 vs rest which start at line 1
      ed_display(ed);
      /* editor will pick up the new breakpoint state and display appropriately  */
      break;
    case KEY_F(8):
      current_mode = timeline_parameter_mode;
      (*ui_user_functions->get_timeline_params_function) ();
      current_mode = debug_mode;
      ed_display(ed);
      break;
    case KEY_F(9):
      current_mode = timeline_window_mode;
      (*ui_user_functions->show_timeline_function)();
      current_mode = debug_mode;
      ed_display(ed);
      /* editor will pick up the new breakpoint state and display appropriately  */
      break;
    default:
      switch (current_mode) {
          case timeline_parameter_mode:
              break;
          case timeline_window_mode:
              break;
          case debug_mode:
          default:
              ed_insert_char(ed, ch);
      };
      
      break;
  }
}

WINDOW *timeline_dialog, *timeline_window;

static int timeline_dialog_input_positions[5][2] = { { 3, 9 }, { 5, 9 }, { 7, 9 }, { 9, 9 } , { 11, 9 }  };
static int timeline_dialog_field_num;

static ui_timeline_dialog_data_t timeline_dialog_data;

static int* timeline_tab() {
 if (++timeline_dialog_field_num >= TIMELINE_DIALOG_NUM_FIELDS) timeline_dialog_field_num = 0;
    return timeline_dialog_input_positions[timeline_dialog_field_num];
}

ui_timeline_dialog_data_t * ui_show_timeline_dialog() {
  int max_x, max_y;
  int ch, i, y, x;
  int* current_field;
  bool done;
    
    timeline_dialog_field_num = 0;
    for (i=0; i<TIMELINE_DIALOG_NUM_FIELDS; i++) {
        timeline_dialog_data.gpio_set[i] = false;
        timeline_dialog_data.gpio_values[i] = 0;
    }
    i=0;
    getmaxyx(stdscr, max_y, max_x);
    timeline_dialog = newwin(SRC_LINES, SRC_COLS, SRC_START_Y, SRC_START_X);
    mvwaddstr(timeline_dialog, 0, 0, "ENTER TIMELINE PARAMETERS (q to cancel)\n"); 
    noecho();
    keypad(stdscr, TRUE);
    
    mvwprintw(timeline_dialog, 3,1,"GPIO 1: ");
    mvwprintw(timeline_dialog, 5,1,"GPIO 2: ");
    mvwprintw(timeline_dialog, 7,1,"GPIO 3: ");
    mvwprintw(timeline_dialog, 9,1,"GPIO 4: ");
    mvwprintw(timeline_dialog, 11,1,"GPIO 5: ");
    
    wmove(timeline_dialog, 3, 9);

    wrefresh(timeline_dialog);

    ch = getch();
    done = false;
    while (ch != 'q' && ch != 'Q' && !done  ) {
      switch (ch) {
          case '\t':
              current_field = timeline_tab();
              i=timeline_dialog_data.gpio_values[timeline_dialog_field_num];
              wmove(timeline_dialog, current_field[0], current_field[1]);
              wrefresh(timeline_dialog);
              break;
          case '\n':
              mvwprintw(timeline_dialog, 0,0, "ENTERING THESE GPIOS TO TRACE: ");
              for (i=0; i < TIMELINE_DIALOG_NUM_FIELDS; i++) {
                  if (timeline_dialog_data.gpio_set[i]) wprintw(timeline_dialog, "%d ",  timeline_dialog_data.gpio_values[i]);
                  else wprintw(timeline_dialog, "X ");
              }
              wrefresh(timeline_dialog);
              done = true;
              break;
          case KEY_BACKSPACE:
              i=timeline_dialog_data.gpio_values[timeline_dialog_field_num];
              getyx(timeline_dialog, y, x);
              mvwprintw(timeline_dialog, 1,0,"                               ", i);
              i = i / 10;
              timeline_dialog_data.gpio_values[timeline_dialog_field_num] = i;
              wmove(timeline_dialog, y,x-1);
              wdelch(timeline_dialog);
              wrefresh(timeline_dialog);
              break;
          default:
              if (48 <= ch && ch <=57) {
                  wprintw(timeline_dialog, "%1d", ch-48);
                  timeline_dialog_data.gpio_set[timeline_dialog_field_num] = true;
                  i = i *10 + (ch-48);
                  if (i >=32 ) {
                      getyx(timeline_dialog, y, x);
                      mvwprintw(timeline_dialog, 1,0,"invalid GPIO number: %d", i);
                      wmove(timeline_dialog, y,x);
                      break;
                  }
                  else {
                      timeline_dialog_data.gpio_values[timeline_dialog_field_num] = i;
                  }
              }
              else printw("?");
      };
      wrefresh(timeline_dialog);
     ch = getch();
    }
    endwin();
    return &timeline_dialog_data;
}

// Timeline Stuff

/*
    A timeline is the series of up and down squiggles that show GPIO high/low values over time.
    Each value in the series is represented by a glyph on the screen. Each glyph is a 2x2 matrix
    of characters using ncurses. Generally, two side-by-side values are needed to determine if
    a transition glyph is needed or a constant hi/low value. To help keep things straight, the
    following mnemonic is used: 
    - the first value is A and the second value is B in a series
    - the first column is 1 and the second column is 2
    - the upper row is U and the lower row is L
    so for example, A2U is the first value, second column, upper. Each of these things have
    and x,y coordinate and a character value.
    Also, generally, only the middle columns (A2 and B1) are filled in. The first column of A 
    and the second column of B are filled in by the previous and next iterations of the procedure.
*/

#define LO2UP_CHAR ACS_LRCORNER
#define UP2HI_CHAR ACS_ULCORNER
#define HI2DOWN_CHAR ACS_URCORNER
#define DOWN2LO_CHAR ACS_LLCORNER
#define HIORLO_CHAR ACS_HLINE
#define BLANK_CHAR ' '
#define TICK_MARKER_CHAR '^'

typedef struct {
    int x;
    int y;
    uint32_t ch;
} timeline_glyph_t;

static timeline_glyph_t A2L, A2U, B1L, B1U;
static timeline_glyph_t A1L, A1U, B2L, B2U;

static int max_timeline_x, max_timeline_y;

#define TIMELINE_LEFT_MARGIN 10
#define ROWS_PER_TIMELINE 3
#define TIMELINE_HEADER 4
#define CHARS_PER_PLACE 2
#define TICK_ROW 2

void ui_draw_timeline_glyph(timeline_glyph_t * g) {
    wmove(timeline_window, g->y, g->x);
    //wprintw(timeline_window, "<%d>", g->ch);
    waddch(timeline_window, g->ch);
}

void ui_draw_timeline_tick(uint32_t place, uint32_t tick) {
    int y = TIMELINE_DIALOG_NUM_FIELDS * ROWS_PER_TIMELINE + TIMELINE_HEADER + TICK_ROW;
    int x = place * CHARS_PER_PLACE + TIMELINE_LEFT_MARGIN;;
    int i;
    char tick_str[4];
    wmove(timeline_window,y-2,x);
    waddch(timeline_window, TICK_MARKER_CHAR);
    wmove(timeline_window,y-1,x);
    waddch(timeline_window, '|');
    snprintf(tick_str, 4, "%03d",tick);
    for (i=0; i<3; i++) {
        wmove(timeline_window,y+i,x);
        waddch(timeline_window, tick_str[i]);
    }
    //wprintw(timeline_window,"%2d",place);
}

void ui_draw_timeline_first(int gpio, bool value) {
    A1U.y = gpio * ROWS_PER_TIMELINE + TIMELINE_HEADER;
    A1L.y = gpio * ROWS_PER_TIMELINE + TIMELINE_HEADER + 1;
    A1U.x = A1L.x = TIMELINE_LEFT_MARGIN;
    if (value) {
        A1U.ch = HIORLO_CHAR;
        A1L.ch = BLANK_CHAR;
    }
    else {
        A1U.ch = BLANK_CHAR;
        A1L.ch = HIORLO_CHAR;
    }
    ui_draw_timeline_glyph(&A1U);
    ui_draw_timeline_glyph(&A1L);
}

void ui_draw_timeline_last(int gpio, int last_place, bool value) {
    B2U.y = gpio * ROWS_PER_TIMELINE + TIMELINE_HEADER;
    B2L.y = gpio * ROWS_PER_TIMELINE + TIMELINE_HEADER + 1;
    B2U.x = B2L.x = last_place * CHARS_PER_PLACE -1 + TIMELINE_LEFT_MARGIN;
    if (value) {
        B2U.ch = HIORLO_CHAR;
        B2L.ch = BLANK_CHAR;
    }
    else {
        B2U.ch = BLANK_CHAR;
        B2L.ch = HIORLO_CHAR;
    }
    ui_draw_timeline_glyph(&B2U);
    ui_draw_timeline_glyph(&B2L);
}

void ui_draw_timeline_value_with_prev(int gpio, int place, bool value, bool prev_value) {
    A2U.y = B1U.y = gpio * ROWS_PER_TIMELINE + TIMELINE_HEADER;
    A2L.y = B1L.y = gpio * ROWS_PER_TIMELINE + TIMELINE_HEADER + 1;
    A2U.x = A2L.x = place * CHARS_PER_PLACE + TIMELINE_LEFT_MARGIN - 1;
    B1U.x = B1L.x = place * CHARS_PER_PLACE + TIMELINE_LEFT_MARGIN;
    if (prev_value != value) {
        if (value) {
             // draw transition zero to one
            A2U.ch = UP2HI_CHAR;
            A2L.ch = LO2UP_CHAR;
            B1U.ch = HIORLO_CHAR;
            B1L.ch = BLANK_CHAR;
        }
        else {
            // draw transiton one to zero
            A2U.ch = HI2DOWN_CHAR;
            A2L.ch = DOWN2LO_CHAR;
            B1U.ch = BLANK_CHAR;
            B1L.ch = HIORLO_CHAR;
        }
    }
    else {
        if (value) {
            // draw high
            A2U.ch = HIORLO_CHAR;
            A2L.ch = BLANK_CHAR;
            B1U.ch = HIORLO_CHAR;
            B1L.ch = BLANK_CHAR;
        }
        else {
            //draw low
            A2U.ch = BLANK_CHAR;
            A2L.ch = HIORLO_CHAR;
            B1U.ch = BLANK_CHAR;
            B1L.ch = HIORLO_CHAR;
        }
    }
    ui_draw_timeline_glyph(&A2U);
    ui_draw_timeline_glyph(&A2L);
    ui_draw_timeline_glyph(&B1U);
    ui_draw_timeline_glyph(&B1L);

}

static void test_timeline(int last_place) {
    int gpio, index, y, x, i;
    bool value, prev_value;
    for (gpio = 0; gpio < 5; gpio++) {
        if (gpio % 2 == 0) value = true;
        else value = false;
        ui_draw_timeline_first(gpio, value);
        prev_value = value;
        for (index = 1; index < last_place; index++) {
            if (gpio % 2 == 0) {
              if (((gpio + index) % 4) == 0) value = true;  
              else value = false;
            }
            else {
              if (((gpio + index) % 4) == 0) value = false;
              else value = true;
            }
            ui_draw_timeline_value_with_prev(gpio, index, value, prev_value);
            prev_value = value;
        }
        ui_draw_timeline_last(gpio, last_place, value);
    }
    for (index = 0; index < last_place; index++) {
        ui_draw_timeline_tick(4, index);
    }
}


static void show_timeline(ui_timeline_display_data_t * data) {
    uint8_t gpio;
    uint32_t place;
    bool prev_values[TIMELINE_DIALOG_NUM_FIELDS];
    uint32_t ticks;   
    ui_gpio_history_t * gpio_history;
    
    // do the first one
    gpio_history = data->callback();
    if (!gpio_history) {
        status_msg("no gpio history, no timeline to display\n");
        return;
    }
    ui_draw_timeline_tick(0, gpio_history->clock_tick);
    for (gpio = 0; gpio < TIMELINE_DIALOG_NUM_FIELDS; gpio++) {
       if (data->to_be_displayed[gpio]) {
           ui_draw_timeline_first(gpio, gpio_history->values[gpio]);
           prev_values[gpio] = gpio_history->values[gpio];
       }
    }
    // do the ones with prev
    for (place = 1; place < data->num_places; place++) {
        gpio_history = data->callback();
        if (!gpio_history) return;
        ui_draw_timeline_tick(place, gpio_history->clock_tick);
        for (gpio = 0; gpio < TIMELINE_DIALOG_NUM_FIELDS; gpio++) {
           if (data->to_be_displayed[gpio]) {
               ui_draw_timeline_value_with_prev(gpio, place, gpio_history->values[gpio], prev_values[gpio]);
               prev_values[gpio] = gpio_history->values[gpio];
           }
        }
    }
    // do the last one
    for (gpio = 0; gpio < TIMELINE_DIALOG_NUM_FIELDS; gpio++) {
        if (data->to_be_displayed[gpio]) {
            ui_draw_timeline_last(gpio, data->num_places, prev_values[gpio]);
        }
    }
}

void ui_show_timeline_window(ui_timeline_display_data_t * data) {
    int ch, i, y, x;
    uint32_t max_places, num_places;
    getmaxyx(stdscr, max_timeline_y, max_timeline_x);
    int max_y = max_timeline_y;
    int max_x = max_timeline_x;
    timeline_window = newwin(SRC_LINES, SRC_COLS, SRC_START_Y, SRC_START_X);
    mvwaddstr(timeline_window, 0, 0, "TIMELINE\n"); 
    mvwaddstr(timeline_window, 2, 0, "press any key to go back to the edit window\n"); 
    noecho();
    keypad(stdscr, TRUE);
        
    //test_timeline(50);
    
    max_places = (SRC_COLS - TIMELINE_LEFT_MARGIN) / CHARS_PER_PLACE;
    if (data->num_places > max_places) data->num_places = max_places;
    
    show_timeline(data);

    wrefresh(timeline_window);

    ch = getch();

    endwin();
    return;
}

