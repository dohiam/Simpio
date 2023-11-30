####################################################################################################################################
#
# Makefile for the Simpio project:
#
# Generally all that should be needed for general development on this project is to update the list of source files in C_SOURCES.
# And in case is this Makefile is not executed from the build subdirectory, then modify the SRC and INC paths as needed too.
#
# Note that there are some build options that one can choose between:
# a) no debug info, or debug info for gdb (for debugging) and gcov (for test coverage)
# b) dynamic linking, or static linking
#
# The targets should not need to be altered unless new languages (beyond C) or new tools are used beyond LEX and YACC.
# Note that the targets include automaticall header file dependency updating (which introduces new project dependency on sed).
#
####################################################################################################################################

############################################
# INPUTS
############################################

C_SOURCES = device_spi_flash.c device_keypad.c editor.c execution.c fifo.c hardware.c hardware_changed.c instruction.c main.c print.c symbols.c ui.c

SRC = ../src
INC = ../inc

############################################
# BUILD OPTIONS (debug, static link, ...)
############################################

# no debug info (note: debug info left on yacc to assist user in debugging syntax issues)
CC = gcc -I ${INC} -Werror 
LEX = lex -i 
YACC = yacc --debug --verbose -d

# static link
LD = gcc -static-libgcc -static
LIB =  -l:libncursesw.a -l:libtinfo.a

# debug info
#CC = gcc -I ${INC} -DSYNTAX_DEBUG=1 -ggdb -g3 -O0 -Werror -fprofile-arcs -ftest-coverage -fprofile-generate
#LEX = lex -i 
#YACC = yacc --debug --verbose -d  

# dynamic link
#LD = ${CC}   -fprofile-arcs  -fprofile-generate
#LIB =  -lncurses -ll  -lgcov

############################################
# TARGETS
############################################

# create list of object files by substituting all C files in C_SOURCES with .o extension and adding the lexx/yacc ones
OBJS := ${C_SOURCES:.c=.o} parser.o lexer.o

# the main target rule to create the simpio executable based on all object files (and copy to the tests directory)
simpio: $(OBJS)
	${LD} ${OBJS} ${LIB} -o simpio
	cp simpio ../tests/simpio

# include all dependency files (substituting .d for all .c in sources) which will trigger creating dependency files as needed
include $(C_SOURCES:.c=.d)

# explicit rules lexx/yacc stuff:
parser.o: y.tab.c
	${CC} -c  y.tab.c -o parser.o 
lexer.o: lex.yy.c 
	${CC} -c lex.yy.c -o lexer.o 
lex.yy.c: y.tab.c ${SRC}/simpio.l 
	${LEX} --i ${SRC}/simpio.l
y.tab.c: ${SRC}/simpio.y ${HEADERS}
	${YACC} ${SRC}/simpio.y

# basic rule pattern for compiling C files
%.o: ${SRC}/%.c
	${CC} -c $< -o $@

# magic gnu-recommended rule for creating dependency files 
# modified to reference the source directory
%.d: ${SRC}/%.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# alternate target to remove all generated files, including code coverage ones
clean:  
	rm -f simpio
	rm -f ${OBJS}
	rm -f y.output y.tab.h y.tab.c lex.yy.c
	rm -f *.d
	rm -f *.gcno
	rm -f *.gcda
	rm -f *.gcov
    
