/*!
 * @file /device_keypad.c
 * @brief Simulated keypad
 * @details
 * This emulates a keypad, i.e., a small matrix of switches connected in a row/column format. See the PIO programming guide
 * as part of this project for more information on this kind of device and how to use it.
 * As all simulated devices in Simpio, once enabled, this exposes a state machine execution function that is called
 * periodically by the Simpio execution engine (see execution.c). Each time this function is called, it looks at the status
 * of the simulated input GPIO lines that it is configured to use, and updates its internal state and possible fiddles the state of
 * output GPIO lines that it is configured to use. 
 *
 * There is a rudimentary UI programmed using the temp_window from ui.c. This allows a user to select a simulated key to simulate 
 * being pressed. 
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#include "device_keypad.h"
#include "hardware.h"
#include "print.h"
#include "ui.h"

/*****************************************************************
 *
 *  KEYPAD DEVICE
 *
 *****************************************************************/

static uint8_t row_pins[4];
static uint8_t col_pins[4];
static int8_t keypress_row, keypress_col;

static void _set_keypress(char ch) {
   switch (ch) {
		case ' ':  keypress_row = keypress_col = -1; break;
		case '1':  keypress_row = 0; keypress_col = 0; break;
		case '2':  keypress_row = 0; keypress_col = 1; break;
		case '3':  keypress_row = 0; keypress_col = 2; break;
		case 'A':  keypress_row = 0; keypress_col = 3; break;
		case '4':  keypress_row = 1; keypress_col = 0; break;
		case '5':  keypress_row = 1; keypress_col = 1; break;
		case '6':  keypress_row = 1; keypress_col = 2; break;
		case 'B':  keypress_row = 1; keypress_col = 3; break;
		case '7':  keypress_row = 2; keypress_col = 0; break;
		case '8':  keypress_row = 2; keypress_col = 1; break;
		case '9':  keypress_row = 2; keypress_col = 2; break;
		case 'C':  keypress_row = 2; keypress_col = 3; break;
		case '*':  keypress_row = 3; keypress_col = 0; break;
		case '0':  keypress_row = 3; keypress_col = 1; break;
		case '#':  keypress_row = 3; keypress_col = 2; break;
		case 'D':  keypress_row = 3; keypress_col = 3; break;
	};
 }

int display_keypad_state() {
    int ch;
    do {
        werase(temp_window);
        if (keypress_row < 0) { ui_temp_window_write("no key currently pressed\n"); }
        else ui_temp_window_write("current keypress is row: %d col: %d\n", keypress_row+1, keypress_col+1);
        ui_temp_window_write("\n\npress key on keyboard from following table to simulate a keypress\n");
        ui_temp_window_write("or press space bar for no key pressed\n");
        ui_temp_window_write("hit q to quit\n\n");
        ui_temp_window_write("              COL 1    COL 2    COL 3    COL 4\n");
        ui_temp_window_write("     ROW 1     1        2        3        A   \n");
        ui_temp_window_write("     ROW 2     4        5        6        B   \n");
        ui_temp_window_write("     ROW 3     7        8        9        C   \n");
        ui_temp_window_write("     ROW 4     *        0        #        D   \n");
        ch = getch();
		if (ch == 'q' || ch == 'Q') return 0;
		_set_keypress(ch);
    } while (ch != 'q' && ch != 'Q');
    return 0;
}

void run_keypad() {
    int i;
    bool v;
    for (i=0; i<4; i++) hardware_set_gpio(col_pins[i],0);
    for (i=0; i<4; i++) {
        v = hardware_get_gpio(row_pins[i]);
        if (v && keypress_row == i) {
            hardware_set_gpio(col_pins[keypress_col], 1);
            PRINTI("set key row pin %d col pin %d\n", row_pins[i], col_pins[keypress_col]);
        }
    }
}

void device_enable_keypad(uint8_t r1_pin, uint8_t r2_pin, uint8_t r3_pin, uint8_t r4_pin, uint8_t c1_pin, uint8_t c2_pin, uint8_t c3_pin, uint8_t c4_pin) {
    keypress_row = keypress_col = -1;
    row_pins[0] = r1_pin;
    row_pins[1] = r2_pin;
    row_pins[2] = r3_pin;
    row_pins[3] = r4_pin;
    col_pins[0] = c1_pin;
    col_pins[1] = c2_pin;
    col_pins[2] = c3_pin;
    col_pins[3] = c4_pin;
    hardware_register_device("keypad", true, run_keypad, display_keypad_state);
}

void device_set_keypress(uint8_t key) {
	if ((0 <= key) && (key <=9)) {
		_set_keypress (key + 48);
	}
}

