/*!
 * @file /editor.c
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
  
#include "editor.h"
#include "instruction.h"
#include "ui.h"

static int linenum_color;

int ed_init(editor_t *ed, FILE *tf, WINDOW * win, int color) {
  linenum_color = color;
  /* read from file into buffer and identify start of each line */
  char ch;
  int saw_cr = 0;
  int saw_lf = 0;
  int line_start = 0;
  bool saw_eof = false;
  ed->num_chars = 0;
  ed->num_lines = 0;
  if (!tf) return 0;
  for (ed->num_chars=0, ch=fgetc(tf); (ed->num_chars < TEXT_BUFFER_SIZE) && !saw_eof; ed->num_chars++, ch=fgetc(tf)) {
    ed->buffer[ed->num_chars] = ch;
    // account for Windows and Linux style EOLs - either CRLF or just LF
    if (saw_lf) {
      if (saw_cr && saw_lf) ed->line_ends[ed->num_lines] = ed->num_chars - 2;
      else ed->line_ends[ed->num_lines] = ed->num_chars - 1;
      ed->line_starts[ed->num_lines] = line_start;
      if (++ed->num_lines >= MAX_NUMBER_OF_LINES) {
        status_msg("ERROR: file size exceeds max number of lines!!!\n");
        break;
      }
      line_start = ed->num_chars;  /* next line starts here */
      saw_lf = 0;
      saw_cr = 0;
    }
    if (ch == '\r') saw_cr = 1;
    if (ch == '\n') saw_lf = 1;
    if (ch == EOF) saw_eof = true;
  }
  if (ed->num_chars == TEXT_BUFFER_SIZE) status_msg("ERROR: file size exceeds buffer size!!!\n");
  /* handle last line that is unhandled when loop ends */
  if (saw_lf && saw_cr) ed->line_ends[ed->num_lines] = ed->num_chars - 2;
  else ed->line_ends[ed->num_lines] = ed->num_chars - 1;
  ed->line_starts[ed->num_lines++] = line_start;
  /* set up the view */
  ed->current_line = 0;
  ed->window = win;
  getmaxyx(stdscr, ed->window_num_rows, ed->window_num_cols);
  ed->first_displayed_line = 0;
  ed->last_displayed_line = (ed->num_lines < ed->window_num_rows) ? ed->num_lines-1 : ed->window_num_rows-1;
  ed->cursor_x = 0;
  ed->cursor_y = 0;
  status_msg("editor: lines=%d chars=%d window rows=%d\n", ed->num_lines, ed->num_chars, ed->window_num_rows);
  return 1;
}

void ed_clear_display(editor_t *ed) {
  werase(ed->window);
}

void ed_clear_display_line(editor_t *ed, int line_num) {
  wmove(ed->window, line_num - ed->first_displayed_line, 0);
  wclrtoeol(ed->window);
  wmove(ed->window, ed->cursor_y, ed->cursor_x);
}

void ed_display_line(editor_t *ed, int line_num) {
  int i;
  int line_len = ed->line_ends[line_num] - ed->line_starts[line_num];
  int len = (ed->window_num_cols < line_len) ? ed->window_num_cols : line_len;
  int row = line_num - ed->first_displayed_line;
  if (instruction_is_breakpoint(line_num+1)) wattron(ed->window, A_BOLD);
  else wattroff(ed->window, A_BOLD);
  wmove(ed->window, row, 0);
  wattron(ed->window, linenum_color);
  wprintw(ed->window, "%03d ", line_num+1);
  wattroff(ed->window, linenum_color);
  for (i=ed->line_starts[line_num]; i < (ed->line_starts[line_num] + len); i++) {
    waddch(ed->window, ed->buffer[i]);
  }
}

void ed_display(editor_t *ed) {
  int i;
  for (i = ed->first_displayed_line; i <= ed->last_displayed_line; i++) {
      ed_display_line(ed, i);  
  }
  //status_msg("refresh: first_line=%d last_line=%d \n", ed->first_displayed_line, ed->last_displayed_line);
}

void ed_display_refresh(editor_t *ed) {
  werase(ed->window);
  ed_display(ed);
  wmove(ed->window, ed->cursor_y, ed->cursor_x+4);  //TODO: make line number field adjustment not hardcoded
  wrefresh(ed->window);
}

void ed_home(editor_t *ed) {
  /* update the model */
  ed->current_line = ed->first_displayed_line;
  /* update the view */
  ed->cursor_x = 0; 
  ed->cursor_y = 0;
  wmove(ed->window, ed->cursor_y, ed->cursor_x);
}

void ed_up(editor_t *ed) {
  int line_len;
  /* update the model */
  if (ed->current_line > 0) ed->current_line--;
  else return;
  /* update the view 
     1) if cursor is at the first line in the window and there is more file that can be displayed, scroll up one line
     2) otherwise just move the cursor up one row on the screen
  */
  if ( (ed->cursor_y == 0) && (ed->first_displayed_line > 0) ) {
      ed->first_displayed_line--;
      ed->last_displayed_line--;
  }
  else ed->cursor_y--;  
  // adjust cursor if this line is shorter
  line_len = ed->line_ends[ed->current_line] - ed->line_starts[ed->current_line] - 1; /* note that x cursor starts at zero */
  if (ed->cursor_x > line_len) ed->cursor_x = line_len; 
  if (ed->cursor_x < 0) ed->cursor_x = 0;
  wmove(ed->window, ed->cursor_y, ed->cursor_x);
}

void ed_down(editor_t *ed) {
  int line_len;
  /* update the model */
  if (ed->current_line < (ed->num_lines-1)) ed->current_line++;
  else return;
  /* update the view
     1) if cursor is at the last line in the window and there is more file that can be displayed, scroll down one line
     2) otherwise just move the cursor down one row on the screen
   */
  if ( (ed->cursor_y == (ed->window_num_rows-1)) && (ed->last_displayed_line < ed->num_lines - 1) ) {
      ed->first_displayed_line++;
      ed->last_displayed_line++;
  }
  else ed->cursor_y++;  
  // adjust cursor if this line is shorter
  line_len = ed->line_ends[ed->current_line] - ed->line_starts[ed->current_line] - 1; /* note that x cursor starts at zero */
  if (ed->cursor_x > line_len) ed->cursor_x = line_len; 
  if (ed->cursor_x < 0) ed->cursor_x = 0;
  wmove(ed->window, ed->cursor_y, ed->cursor_x);
}

void ed_right(editor_t *ed) {
  int line_len;
  line_len = ed->line_ends[ed->current_line] - ed->line_starts[ed->current_line] - 1; /* note that x cursor starts at zero */
  //status_msg("current_line:%d start:%d end:%d\n", ed->current_line, ed->line_starts[ed->current_line], ed->line_ends[ed->current_line]);
  ed->cursor_x++;
  if (ed->cursor_x > line_len) ed->cursor_x = line_len;  
  if (ed->cursor_x < 0) ed->cursor_x = 0;
  wmove(ed->window, ed->cursor_y, ed->cursor_x);
}

void ed_left(editor_t *ed) {
  if (ed->cursor_x <= 0) { ed->cursor_x = 0; return; }
  ed->cursor_x--;
  wmove(ed->window, ed->cursor_y, ed->cursor_x);
}

void ed_page_down(editor_t *ed) {
  int scroll_amount = ed->window_num_rows;
  int prev_last_line, movement;
  prev_last_line = ed-> last_displayed_line;
  if ((ed->last_displayed_line + scroll_amount) >= ed->num_lines) ed->last_displayed_line = ed->num_lines - 1;
  else ed->last_displayed_line += scroll_amount;
  ed->first_displayed_line = ed->last_displayed_line - ed->window_num_rows + 1;
  movement = ed->last_displayed_line - prev_last_line;
  ed->current_line += movement;
}

void ed_page_up(editor_t *ed) {
  int scroll_amount = ed->window_num_rows;
  int prev_first_line, movement;
  prev_first_line = ed->first_displayed_line;
  if ((ed->first_displayed_line - scroll_amount) <= 0) ed->first_displayed_line = 0;
  else ed->first_displayed_line -= scroll_amount;
  ed->last_displayed_line = ed->first_displayed_line + ed->window_num_rows - 1;
  movement = prev_first_line - ed->first_displayed_line;
  ed->current_line -= movement;
}

void ed_del_char(editor_t *ed) {
  /* update model */
  int i;
  for (i = ed->line_starts[ed->current_line] + ed->cursor_x; i < ed->num_chars; i++) ed->buffer[i] = ed->buffer[i+1];
  ed->num_chars--;
  ed->line_ends[ed->current_line]--;
  for (i=ed->num_lines-1; i>ed->current_line; i--) {
    ed->line_starts[i]--;
    ed->line_ends[i]--;
  }
  /* update view */
  wdelch(ed->window);
}

void ed_insert_char(editor_t *ed, char ch) {
  /* update model (if full, do nothing, else make room and add the char */
  int i, pos;
  if (ed->num_chars >= TEXT_BUFFER_SIZE) {
    status_msg("buffer full\n");
    return;
  }
  pos = ed->line_starts[ed->current_line] + ed->cursor_x;
  for (i = ed->num_chars; i > pos; i--) ed->buffer[i] = ed->buffer[i-1];
  ed->num_chars++;
  ed->line_ends[ed->current_line]++;
  for (i=ed->num_lines-1; i>ed->current_line; i--) {
    ed->line_starts[i]++;
    ed->line_ends[i]++;
  }
  ed->buffer[pos] = ch;
  /* update view */
  waddch(ed->window, ch);
  ed_right(ed);
}

void ed_goto_line(editor_t *ed, int line_num) {
  line_num--; /* adjust so that it starts at zero, like the window numbering */
  if (line_num < ed->first_displayed_line || line_num > ed->last_displayed_line) {
    /* scroll to line_num */
    ed->first_displayed_line = line_num;
    ed->last_displayed_line = ed->first_displayed_line + ed->window_num_rows - 1;
    if (ed->last_displayed_line >= ed->num_lines) {
        ed->last_displayed_line = ed->num_lines;
        ed->first_displayed_line = ed->num_lines - ed->window_num_rows + 1;
    }
  }
  ed->cursor_y = line_num - ed->first_displayed_line;
  ed->cursor_x = 0;
  ed->current_line = line_num;
  wmove(ed->window, ed->cursor_y, ed->cursor_x);
}
