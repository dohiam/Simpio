%{
/**********************************************************************************
 * SIMPIO LEXER:
 * 
 * 
 * 
 * 
 * 
 * 
 **********************************************************************************/
#include <stdio.h>
#include "constants.h"
#include "y.tab.h"
#include "instruction.h"
#include "print.h"
extern instruction_t ci;  /* owned by simpio.y */

/* todo: add all these to a project configuration file */

int line_count = 0;

int temp_i;

%}

%option yylineno

%s C_COMMENT


%%

^[ \t]+   { PRINTD("Ate leading spaces\n"); }

"/*"                     { BEGIN C_COMMENT; PRINTD("beginning C comment\n"); }
<C_COMMENT>"*/"          { BEGIN INITIAL; PRINTD("ending C comment\n"); }
<C_COMMENT>\n            { PRINTD("part of C comment - end line\n"); line_count++;}
<C_COMMENT>.             { PRINTD("part of C comment\n"); }

 /* C++ style comment */
\/\/.*                   { PRINTD("C++ comment\n"); }

 /* PIO style comment */
\;.*                     { PRINTD("PIO style comment\n"); }

,                        { PRINTD("Ignoring comma\n"); }

\n                       { return _EOL; } 

x--                      { return _X_DECREMENT; }
y--                      { return _Y_DECREMENT; }
!x                       { return _NOT_X; }
!y                       { return _NOT_Y; }
x!=y                     { return _X_NOT_EQUAL_Y; }
pin                      { return _PIN; }
!osre                    { return _NOT_OSRE; }

!                        { return _BANG; }
:                        { return _COLON; }
::                       { return _COLON_COLON; }

opt                      { return _OPT; }
pins                     { return _PINS; }
pindirs                  { return _PINDIRS; }
x                        { return _X; }
y                        { return _Y; }
null                     { return _NULL; }
isr                      { return _ISR; }
osr                      { return _OSR; }
pc                       { return _PC; }
exec                     { return _EXEC; }
clear                    { return _CLEAR; }
nowait                   { return _NOWAIT; }
rel                      { return _RELATIVE; }
status                   { return _STATUS; }

iffull                   { return _IFFULL; }
ifempty                  { return _IFEMPTY; }
block                    { return _BLOCK; }
noblock                  { return _NOBLOCK; }

\.define                 { PRINTD("define statement\n"); return _DEFINE; }
\.program                { PRINTD("program statement\n"); return _PROGRAM; }
\.origen                 { PRINTD("origen statement\n"); return _ORIGEN; }
\.wrap_target            { PRINTD("wrap target statement\n"); return _WRAP_TARGET; }
\.wrap                   { PRINTD("wrap statement\n"); return _WRAP; }
\.lang_opt               { PRINTD("lang_opt statement\n"); return _LANG_OPT; }
\.word                   { PRINTD("word statement\n"); return _WORD; }
\.data                   { PRINTD("data statement\n"); return _DATA_CONFIG; }
\.device                 { PRINTD("device statement\n"); return _DEVICE; }
spi_flash                { PRINTD("spi flash device\n"); return _SPI_FLASH; }

\.config                 { PRINTD("config statement\n"); return _CONFIG; }
pio                      { return _PIO; }
sm                       { return _SM; }
jmp_pin                  { return _PIN_CONDITION; }
set_pins                 { return _SET_PINS; }
in_pins                  { return _IN_PINS; }
out_pins                 { return _OUT_PINS; }
side_set_pins            { return _SIDE_SET_PINS; }
side_set_count           { return _SIDE_SET_COUNT; }
shiftctl_out             { return _SHIFTCTL_OUT; }
shiftctl_in              { return _SHIFTCTL_IN; }
execctrl_status_sel      { return _EXECCTRL_STATUS_SEL; }
tx_level                 { return _TX_LEVEL; }
rx_level                 { return _RX_LEVEL; }
user_processor           { return _USER_PROCESSOR; }
fifo_merge               { return _FIFO_MERGE; }
clkdiv                   { return _CLKDIV; }
serial                   { return _SERIAL; }
rs232                    { return _RS232; }
usb                      { return _USB; }


jmp                      { return _JMP; }
nop                      { return _NOP; }
wait                     { return _WAIT; }
in                       { return _IN; }
out                      { return _OUT; }
push                     { return _PUSH; }
pull                     { return _PULL; }
mov                      { return _MOV; }
irq                      { return _IRQ; }
set                      { return _SET; }

write                    { return _WRITE; }
read                     { return _READ; }
repeat                   { return _REPEAT; }
print                    { return _PRINT; }
data                     { return _DATA; }
exit                     { return _EXIT; }
readln                   { return _READLN; }
user_var                 { return _VAR; }
high                     { return _HIGH; }
low                      { return _LOW;  }
\[->\]                   { return _CONTINUE_USER; }

side                     { return _SIDE; }
gpio                     { return _GPIO; }

PUBLIC                   { return _PUBLIC; }

0x[0-9a-fA-F]+           { yylval.ival = strtol(yytext+2, NULL, 16); PRINTD("Hex string: %s, value:%0x\n", yytext+2, yylval.ival); return _HEX_NUMBER; }
0b[0-1]+                 { yylval.ival = strtol(yytext+2, NULL, 2); return _BINARY_NUMBER; }
[0-1]                    { yylval.ival = strtol(yytext, NULL, 10); return _BINARY_DIGIT; }
[0-9]+                   { yylval.ival = strtol(yytext, NULL, 10); return _DECIMAL_NUMBER; }

[A-Za-z][0-9A-Za-z_]*    { PRINTD("Symbol:'%s'\n",yytext); snprintf(yylval.sval, SYMBOL_MAX, "%s", yytext); return _SYMBOL; }

\"([^\\\"]|\\.)*\"       { PRINTD("Quoted String:'%s'",yytext); snprintf(yylval.strval, SYMBOL_MAX, "%s", &(yytext[1])); yylval.strval[yyleng-2]='\0'; return _STRING;}

\[[0-9]+\]               { temp_i = strlen(yytext); yytext[temp_i-1]=0; yylval.ival = strtol(yytext+1, NULL, 10); PRINTD("Delay:'%d'",yylval.ival); return _DELAY; }


%%  