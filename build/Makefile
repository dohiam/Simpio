SRC = ../src
INC = ../inc

HEADERS = ${INC}/editor.h ${INC}/enumerator.h ${INC}/execution.h ${INC}/fifo.h ${INC}/hardware.h ${INC}/hardware_changed.h ${INC}/instruction.h ${INC}/parser.h ${INC}/print.h ${INC}/ui.h ${INC}/devices.h

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

 
simpio: main.o execution.o instruction.o hardware.o fifo.o hardware_changed.o editor.o ui.o print.o parser.o lexer.o symbols.o devices.o
	${LD} main.o execution.o instruction.o hardware.o fifo.o hardware_changed.o editor.o ui.o print.o parser.o lexer.o -o simpio symbols.o devices.o ${LIB} 
	cp simpio ../tests/simpio
    
devices.o: ${SRC}/devices.c ${HEADERS}
	${CC} -c ${SRC}/devices.c -o devices.o

symbols.o: ${SRC}/symbols.c ${INC}/symbols.h
	${CC} -c ${SRC}/symbols.c -o symbols.o

main.o: ${SRC}/main.c ${HEADERS}
	${CC} -c ${SRC}/main.c -o main.o
    
execution.o: ${SRC}/execution.c ${HEADERS}
	${CC} -c ${SRC}/execution.c -o execution.o

instruction.o: ${SRC}/instruction.c ${HEADERS}
	${CC} -c ${SRC}/instruction.c -o instruction.o

hardware.o: ${SRC}/hardware.c ${HEADERS}
	${CC} -c ${SRC}/hardware.c -o hardware.o

hardware_changed.o: ${SRC}/hardware_changed.c ${HEADERS}
	${CC} -c ${SRC}/hardware_changed.c -o hardware_changed.o

fifo_test: ${SRC}/fifo_test.c ${INC}/fifo.h fifo.o
	${CC} ${SRC}/fifo_test.c fifo.o -o fifo_test

fifo.o: ${SRC}/fifo.c ${HEADERS} 
	${CC} -c ${SRC}/fifo.c -o fifo.o

editor.o: ${SRC}/editor.c ${HEADERS}
	${CC} -c ${SRC}/editor.c -o editor.o

ui.o: ${SRC}/ui.c ${HEADERS}
	${CC} -c ${SRC}/ui.c -o ui.o

print.o: ${SRC}/print.c ${HEADERS}
	${CC} -c ${SRC}/print.c -o print.o

parser.o: y.tab.c
	${CC} -c  y.tab.c -o parser.o 
	
lexer.o: lex.yy.c 
	${CC} -c lex.yy.c -o lexer.o 
	
lex.yy.c: y.tab.c ${SRC}/simpio.l 
	${LEX} --i ${SRC}/simpio.l

y.tab.c: ${SRC}/simpio.y ${HEADERS}
	${YACC} ${SRC}/simpio.y
    
clean:  
	rm -f lex.yy.c y.tab.c y.tab.h y.output ui.o editor.o hardware.o instruction.o execution.o hardware_changed.o lexer.o parser.o fifo.o print.o symbols.o devices.o main.o simpio
	rm -f *.gcno
	rm -f *.gcda
	rm -f *.gcov
    
