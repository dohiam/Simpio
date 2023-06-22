/**********************************************************************************
 * MAIN:
 * TODO: be sure to include desciption about this controlling the regs window
 * 
 * 
 * 
 * 
 * 
 **********************************************************************************/
  

#include "ui.h"  
#include "instruction.h"
#include "execution.h"
#include "hardware_changed.h"
#include "parser.h"
#include "print.h"
#include <sys/stat.h>
#include <string.h>

char temp_file[] = "temp_pio_file"; /* save to temporary file until debugged */
char * input_file;


/* TODO: update REGs window layout: 
GPIOS:                   PINDIRS:
  0 1 2 3 4 5 6 7 8 9      0 1 2 3 4 5 6 7 8 9
0 X X X X X X X X X X    0 X X X X X X X X X X
1 X X X X X X X X X X    1 X X X X X X X X X X
2 X X X X X X X X X X    2 X X X X X X X X X X
3 X X X X X X X X X X    3 X X X X X X X X X X
PIO:0 IRQ-0:X IRQ-1:X 
SM:0 CLOCK: XXX  DELAY: XX   X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
SM:1 CLOCK: XXX      X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
SM:2 CLOCK: XXX      X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
SM:3 CLOCK: XXX      X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
PIO:1 IRQ-0:X IRQ-1:X
SM:0 CLOCK: XXX      X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
SM:1 CLOCK: XXX      X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
SM:2 CLOCK: XXX      X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
SM:3 CLOCK: XXX      X:12345678   Y:12345678 
   OSR:12345678 ISR:12345678     IN:12345678 OUT:12345678
*/

#define CHAR_EMPTY 'E'
#define CHAR_FULL  'F'
#define CHAR_NOT_FULL_OR_EMPTY 'D'

#define print_with_change(XYZ, fmt)   if (hardware_changed->sms[sm_num].XYZ) wattron(regs_win, A_BOLD); regs_msg(fmt, sm->XYZ); wattroff(regs_win, A_BOLD);
      
void update_regs() {
    char status_char;
    int pio_num, sm_num, up_num, gpio_num, row, col, col_max, i, pad;
    hardware_changed_t * hardware_changed = hardware_get_changed();
    user_variable_t * var; user_variable_enumerator_t var_e;
    regs_window_reset();
    regs_msg("GPIOS:                    PINDIRS:                User Vars:\n");
    regs_msg("  0 1 2 3 4 5 6 7 8 9       0 1 2 3 4 5 6 7 8 9");
    var = user_variable_first(&var_e);
    if (var && (var->name[0] != 0) ) {
      if (var->has_value) { regs_msg("   %s=%08X\n", var->name, var->value); }
      else regs_msg("   %s=___\n", var->name);
    }
    else regs_msg("\n");
    row = 0;
    col = 0;
    col_max = 10;
    var = user_variable_next(&var_e);
    int var_count=0;
    for (gpio_num = 0; gpio_num < NUM_GPIOS; gpio_num++) {
        if (col == 0) regs_msg("%1d ", row);
        if (hardware_changed->gpios[gpio_num].value) {
          wattron(regs_win, A_BOLD);
        }
        regs_msg("%1d ", hardware_get_gpio(gpio_num));
        wattroff(regs_win, A_BOLD);      
        if (++col == col_max) {
            regs_msg("    %1d ", row);
            for (i=col_max-1; i>=0; i--) {  // note that col has been incremented but not yet gpio_num
               if (hardware_changed->gpios[gpio_num-i].pindir) wattron(regs_win, A_BOLD);
               regs_msg("%1d ", hardware_get_gpio_dir(gpio_num-i));
               wattroff(regs_win, A_BOLD);      
            }
            if (var && (var->name[0] != 0) ) { 
              if (var->has_value) { regs_msg("%s=%08X", var->name, var->value); }
              else regs_msg("  %s=___", var->name);
              var = user_variable_next(&var_e);
            }
            regs_msg("\n");
            row++;
            col = 0;
        }      
    }
    //print the trailing gpio dirs
    pad = 22 - (NUM_GPIOS % col_max);
    for (i=0; i<pad; i++) regs_msg(" ");
    regs_msg("%1d ", NUM_GPIOS / col_max);
    for (gpio_num = NUM_GPIOS - (NUM_GPIOS % col_max); gpio_num < NUM_GPIOS; gpio_num++) {
        if (hardware_changed->gpios[gpio_num-i].pindir) wattron(regs_win, A_BOLD);
        regs_msg("%1d ", hardware_get_gpio_dir(gpio_num-i));
        wattroff(regs_win, A_BOLD);      
    }
    regs_msg("\n");
    pio_num = 0;
    FOR_ENUMERATION(pio, pio_t, hardware_pio) { 
        regs_msg("PIO: %1d ", pio_num);
        if (hardware_changed->pios[pio_num].irqs[0]) wattron(regs_win, A_BOLD);
        regs_msg("IRQ-0: %1d ", pio->irqs[0]);
        wattroff(regs_win, A_BOLD);      
        if (hardware_changed->pios[pio_num].irqs[1]) wattron(regs_win, A_BOLD);
        regs_msg("IRQ-1: %1d\n", pio->irqs[1]);
        wattroff(regs_win, A_BOLD);      
        sm_num = 0;
        FOR_ENUMERATION(sm, sm_t, hardware_sm) {
            if (sm->pio_num != pio_num) {sm_num++; continue;}
            regs_msg("SM:%02d ", sm_num);
            regs_msg("CLOCK: %04ld", sm->clock_tick);
            if (pio->instructions[sm->pc].delay_left > 0) wattron(regs_win, A_BOLD);
            regs_msg("  DELAY: %02d", pio->instructions[sm->pc].delay_left); 
            wattroff(regs_win, A_BOLD);      
            print_with_change(scratch_x," X:%08X ") 
            print_with_change(scratch_y,"Y:%08X ") 
            print_with_change(exec_machine_instruction,"EXEC:%08X \n") 
            regs_msg("PC:%02d",sm->pc);
            regs_msg(" OSR");
            print_with_change(shift_out_count,"-%02d") 
            print_with_change(osr,":%08X") 
            regs_msg(" ISR");
            print_with_change(shift_in_count,"-%02d") 
            print_with_change(isr,":%08X") 
            regs_msg(" RX");
            if (hardware_changed->sms[sm_num].fifo == RX_STATE) wattron(regs_win, A_BOLD);
            if (sm->fifo.rx_state == FIFO_EMPTY) status_char = CHAR_EMPTY;
            else {if (sm->fifo.rx_state == FIFO_FULL) status_char = CHAR_FULL;
                  else status_char = CHAR_NOT_FULL_OR_EMPTY;}
            regs_msg("-%c:", status_char);
            wattroff(regs_win, A_BOLD);
            if (sm->fifo.mode != TX_ONLY) {
                if ((hardware_changed->sms[sm_num].fifo == RX_CONTENTS) || (hardware_changed->sms[sm_num].fifo == RX_STATE)) wattron(regs_win, A_BOLD); 
                for (i=sm->fifo.rx_top-1; i>=sm->fifo.rx_bottom; i--) {
                    regs_msg("%02X", sm->fifo.buffer[i]);
                }
                wattroff(regs_win, A_BOLD);   
                regs_msg(" ");
            }
            regs_msg("TX");
            if (hardware_changed->sms[sm_num].fifo == TX_STATE) wattron(regs_win, A_BOLD);
            if (sm->fifo.tx_state == FIFO_EMPTY) status_char = CHAR_EMPTY;
            else {if (sm->fifo.tx_state == FIFO_FULL) status_char = CHAR_FULL;
                  else status_char = CHAR_NOT_FULL_OR_EMPTY;}
            regs_msg("-%c:", status_char);
            wattroff(regs_win, A_BOLD);
            if (sm->fifo.mode != RX_ONLY) {
                if ((hardware_changed->sms[sm_num].fifo == TX_CONTENTS) || (hardware_changed->sms[sm_num].fifo == TX_STATE)) wattron(regs_win, A_BOLD); 
                for (i=sm->fifo.tx_top-1; i>=sm->fifo.tx_bottom; i--) {
                    regs_msg("%02X", sm->fifo.buffer[i]);
                }
                wattroff(regs_win, A_BOLD);   
                regs_msg("\n");
            }
            sm_num++;
        }
        pio_num++;
    }
    FOR_ENUMERATION(up, user_processor_t, hardware_user_processor) {
       regs_msg("UP%d PC:%d Delay:%d DATA:%s\n", up->this_num, up->pc, up->instructions[up->pc].delay_left, up->data);
    }
}

static bool built = false;
static bool first_stepit = true;

/* returns line number that had an error, -1 if no errors */ 
int buildit(char *pio_pgm) {
    int error_line;
    first_stepit = true;
    error_line = simpio_parse(temp_file);
    if (error_line != 0) {
      //status_msg("Error on line %d\n", error_line);
      return error_line;
    }
    else {
      built = true;
    }
    return -1;
}

static int prev_line;

/* returns the line number in the source program with the next instruction to execute */
int stepit() {
    int next_line;
    instruction_or_user_instruction_t instr;
    if (!built) {
      status_msg("need to build before stepping\n");
      return 1;
    }
    hardware_snapshot();
    if (first_stepit) {
        first_stepit = 0;
        next_line = exec_first_instruction_that_will_be_executed();
        if (next_line >= 0) {
            update_regs();
            PRINTI("program starting at line %d\n", next_line);
            prev_line = next_line;
            return next_line;
        } 
        else {
            status_msg("no instructions to execute!\n");
            return 1;
        }
    }
    else {
        next_line = exec_step_programs_next_instruction();
        if (next_line > 0) {
            instruction_for_line(prev_line, &instr);
            if (instr.instruction_type == _no_instruction) { status_msg("error: no previous instruction!\n"); }
            else {
              if (instr.instruction_type == _instruction) {
                  if (instr.ioru.instruction_ptr->in_delay_state) { status_msg("Line %d delaying\n", prev_line); }
                  else { if (instr.ioru.instruction_ptr->not_completed) { status_msg("Line %d blocked or not done yet\n", prev_line); }
                         //else status_msg("\n"); 
                       }
              }
              else { 
                  if (instr.ioru.user_instruction_ptr->in_delay_state) { status_msg("Line %d delaying\n", prev_line);  }
                  else { if (instr.ioru.user_instruction_ptr->not_completed) { status_msg("line %d blocked or not done yet\n", prev_line); }
                         //else status_msg("\n"); 
                       }
              }
            }
        }
        else { 
          status_msg("step done, no instructions found to execute\n"); 
        }
    }
    update_regs();
    prev_line = next_line;
    if (next_line > 0) return next_line;
    else {
      status_msg("no next instruction to execute\n");
      return 1;
    }
}

/* returns 0 if the breakpoint could not be set, otherwise return 1 */
int toggleit(int line) {       
    if (!built) {
      status_msg("need to build before setting breakpoint\n");
      return 0;
    }
    if (instruction_toggle_breakpoint(line)) {status_msg("toggled breakpoint line %d\n", line);}
    else {status_msg("failed to toggle breakpoint line %d\n", line);}    
    return 1;
}

/* returns line number the program stopped at */
int runit() {   
    int hit_line;
    hardware_snapshot();
    if (!built) {
      status_msg("need to build before running\n");
      return 1;
    }
    first_stepit = false;
    status_msg("running program \n");
    hit_line = exec_run_all_programs();
    status_msg("program stopped at line %d\n", hit_line);
    update_regs();
    prev_line = hit_line;
    return hit_line;
}

/* returns 1 if saved successfully, 0 if there was an error */
int saveit(char *pio_pgm) {
    FILE *fp;
    int rc;
    status_msg("save program \n");
    fp = fopen(temp_file,"w+");
    if ( fp ) {
	   fputs(pio_pgm,fp);
       rc = -1;
    }
    else {
       status_msg("Failed to open the file for saving\n");
       rc = 0;
    }
    fclose(fp);
    return rc;
}

static ui_timeline_dialog_data_t * timeline_dialog_data;

static uint32_t max_timeline_values, num_timeline_values;

void get_timeline_parameters() {
  timeline_dialog_data = ui_show_timeline_dialog(); 
  max_timeline_values = hardware_changed_gpio_history_init();
}

// callback function to get value to be displayed

static ui_timeline_display_data_t timeline_display_data;

static ui_gpio_history_t ui_history;
static gpio_history_t * hw_history;
static uint32_t gpio_history_num, iteration_count; // since num to display may be less than the history, we may need to "eat" the first few values

ui_gpio_history_t * timeline_callback_function() {
  uint8_t gpio_num;
  int32_t pad = gpio_history_num - timeline_display_data.num_places;
  while (iteration_count < pad) {
    hw_history = hardware_changed_gpio_history_get();
    iteration_count++;
  }
  hw_history = hardware_changed_gpio_history_get();
  if (!hw_history) return NULL;
  // map display gpio_nums to hardware numbers
  for (gpio_num=0; gpio_num<TIMELINE_DIALOG_NUM_FIELDS; gpio_num++) {
    if (timeline_dialog_data->gpio_set[gpio_num]) {
      ui_history.values[gpio_num] = hw_history->values[timeline_dialog_data->gpio_values[gpio_num]];
    }
  }
  ui_history.clock_tick = hw_history->clock_tick;
  return &ui_history;
}

void show_timeline() {
    uint8_t i;
    gpio_history_num = timeline_display_data.num_places = hardware_changed_gpio_history_iteration();
    if (gpio_history_num == 0) {
        status_msg("no timeline history, nothing to show\n");
        return;  
    }
    iteration_count = 0;
    for (i=0; i < TIMELINE_DIALOG_NUM_FIELDS; i++) timeline_display_data.to_be_displayed[i] = timeline_dialog_data->gpio_set[i];
    timeline_display_data.callback = &timeline_callback_function;
    ui_show_timeline_window(&timeline_display_data);
}

ui_user_functions_t ui_functions = {&buildit, &stepit, &toggleit, &runit, &saveit, &get_timeline_parameters, &show_timeline, NULL};  // filename filled in later

int main_test(int argc, char** argv) {
  instruction_or_user_instruction_t instr;
  int next_line;  
  printf("running program \n");
  int line = atoi(argv[3]);
  printf("line:%d\n",line);
  if (line <= 0) {
    printf("error: expected positive breakpoint line number as third argument\n");
    return -1;
  }
  bool toggled = instruction_toggle_breakpoint(line);
  if (!toggled) {
    printf("error, couldn't find instruction at line %d\n", line);
    return -1;
  }
  next_line = exec_run_all_programs(&instr);
  printf("stopped execution at line %d\n", next_line);
  if (next_line == line) {
      printf("stopped at expected line\n");
      return 0;
  }
  else return next_line;
}

typedef struct {
    bool syntax;
    bool test;
    bool ui;
    bool print;
    bool inter;
    bool debug;
} options_t;

static options_t options;

void parse_options(int argc, char** argv) {
    char * optionstr;
    int line;
    if (argc > 2) {
        optionstr= argv[1];
        options.syntax   = strchr(optionstr, 's');
        options.test     = strchr(optionstr, 't');
        options.ui       = strchr(optionstr, 'u');
        options.print    = strchr(optionstr, 'p');
        options.inter    = strchr(optionstr, 'i');
        options.debug    = strchr(optionstr, 'd');
    }
    else {
        options.syntax   = false;
        options.test     = false;
        options.ui       = true;
        options.print    = false;
        options.inter    = false;
        options.debug    = false;
    }
    if (argc > 3) {
        line = atoi(argv[3]);
        if (line <= 0) {
            printf("error: expected positive breakpoint line number as third argument\n");
            exit(-1);
        }
    }
    else line = -1;
    if (options.print & !options.syntax) {
        printf("option print requires options syntax, auto setting syntax option\n");
        options.syntax = true;
    }
    if (options.inter & !options.syntax) {
        printf("option interactive requires options syntax, auto setting syntax option\n");
        options.syntax = true;
    }
    if (options.test & !options.syntax) {
        printf("option test requires options syntax, auto setting syntax option\n");
        options.syntax = true;
    }
    if (options.test && options.ui) {
        printf("incompatible options t & u; choose either ui (u) or test mode (t), not both\n");
        exit(-1);
    }
    if (options.test && line <0) {
        printf("test mode requires a line number to run to for test success\n");
        exit(-1);
    }
}

#define INPUT_BUFF_SIZE 80
static char input_buff[INPUT_BUFF_SIZE];

int main(int argc, char** argv) {
  int max_x, max_y;
  int ch, rc, next_line, break_line;
  FILE *pio_pgm;
  bool toggled;
  struct stat stat_rc;
  
  if( argc < 2 || argc >4 ) {
    printf("Usage: %s <filename> [stupid] [line_number] \n", argv[0]);
    printf("[stupid] means optional options s, t, u, p, i, and/or d\n");
    printf("s=syntax details, t=test  (run to line), u=ui, p=print config, i=interactive mode, d=details\n");
    printf("default (no options) means run with ui and info messages\n");
    printf("good option examples:\n");
    printf("   %s <pio file> s         ===> syntax check and print results to terminal\n", argv[0]);
    printf("   %s <pio file> sd        ===> print detailed parsing messages to terminal and stop\n", argv[0]);
    printf("   %s <pio file> t <line>  ===> run to line with minimal messages (for simpio test suite)\n", argv[0]);
    printf("   %s <pio file> u         ===> run UI with minimal messages\n", argv[0]);
    printf("   %s <pio file> ui        ===> run UI with more info messages\n", argv[0]);
    printf("   %s <pio file> ud        ===> run UI with more detailed messages\n", argv[0]);
    printf("   %s <pio file> p         ===> parse and print hardware configuration\n", argv[0]);
    printf("   %s <pio file> i         ===> interactive mode (no UI) with info messages\n", argv[0]);
    printf("   %s <pio file> id        ===> interactive mode (no UI) with detailed messages\n", argv[0]);
    exit(-1); 
  }
  
  if (argc >2) ui_functions.filename = argv[2];
  else ui_functions.filename = argv[1];
  if (stat(ui_functions.filename, &stat_rc)) {
      printf("unable to find file %s\n", ui_functions.filename);
      exit(-1);
  }

  parse_options(argc, argv);
    
  if (options.syntax) {
      set_print_ui(false);
      if (options.debug) {
          set_print_level(DEBUG_PRINT_LEVEL);
          yydebug = 1;
          simpio_parse_debug(argv[2]);
          printf("Debug syntax check complete, exiting...\n");
      }
      else {
          set_print_level(MIN_PRINT_LEVEL);
          yydebug = 0;
          rc = simpio_parse(argv[2]);
          if (rc) {
              printf("syntax error on line %d\n\n", rc);
              exit(-1);
          }
          else printf("\nsyntax ok\n\n");
      }
      if (options.print) {
            printf_hardware_configuration();
            printf_labels();
            printf_defines();
            printf_instructions();
            printf_user_instructions();
      }
  }
    
  if (options.inter && !options.ui) {
    set_print_ui(false);
    printf("enter q to quit or any other key to step execution\n");
    ch = getchar();
    if (ch == 'q' || ch == 'Q') exit(0);
    set_print_lines(argv[2]);
    next_line = exec_first_instruction_that_will_be_executed();
    if (next_line >= 0) {
        printf("program starting at line %d\n", next_line);
        print_line(next_line);
        } 
    else {
        status_msg("no instructions to execute!\n");
        exit(-1);
        }
    if (options.debug) set_print_level(DEBUG_PRINT_LEVEL);
    else set_print_level(INFO_PRINT_LEVEL);
    ch = getchar();     
    while (ch != 'q' && ch != 'Q') {
        switch (ch) {
            case 'b':
            case 'B':
                if (fgets (input_buff, INPUT_BUFF_SIZE, stdin) == NULL) {
                    printf("ERROR: enter line number after character 'b' to toggle breakpoint on that line\n");
                    break;
                }
                else {
                    break_line = atoi(input_buff);
                    if (break_line == 0) {
                        printf("ERROR: enter line number after character 'b' to toggle breakpoint on that line\n");
                        break;
                    }
                    else {
                        toggled = instruction_toggle_breakpoint(break_line);
                        if (!toggled) printf("error, couldn't find instruction at line %d\n", break_line);
                        else { 
                            if (instruction_is_breakpoint(break_line)) printf("set breakpoint on the following line:\n");
                            else printf("cleared breakpoint on the following line:\n");
                            print_line(break_line);
                        }
                    }
                }
                break;
            case 'r':
            case 'R':
                next_line = exec_run_all_programs();
                print_line(next_line);
                break;
            default:
                next_line = exec_step_programs_next_instruction();
                print_line(next_line);
        };
        ch = getchar();
    }
    exit(0);
  }
    
  if (options.test) {
    set_print_ui(false);
    rc = main_test(argc, argv);
    printf("exiting: rc:%d\n", rc);
    exit(rc);
  }
    
  if (options.ui) {
    yydebug = 0;
    set_print_ui(true);
    if (options.debug) set_print_level(DEBUG_PRINT_LEVEL);
    else {
        if (options.inter) set_print_level(INFO_PRINT_LEVEL);
        else set_print_level(MIN_PRINT_LEVEL);
    }
    ui_run(&ui_functions);
  }
    
  exit(0);
    
        
}
  
