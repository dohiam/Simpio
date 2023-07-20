#include "devices.h"
#include "hardware.h"
#include "print.h"
#include "ui.h"

//TODO: overview (could be more clever but hopefully not more straightforward)

#define BYTE_RECEIVED (spif_state.shift_count == 8)
#define BYTE_SENT     (spif_state.shift_count == 8)

#define FLASH_CMD_PAGE_PROGRAM               0x02
#define FLASH_CMD_READ                       0x03
#define FLASH_CMD_STATUS                     0x05
#define FLASH_CMD_WRITE_EN                   0x06
#define FLASH_CMD_SECTOR_ERASE               0x20
#define FLASH_CMD_READ_MANUFACTURER_ID       0X90

static uint spif_clk, spif_tx, spif_rx, spif_cs;

typedef enum { spif_idle, spif_getting_cmd, spif_getting_addr1, spif_getting_addr2, spif_getting_addr3, spif_programming, spif_reading, spif_writing_response, spif_processing_cmd, spif_done } spif_state_e;

typedef enum { spif_mode_0, spif_mode_3 } spif_mode_e;

typedef enum {spif_shift_waiting_on_clk_low_then_high, spif_shift_waiting_on_clk_high, spif_shift_waiting_on_clk_high_then_low, spif_shift_waiting_on_clk_low } spif_shift_state_e;

static uint8_t spif_id[] = { 0xAB, 0XCD };

static uint8_t spif_storage[SPI_FLASH_SIZE];

typedef struct {
    spif_state_e        state;
    spif_mode_e         mode;
    spif_shift_state_e  shift_state;
    uint8_t             cmd;
    uint8_t             prev_cmd;
    uint8_t             addr1;
    uint8_t             addr2;
    uint8_t             addr3;
    uint8_t             status_register_1;
    uint32_t            addr;
    uint8_t             response_byte;
    uint8_t             program_byte;
    uint8_t             shift_count;
    uint32_t            num_bytes;
    uint32_t            bytes_responded;
    uint32_t            bytes_received;
    uint32_t            byte_index;
    uint32_t            delay;                  // will report busy until this many times called
    uint8_t *           data_ptr;
    bool                last_clk;
    bool                busy;                   // bit 0 of status register 1
    bool                write_enable_latch;     // bit 1 of status register 1
} spif_state_t;

static spif_state_t spif_state;

/*****************************************************************
 *
 *  SPI FLASH UTILITY ROUTINES
 *
 *****************************************************************/
                
void spif_shift_in_next_bit(uint8_t * byte) {
    bool clk = hardware_get_gpio(spif_clk);
    switch (spif_state.shift_state) {
         case spif_shift_waiting_on_clk_low_then_high:
            if (!clk) spif_state.shift_state = spif_shift_waiting_on_clk_high;
            break;
        case spif_shift_waiting_on_clk_high:
            if (clk) {
                *byte = *byte << 1;
                *byte = *byte + hardware_get_gpio(spif_tx);
                spif_state.shift_count++;
                spif_state.shift_state = spif_shift_waiting_on_clk_low_then_high;
            }
            break;            
    };
}

void spif_shift_out_next_bit(uint8_t * byte) {
    bool clk = hardware_get_gpio(spif_clk);
    switch (spif_state.shift_state) {
        case spif_shift_waiting_on_clk_high_then_low:
            if (clk) spif_state.shift_state = spif_shift_waiting_on_clk_low;
            break;
        case spif_shift_waiting_on_clk_low:
            if (!clk) {
                if (*byte & 0x80) hardware_set_gpio(spif_rx, 1);
                else hardware_set_gpio(spif_rx, 0);
                *byte = *byte << 1;
                spif_state.shift_count++;
                spif_state.shift_state = spif_shift_waiting_on_clk_high_then_low;
            }
            break;            
    };
}
    
uint32_t spif_get_addr() {
    uint32_t addr;
    addr = spif_state.addr1;
    addr = (addr << 8) + spif_state.addr2;
    addr = (addr << 8) + spif_state.addr3;
}

void spif_reset_for_next_cmd() {
   spif_state.prev_cmd   = spif_state.cmd;
   spif_state.cmd   = 0;
   spif_state.addr1 = 0;
   spif_state.addr2 = 0;
   spif_state.addr3 = 0;
   spif_state.addr  = 0;
   spif_state.response_byte = 0;
   spif_state.program_byte = 0;
   spif_state.shift_count = 0;
   spif_state.bytes_responded = 0;
   spif_state.bytes_received = 0;
   spif_state.byte_index = 0;
   spif_state.data_ptr = NULL;
}

    
/*****************************************************************
 *
 *  SPI FLASH DEVICE MANAGEMENT (HANDLERS, ETC.)
 *
 *****************************************************************/
                
void spif_sm();

int display_spi_flash_state() {
    int ch, i,j;
    werase(temp_window);
    ui_temp_window_write("clk pin(%d) = %d\n", spif_clk, hardware_get_gpio(spif_clk));
    ui_temp_window_write("tx  pin(%d) = %d\n", spif_tx, hardware_get_gpio(spif_tx));
    ui_temp_window_write("rx  pin(%d) = %d\n", spif_rx, hardware_get_gpio(spif_rx));
    ui_temp_window_write("cs  pin(%d) = %d\n", spif_cs, hardware_get_gpio(spif_cs));
    ui_temp_window_write("state  = ");
    switch(spif_state.state) {
        case spif_idle:             ui_temp_window_write("idle\n");                     break;
        case spif_getting_cmd:      ui_temp_window_write("getting command\n");          break;
        case spif_getting_addr1:    ui_temp_window_write("getting address byte 1\n");   break;
        case spif_getting_addr2:    ui_temp_window_write("getting address byte 2\n");   break;
        case spif_getting_addr3:    ui_temp_window_write("getting address byte 3\n");   break;
        case spif_programming:      ui_temp_window_write("writing data\n");             break;
        case spif_reading:          ui_temp_window_write("reading data\n");             break;
        case spif_writing_response: ui_temp_window_write("writing response\n");         break;
        case spif_processing_cmd:   ui_temp_window_write("processing command\n");       break;
        case spif_done:             ui_temp_window_write("done\n");       break;
    };
    switch (spif_state.shift_state) {
        case spif_shift_waiting_on_clk_low_then_high: ui_temp_window_write("waiting on clk low then high");  break;
        case spif_shift_waiting_on_clk_high:          ui_temp_window_write("waiting on clk high");           break;
        case spif_shift_waiting_on_clk_high_then_low: ui_temp_window_write("waiting on clk high then low");  break;
        case spif_shift_waiting_on_clk_low:          ui_temp_window_write("waiting on clk low");             break;

    };
    ui_temp_window_write("\n");
    ui_temp_window_write("current cmd  = %02X\n", spif_state.cmd);
    ui_temp_window_write("prev cmd  = %02X\n", spif_state.prev_cmd);
    ui_temp_window_write("shift count  = %d\n", spif_state.shift_count);
    ui_temp_window_write("last clk pin: %d\n", spif_state.last_clk);
    ui_temp_window_write("status register 1: %d\n", spif_state.status_register_1);
    spif_get_addr();
    ui_temp_window_write("address: %d\n", spif_state.addr);
    ui_temp_window_write("current response: %02X\n", spif_state.response_byte);
    ui_temp_window_write("bytes responded so far: %d\n", spif_state.bytes_responded);
    ui_temp_window_write("byte to program: %02X\n", spif_state.program_byte);
    ui_temp_window_write("byte received so far: %d\n", spif_state.bytes_received);
    ui_temp_window_write("num bytes expected: %d\n", spif_state.num_bytes);
    ui_temp_window_write("byte index: %d\n", spif_state.byte_index);
    if (spif_state.busy) {ui_temp_window_write("device is busy for %d more cycles\n", spif_state.delay);}
    else ui_temp_window_write("device is idle\n");
    if (spif_state.write_enable_latch) {ui_temp_window_write("device is enabled for write\n");}
    else ui_temp_window_write("device is not enabled for write\n");
    ui_temp_window_write("simulated flash storage (first %d bytes):\n", SPI_FLASH_DISPLAY_LINES * SPI_FLASH_DISPLAY_LINE_SIZE);
    for (i=0; i < SPI_FLASH_DISPLAY_LINES; i++) {
        for (j=0; j < SPI_FLASH_DISPLAY_LINE_SIZE; j++) {
            ui_temp_window_write("%02X ", spif_storage[ (i*SPI_FLASH_DISPLAY_LINE_SIZE) +j]);
        }
        ui_temp_window_write("\n");
    }
    ui_temp_window_write("\npress PF6 to step next instruction: %d, 2-9 to run iterations, else qq to exit\n", spif_state.last_clk);
    ch = getch();
    if ('q' == ch) return 0;
    if ('2' <= ch && ch <= '9') return (ch - '0');
    if (ch == KEY_F(6)) return 1;
    return 0;
}
        
void run_spi_flash() {
    int rc;
    spif_sm();
}

void devices_enable_spi_flash(uint8_t clk_pin, uint8_t tx_pin, uint8_t rx_pin, uint8_t cs_pin) {
    PRINTI("enabling spi flash\n");
    spif_clk = clk_pin;
    spif_tx = tx_pin;
    spif_rx = rx_pin;
    spif_cs = cs_pin;
    hardware_register_device("spi flash", true, run_spi_flash, display_spi_flash_state);
    spif_state.busy = false;
    spif_state.write_enable_latch = false;
    spif_state.delay = 0;
    spif_state.cmd = 0;
    spif_reset_for_next_cmd();
}

/*****************************************************************
 *
 *  SPI FLASH DEVICE PROCESSING
 *
 *****************************************************************/
                
void spif_stay_busy_for(uint32_t num_cycles) {
    spif_state.delay = num_cycles;
    spif_state.busy = true;
}
    
bool spif_finish() {
    if (hardware_get_gpio(spif_cs)) {
        spif_state.state = spif_idle;
        spif_reset_for_next_cmd();
        return true;
    }
    else return false;
}
    
void spif_setup_response(uint8_t * data2write, uint32_t num_bytes) {
    spif_state.state = spif_writing_response;
    spif_state.data_ptr = data2write;
    spif_state.response_byte = data2write[0];
    spif_state.byte_index = 0;
    spif_state.num_bytes = num_bytes;
    spif_state.shift_count = 0;
    spif_state.shift_state = spif_shift_waiting_on_clk_low;
}

void spif_response() {
    spif_shift_out_next_bit(&(spif_state.response_byte));
    if BYTE_SENT {
        spif_state.bytes_responded++;
        if (spif_state.bytes_responded == spif_state.num_bytes) {
            spif_state.state = spif_done;
        }
        else {
            spif_state.response_byte = spif_state.data_ptr[spif_state.bytes_responded];
            spif_state.shift_count = 0;
        }
    }
}
    
void spif_setup_read() {
    spif_state.state = spif_reading;
    spif_state.addr = spif_get_addr();
    spif_state.data_ptr = &(spif_state.response_byte);
    spif_state.response_byte = spif_storage[(spif_state.addr)++];
    spif_state.shift_count = 0;
    spif_state.shift_state = spif_shift_waiting_on_clk_low;
}
    
void spif_read() {
    if (spif_state.addr >= SPI_FLASH_SIZE) {
        spif_state.response_byte = 0xff;
    }
    else {
        spif_state.response_byte = spif_storage[(spif_state.addr)++];
    }
    (spif_state.bytes_responded)++;
    spif_state.shift_count = 0;
}

void spif_setup_program() {
    spif_state.state = spif_programming;
    spif_state.addr = spif_get_addr();
    spif_state.data_ptr = &(spif_state.program_byte);
    spif_state.shift_count = 0;
    spif_state.shift_state = spif_shift_waiting_on_clk_low_then_high;
}
    
void spif_program() {
    if (spif_state.addr >= SPI_FLASH_SIZE) return;
    spif_storage[(spif_state.addr)++] = spif_state.program_byte;
    spif_state.shift_count = 0;
}

void spif_setup_sector_erase() {
    // realistically, only one sector (sector zero) will probably ever be supported by this simulator
    uint32_t addr, first, max, i;
    addr = spif_get_addr();
    first = addr * SPI_SECTOR_SIZE;
    if ( first > SPI_FLASH_SIZE ) return; 
    max = (addr+1) * SPI_SECTOR_SIZE;
    if (max > SPI_FLASH_SIZE) max = SPI_FLASH_SIZE;
    for (i=first; i<max; i++) spif_storage[i] = 0xFF;
}
    
void spif_setup_write_enable() {
    spif_state.write_enable_latch = true;
    spif_state.state = spif_done;
}
    
void spif_setup_read_manufacturer_id() {
    spif_state.state = spif_getting_addr1;
    spif_state.shift_count = 0;
    spif_state.shift_state = spif_shift_waiting_on_clk_low_then_high;
}
    
/*****************************************************************
 *
 *  SPI FLASH STATE MACHINE
 *
 *****************************************************************/
                
void spif_process_cmd() {
    switch(spif_state.cmd) {
        case FLASH_CMD_PAGE_PROGRAM:
        case FLASH_CMD_READ:
        case FLASH_CMD_SECTOR_ERASE:
        case FLASH_CMD_READ_MANUFACTURER_ID:
            spif_state.state = spif_getting_addr1;
            spif_state.shift_count = 0;
            spif_state.shift_state = spif_shift_waiting_on_clk_low_then_high;
            break;
        case FLASH_CMD_STATUS:
            spif_state.status_register_1 = spif_state.write_enable_latch;
            spif_state.status_register_1 = (spif_state.status_register_1 << 1) + spif_state.busy;
            spif_setup_response(&(spif_state.status_register_1), 1);
            break;
        case FLASH_CMD_WRITE_EN:
            spif_setup_write_enable();
            break;
        default:
            PRINT("unknown cmd %02X\n", spif_state.cmd);
            spif_state.shift_count = 0;
            spif_state.state = spif_done;
            break;
    };
}

void spif_process_addr1() {
    spif_state.state = spif_getting_addr2;
    spif_state.shift_count = 0;
    spif_state.shift_state = spif_shift_waiting_on_clk_low_then_high;
}

void spif_process_addr2() {
    spif_state.state = spif_getting_addr3;
    spif_state.shift_count = 0;
    spif_state.shift_state = spif_shift_waiting_on_clk_low_then_high;
}

void spif_process_addr3() {
    switch(spif_state.cmd) {
        case FLASH_CMD_PAGE_PROGRAM:
            spif_setup_program();
            break;
        case FLASH_CMD_READ:
            spif_setup_read();
            break;
        case FLASH_CMD_SECTOR_ERASE:
            spif_setup_sector_erase();
            break;
        case FLASH_CMD_READ_MANUFACTURER_ID:
            spif_setup_response(spif_id, 2);
            break;
        case FLASH_CMD_WRITE_EN:
        case FLASH_CMD_STATUS:
            PRINT("unexpected cmd with address %02X\n", spif_state.cmd);
            spif_state.shift_count = 0;
            spif_state.state = spif_done;
            break;
        default:
            PRINT("unknown cmd %02X\n", spif_state.cmd);
            spif_state.shift_count = 0;
            spif_state.state = spif_done;
            break;
    };
}

/*****************************************************************
 *
 *  SPI WHEN TRANSFERRING DATA WITH HOST
 *
 *****************************************************************/

// come out of idle when cs goes low
void spif_when_idle() {
    if (!hardware_get_gpio(spif_cs)) {
        spif_state.state = spif_getting_cmd;
        spif_state.last_clk = hardware_get_gpio(spif_clk);
        if (spif_state.last_clk) {
            spif_state.mode = spif_mode_3;
            spif_state.shift_state = spif_shift_waiting_on_clk_low_then_high;
        }
        else {
            spif_state.mode = spif_mode_0;
            spif_state.shift_state = spif_shift_waiting_on_clk_high;
        }
        spif_state.shift_count = 0;
    }
}

void spif_when_getting_cmd() {
    spif_shift_in_next_bit(&(spif_state.cmd));
    if BYTE_RECEIVED {
        spif_process_cmd();
    }
}

void spif_when_getting_addr1() {
    spif_shift_in_next_bit(&(spif_state.addr1));
    if BYTE_RECEIVED {
        spif_process_addr1();
    }
}

void spif_when_getting_addr2() {
    spif_shift_in_next_bit(&(spif_state.addr2));
    if BYTE_RECEIVED {
        spif_process_addr2();
    }
}

void spif_when_getting_addr3() {
    spif_shift_in_next_bit(&(spif_state.addr3));
    if BYTE_RECEIVED {
        spif_process_addr3();
    }
}

void spif_when_programming() {
    spif_shift_in_next_bit(&(spif_state.program_byte));
    if BYTE_RECEIVED {
        spif_program();
    }
}

void spif_when_reading() {
    spif_shift_out_next_bit(&(spif_state.response_byte));
    if BYTE_SENT {
        spif_read();
    }
}

void spif_when_writing_response() {
    spif_shift_out_next_bit(&(spif_state.response_byte));
    if BYTE_SENT {
        spif_response();
    }
}

void spif_when_processing_cmd() {
}

void spif_sm() {
    if (spif_state.delay > 0) {
        if (--spif_state.delay == 0) spif_state.busy = false;
    }
    if (spif_finish()) return;
    switch (spif_state.state) {
        case spif_idle:               spif_when_idle();           break;
        case spif_getting_cmd:        spif_when_getting_cmd();    break;
        case spif_getting_addr1:      spif_when_getting_addr1();  break;
        case spif_getting_addr2:      spif_when_getting_addr2();  break;
        case spif_getting_addr3:      spif_when_getting_addr3();  break;
        case spif_programming:        spif_when_programming();    break;
        case spif_reading:            spif_when_reading();        break;
        case spif_writing_response:   spif_when_writing_response();   break;
        case spif_processing_cmd:     spif_when_processing_cmd(); break;
        case spif_done:               break;
    };
}
        

