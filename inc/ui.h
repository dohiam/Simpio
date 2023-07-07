/*!
 * @file /ui.h
 * @brief Simpio User Interface
 * @details
 * This defines the parameters, callbacks, and functions involved in providing the Simpio User Interface
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef UI_H
#define UI_H
#include <ncurses.h>
#include <stdlib.h>

/***********************************************************************************************************
 *
 * USAGE: 
 *  1) Can adjust the 3 window layouts using the #defines
 *     a) source code window
 *     b) registers window
 *     c) status window
 *  2) Write callback functions:
 *     a) build/compile the program
 *     b) set and clear breakpoints given a line number in the source file
 *     c) step the program one statement
 *     d) continue until the next breakpoint
 *     e) save the current program
 *     NOTE: these will use status_msg and data_msg as appropriate to return their results to the UI
 *
 ***********************************************************************************************************/

#define SRC_START_Y 0
#define SRC_START_X 0
#define SRC_LINES max_y
#define SRC_COLS (2 * max_x / 3)
#define REGS_START_Y 0
#define REGS_START_X (SRC_COLS + 1)
#define REGS_LINES (1 * max_y /2)
#define REGS_COLS (max_x - REGS_START_X)
#define STATUS_START_Y (REGS_LINES)
#define STATUS_START_X REGS_START_X
#define STATUS_LINES (max_y - REGS_LINES)
#define STATUS_COLS REGS_COLS
#define STATUS_BUFFER_SIZE 2048

/**********************************************************************************
 * run UI  
 **********************************************************************************/


/* using all the following functions does cause a lot of back and forth between the main procedure and UI, but
   this allows the ui to focus just on drawing things without having to know anything about the hardware logic. 
 */

typedef int  (*build_compile_t)(char *pio_pgm);   /* returns line number that had an error, -1 if no errors */ 
typedef int  (*step_pgm_t)();                     /* returns the line number in the source program with the next instruction to execute */
typedef int  (*toggle_breakpoint_t)(int line);    /* returns 0 if the breakpoint could not be set, otherwise return 1 */
typedef int  (*run_execute_t)();                  /* returns line number the program stopped at */
typedef int  (*save_pgm_t)(char *pio_pgm);        /* returns 1 if saved successfully, 0 if there was an error */
typedef void (*get_timeline_params_t) ();         /* sets timeline paramters that can be retrieved later separate accessor functions */
typedef void (*show_timeline_t) ();               /* function to show the timeline window (using provided ui drawing functions */
typedef void (*show_fifos_t) ();                  /* function to show contents of the fifos */

typedef struct {
    build_compile_t        build_compile_function;
    step_pgm_t             step_function;
    toggle_breakpoint_t    toggle_function;
    run_execute_t          run_function;
    save_pgm_t             save_function;
    get_timeline_params_t  get_timeline_params_function;
    show_timeline_t        show_timeline_function;
    show_fifos_t           show_fifos;
    char * filename;
} ui_user_functions_t;

void ui_run(ui_user_functions_t * user_functions);

/**********************************************************************************
 * write to the UI windows (imlemented as macros, thus the extern declaration)  
 **********************************************************************************/

extern WINDOW *src_win, *regs_win, *status_win;

#define status_window_reset() wmove(status_win, 0, 0)
#define regs_window_reset() wmove(regs_win, 0, 0)

#define status_msg(...) wprintw(status_win, __VA_ARGS__); wrefresh(status_win)
#define regs_msg(...) wprintw(regs_win, __VA_ARGS__); wrefresh(regs_win)

/**********************************************************************************
 * timeline dialog 
 **********************************************************************************/

#define TIMELINE_DIALOG_NUM_FIELDS 5

typedef struct {
    bool gpio_set[TIMELINE_DIALOG_NUM_FIELDS];
    uint8_t gpio_values[TIMELINE_DIALOG_NUM_FIELDS];
} ui_timeline_dialog_data_t;

/**********************************************************************************
 * timeline display 
 **********************************************************************************/

/* function timeline_window calls to get gpio values:
   note that these don't care which hardware gpio it is, only the index for display (0..4)
   note that this type of interface is to separate display logic from underlying hardware logic
 */

typedef struct {
    bool  values[TIMELINE_DIALOG_NUM_FIELDS];
    uint32_t clock_tick; 
} ui_gpio_history_t;

// callback function to get value to be displayed
typedef ui_gpio_history_t * (*ui_timeline_iteration_callback_t)();

typedef struct {
    uint32_t num_places; // may get lowered depending on how many can be done
    bool     to_be_displayed[TIMELINE_DIALOG_NUM_FIELDS];
    ui_timeline_iteration_callback_t callback;
} ui_timeline_display_data_t;

ui_timeline_dialog_data_t * ui_show_timeline_dialog();

void ui_show_timeline_window(ui_timeline_display_data_t * data_to_display);

/**********************************************************************************
 * blank temp window (for whatever) 
 **********************************************************************************/

extern WINDOW *temp_window;

void ui_temp_window_open();
#define ui_temp_window_write(...) wprintw(temp_window, __VA_ARGS__); wrefresh(temp_window)
#define ui_temp_window_getch() wgetch(temp_window)
void ui_temp_window_close();


/**********************************************************************************
 * The following are to allow the user to break out of a program that is running
 * forever. Before running something that may run forever, call the enter function.
 * Then periodically call the check function to give the user an option to break
 * out of it. If check returns true, then cleanly exit running forever and 
 * call the exit function to return control to the UI.
 **********************************************************************************/

void ui_enter_run_break_mode();
void ui_exit_run_break_mode();
bool ui_break_check();

#endif