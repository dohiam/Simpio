/*!
 * @file /editor.h
 * @brief A very simple text editor, part of the Simpio project.
 * @details
 * super simple text management: suitable only for very small files!
 * a) model: a linear array (buffer) of chars read from a file, with an array of indecies 
 *           into the first char on a line, and a cursor position at a certain line and 
 *            character position on that line
 * b) view: displayed in a window from first to last line (based on size of the window)
 *          with cursor displayed on x and y position on the screen
 * c) controller: scrolling operations affect view only (but somewhat based on the model)
 *                while operations that add and remove chars affect both the view and
 *                the underlying model
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef EDITOR_H
#define EDITOR_H

#include <ncurses.h>
#include <stdlib.h>

/**********************************************************************************
 **********************************************************************************/

#define MAX_NUMBER_OF_LINES 1000
#define ESTIMATED_LINE_SIZE 80
#define TEXT_BUFFER_SIZE MAX_NUMBER_OF_LINES * ESTIMATED_LINE_SIZE

typedef struct {
  /* model part */
  FILE *f;
  char buffer[TEXT_BUFFER_SIZE];
  int  num_chars;
  int  line_starts[MAX_NUMBER_OF_LINES];
  int  line_ends[MAX_NUMBER_OF_LINES];
  int  num_lines; 
  /* view part */
  WINDOW * window;
  int  window_num_rows;
  int  window_num_cols;
  int  first_displayed_line;
  int  last_displayed_line;
  int  cursor_x;
  int  cursor_y;
  /* view <-> model mapping */
  int  current_line;  /* line in the buffer corresponding to the line on the screen */
} editor_t;
  
int ed_init(editor_t *ed, FILE *tf, WINDOW * win, int linenum_color);
void ed_display_line(editor_t * tb, int line_num);
void ed_display(editor_t * tb);
void ed_display_refresh(editor_t * tb);
void ed_home(editor_t * tb);
void ed_up(editor_t * tb);
void ed_down(editor_t * tb);
void ed_right(editor_t * tb);
void ed_left(editor_t * tb);
void ed_page_down(editor_t * tb);
void ed_page_up(editor_t * tb);
void ed_del_char(editor_t *ed);
void ed_insert_char(editor_t *ed, char ch);
void ed_clear_display(editor_t *ed);
void ed_clear_display_line(editor_t * tb, int line_num);
void ed_goto_line(editor_t * tb, int line_num);

#endif
