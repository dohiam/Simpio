/*!
 * @file /device_keypad.h
 * @brief Simple keypad (matrix of switches) simulated device
 * @details
 * This configures and enables the simulated keypad device. This will add itself to the list of devices enabled in execution and ui (see execution.c and ui.c).
 * The enable function below is intended to be called by the parser when it encounters a command in the PIO program to configure this device
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef DEVICE_KEYPAD_H
#define DEVICE_KEYPAD_H

#include <stdint.h>

// from the keypad's perspective, row pins are input and column pins are driven based on keypress,
// so from the user's perspective, row pins are driven and column pins are checked for keypress connections
void device_enable_keypad(uint8_t r1_pin, uint8_t r2_pin, uint8_t r3_pin, uint8_t r4_pin, uint8_t c1_pin, uint8_t c2_pin, uint8_t c3_pin, uint8_t c4_pin);

void device_set_keypress(uint8_t key);
#endif
