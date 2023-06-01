/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    _DEFINE = 258,
    _PROGRAM = 259,
    _ORIGEN = 260,
    _WRAP_TARGET = 261,
    _WRAP = 262,
    _LANG_OPT = 263,
    _WORD = 264,
    _JMP = 265,
    _NOP = 266,
    _WAIT = 267,
    _IN = 268,
    _OUT = 269,
    _PUSH = 270,
    _PULL = 271,
    _MOV = 272,
    _SET = 273,
    _PUBLIC = 274,
    _OPT = 275,
    _GPIO = 276,
    _IRQ = 277,
    _NOT_X = 278,
    _NOT_Y = 279,
    _X_DECREMENT = 280,
    _Y_DECREMENT = 281,
    _X_NOT_EQUAL_Y = 282,
    _PIN = 283,
    _NOT_OSRE = 284,
    _SIDE = 285,
    _EOL = 286,
    _PINS = 287,
    _PINDIRS = 288,
    _X = 289,
    _Y = 290,
    _NULL = 291,
    _ISR = 292,
    _OSR = 293,
    _PC = 294,
    _EXEC = 295,
    _IFFULL = 296,
    _IFEMPTY = 297,
    _BLOCK = 298,
    _NOBLOCK = 299,
    _CLEAR = 300,
    _NOWAIT = 301,
    _RELATIVE = 302,
    _STATUS = 303,
    _AUTOPUSH = 304,
    _AUTOPULL = 305,
    _BANG = 306,
    _COLON = 307,
    _COLON_COLON = 308,
    _CONFIG = 309,
    _PIO = 310,
    _SM = 311,
    _PIN_CONDITION = 312,
    _SET_PINS = 313,
    _IN_PINS = 314,
    _OUT_PINS = 315,
    _SIDE_SET_PINS = 316,
    _SIDE_SET_COUNT = 317,
    _USER_PROCESSOR = 318,
    _RX_FIFO_MERGE = 319,
    _TX_FIFO_MERGE = 320,
    _SHIFTCTL_PULL_THRESH = 321,
    _SHIFTCTL_PUSH_THRESH = 322,
    _SHIFTCTL_OUT_SHIFTDIR = 323,
    _SHIFTCTL_IN_SHIFTDIR = 324,
    _BINARY_DIGIT = 325,
    _HEX_NUMBER = 326,
    _BINARY_NUMBER = 327,
    _DECIMAL_NUMBER = 328,
    _DELAY = 329,
    _SYMBOL = 330,
    _EXECCTRL_STATUS_SEL = 331,
    _TX_LEVEL = 332,
    _RX_LEVEL = 333,
    _WRITE = 334,
    _READ = 335,
    _VAR = 336,
    _HIGH = 337,
    _LOW = 338,
    _CONTINUE_USER = 339
  };
#endif
/* Tokens.  */
#define _DEFINE 258
#define _PROGRAM 259
#define _ORIGEN 260
#define _WRAP_TARGET 261
#define _WRAP 262
#define _LANG_OPT 263
#define _WORD 264
#define _JMP 265
#define _NOP 266
#define _WAIT 267
#define _IN 268
#define _OUT 269
#define _PUSH 270
#define _PULL 271
#define _MOV 272
#define _SET 273
#define _PUBLIC 274
#define _OPT 275
#define _GPIO 276
#define _IRQ 277
#define _NOT_X 278
#define _NOT_Y 279
#define _X_DECREMENT 280
#define _Y_DECREMENT 281
#define _X_NOT_EQUAL_Y 282
#define _PIN 283
#define _NOT_OSRE 284
#define _SIDE 285
#define _EOL 286
#define _PINS 287
#define _PINDIRS 288
#define _X 289
#define _Y 290
#define _NULL 291
#define _ISR 292
#define _OSR 293
#define _PC 294
#define _EXEC 295
#define _IFFULL 296
#define _IFEMPTY 297
#define _BLOCK 298
#define _NOBLOCK 299
#define _CLEAR 300
#define _NOWAIT 301
#define _RELATIVE 302
#define _STATUS 303
#define _AUTOPUSH 304
#define _AUTOPULL 305
#define _BANG 306
#define _COLON 307
#define _COLON_COLON 308
#define _CONFIG 309
#define _PIO 310
#define _SM 311
#define _PIN_CONDITION 312
#define _SET_PINS 313
#define _IN_PINS 314
#define _OUT_PINS 315
#define _SIDE_SET_PINS 316
#define _SIDE_SET_COUNT 317
#define _USER_PROCESSOR 318
#define _RX_FIFO_MERGE 319
#define _TX_FIFO_MERGE 320
#define _SHIFTCTL_PULL_THRESH 321
#define _SHIFTCTL_PUSH_THRESH 322
#define _SHIFTCTL_OUT_SHIFTDIR 323
#define _SHIFTCTL_IN_SHIFTDIR 324
#define _BINARY_DIGIT 325
#define _HEX_NUMBER 326
#define _BINARY_NUMBER 327
#define _DECIMAL_NUMBER 328
#define _DELAY 329
#define _SYMBOL 330
#define _EXECCTRL_STATUS_SEL 331
#define _TX_LEVEL 332
#define _RX_LEVEL 333
#define _WRITE 334
#define _READ 335
#define _VAR 336
#define _HIGH 337
#define _LOW 338
#define _CONTINUE_USER 339

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 109 "../src/simpio.y"

    int       ival;
    char      sval[32];  

#line 230 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
