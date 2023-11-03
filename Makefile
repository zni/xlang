.PHONY: all clean

all:
	if [ ! -d "bin" ]; then mkdir bin; fi
	${BISON_PATH}bison -d -o src/parser.tab.c src/parser.y
	${FLEX_PATH}flex -o src/lexer.yy.c src/lexer.l
	clang src/lexer.yy.c src/parser.tab.c ${CPPFLAGS} ${LDFLAGS} -lfl -o bin/parser

clean:
	rm src/lexer.yy.c
	rm src/parser.tab.c
	rm src/parser.tab.h
	rm bin/parser