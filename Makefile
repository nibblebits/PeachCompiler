OBJECTS= ./build/compiler.o ./build/cprocess.o ./build/rdefault.o \
			./build/lexer.o ./build/token.o ./build/lex_process.o \
			./build/parser.o ./build/scope.o ./build/symresolver.o \
			./build/codegen.o ./build/stackframe.o ./build/resolver.o \
			./build/fixup.o ./build/array.o ./build/datatype.o \
			./build/node.o ./build/expressionable.o ./build/helper.o \
			./build/helpers/buffer.o ./build/helpers/vector.o ./build/preprocessor.o
INCLUDES= -I./
CC = gcc
CFLAGS = -Wall -Wno-cpp

all: ${OBJECTS}
	$(CC) $(CFLAGS) main.c ${INCLUDES} ${OBJECTS} -g -o ./main

./build/compiler.o: ./compiler.c
	$(CC) $(CFLAGS) compiler.c  ${INCLUDES} -o ./build/compiler.o -g -c

./build/cprocess.o: ./cprocess.c
	$(CC) $(CFLAGS) cprocess.c ${INCLUDES} -o ./build/cprocess.o -g -c

./build/rdefault.o: ./rdefault.c
	$(CC) $(CFLAGS) rdefault.c ${INCLUDES} -o ./build/rdefault.o -g -c

./build/lexer.o: ./lexer.c
	$(CC) $(CFLAGS) lexer.c ${INCLUDES} -o ./build/lexer.o -g -c

./build/token.o: ./token.c
	$(CC) $(CFLAGS) token.c ${INCLUDES} -o ./build/token.o -g -c

./build/lex_process.o: ./lex_process.c
	$(CC) $(CFLAGS) lex_process.c ${INCLUDES} -o ./build/lex_process.o -g -c

./build/parser.o: ./parser.c
	$(CC) $(CFLAGS) parser.c ${INCLUDES} -o ./build/parser.o -g -c

./build/node.o: ./node.c
	$(CC) $(CFLAGS) node.c ${INCLUDES} -o ./build/node.o -g -c

./build/scope.o: ./scope.c
	$(CC) $(CFLAGS) scope.c ${INCLUDES} -o ./build/scope.o -g -c

./build/symresolver.o: ./symresolver.c
	$(CC) $(CFLAGS) symresolver.c ${INCLUDES} -o ./build/symresolver.o -g -c

./build/codegen.o: ./codegen.c
	$(CC) $(CFLAGS) codegen.c ${INCLUDES} -o ./build/codegen.o -g -c

./build/stackframe.o: ./stackframe.c
	$(CC) $(CFLAGS) stackframe.c ${INCLUDES} -o ./build/stackframe.o -g -c

./build/resolver.o: ./resolver.c
	$(CC) $(CFLAGS) resolver.c ${INCLUDES} -o ./build/resolver.o -g -c

./build/fixup.o: ./fixup.c
	$(CC) $(CFLAGS) fixup.c $(INCLUDES) -o ./build/fixup.o -g -c

./build/array.o: ./array.c
	$(CC) $(CFLAGS) array.c ${INCLUDES} -o ./build/array.o -g -c

./build/expressionable.o: ./expressionable.c
	$(CC) $(CFLAGS) expressionable.c ${INCLUDES} -o ./build/expressionable.o -g -c

./build/helper.o: ./helper.c
	$(CC) $(CFLAGS) helper.c ${INCLUDES} -o ./build/helper.o -g -c

./build/datatype.o: ./datatype.c
	$(CC) $(CFLAGS) datatype.c ${INCLUDES} -o ./build/datatype.o -g -c

./build/preprocessor.o: ./preprocessor/preprocessor.c
	$(CC) $(CFLAGS) ./preprocessor/preprocessor.c ${INCLUDES} -o ./build/preprocessor.o -g -c

./build/helpers/buffer.o: ./helpers/buffer.c
	$(CC) $(CFLAGS) ./helpers/buffer.c ${INCLUDES} -o ./build/helpers/buffer.o -g -c

./build/helpers/vector.o: ./helpers/vector.c
	$(CC) $(CFLAGS) ./helpers/vector.c ${INCLUDES} -o ./build/helpers/vector.o -g -c

clean:
	rm -f ./main
	rm -rf ${OBJECTS}