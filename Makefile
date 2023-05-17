OBJECTS= ./build/compiler.o ./build/cprocess.o ./build/validator.o ./build/rdefault.o ./build/lexer.o ./build/token.o ./build/lex_process.o ./build/parser.o ./build/scope.o ./build/symresolver.o ./build/codegen.o ./build/stackframe.o ./build/resolver.o ./build/fixup.o ./build/array.o ./build/datatype.o ./build/node.o ./build/expressionable.o ./build/helper.o ./build/helpers/buffer.o ./build/helpers/vector.o ./build/preprocessor/preprocessor.o ./build/preprocessor/static-include.o ./build/preprocessor/static-includes/stdarg.o ./build/preprocessor/static-includes/stddef.o ./build/preprocessor/native.o
INCLUDES= -I./

all: ${OBJECTS}
	gcc main.c ${INCLUDES} ${OBJECTS} -g -o ./main 

./build/compiler.o: ./compiler.c
	gcc compiler.c  ${INCLUDES} -o ./build/compiler.o -g -c

./build/cprocess.o: ./cprocess.c
	gcc cprocess.c ${INCLUDES} -o ./build/cprocess.o -g -c

./build/validator.o: ./validator.c
	gcc validator.c ${INCLUDES} -o ./build/validator.o -g -c


./build/rdefault.o: ./rdefault.c
	gcc rdefault.c ${INCLUDES} -o ./build/rdefault.o -g -c

./build/lexer.o: ./lexer.c
	gcc lexer.c ${INCLUDES} -o ./build/lexer.o -g -c

./build/token.o: ./token.c
	gcc token.c ${INCLUDES} -o ./build/token.o -g -c

./build/lex_process.o: ./lex_process.c
	gcc lex_process.c ${INCLUDES} -o ./build/lex_process.o -g -c

./build/parser.o: ./parser.c
	gcc parser.c ${INCLUDES} -o ./build/parser.o -g -c

./build/node.o: ./node.c
	gcc node.c ${INCLUDES} -o ./build/node.o -g -c

./build/scope.o: ./scope.c
	gcc scope.c ${INCLUDES} -o ./build/scope.o -g -c

./build/symresolver.o: ./symresolver.c
	gcc symresolver.c ${INCLUDES} -o ./build/symresolver.o -g -c

./build/codegen.o: ./codegen.c
	gcc codegen.c ${INCLUDES} -o ./build/codegen.o -g -c

./build/stackframe.o: ./stackframe.c
	gcc stackframe.c ${INCLUDES} -o ./build/stackframe.o -g -c

./build/resolver.o: ./resolver.c
	gcc resolver.c ${INCLUDES} -o ./build/resolver.o -g -c



./build/fixup.o: ./fixup.c
	gcc fixup.c $(INCLUDES) -o ./build/fixup.o -g -c

./build/array.o: ./array.c
	gcc array.c ${INCLUDES} -o ./build/array.o -g -c

./build/expressionable.o: ./expressionable.c
	gcc expressionable.c ${INCLUDES} -o ./build/expressionable.o -g -c

./build/helper.o: ./helper.c
	gcc helper.c ${INCLUDES} -o ./build/helper.o -g -c

./build/datatype.o: ./datatype.c
	gcc datatype.c ${INCLUDES} -o ./build/datatype.o -g -c


./build/preprocessor/preprocessor.o: ./preprocessor/preprocessor.c
	gcc ./preprocessor/preprocessor.c ${INCLUDES} -o ./build/preprocessor/preprocessor.o -g -c

./build/preprocessor/static-includes/stdarg.o: ./preprocessor/static-includes/stdarg.c
	gcc ./preprocessor/static-includes/stdarg.c ${INCLUDES} -o ./build/preprocessor/static-includes/stdarg.o -g -c

./build/preprocessor/static-includes/stddef.o: ./preprocessor/static-includes/stddef.c
	gcc ./preprocessor/static-includes/stddef.c ${INCLUDES} -o ./build/preprocessor/static-includes/stddef.o -g -c




./build/preprocessor/static-include.o: ./preprocessor/static-include.c
	gcc ./preprocessor/static-include.c ${INCLUDES} -o ./build/preprocessor/static-include.o -g -c

./build/preprocessor/native.o: ./preprocessor/native.c
	gcc ./preprocessor/native.c ${INCLUDES} -o ./build/preprocessor/native.o -g -c



./build/helpers/buffer.o: ./helpers/buffer.c
	gcc ./helpers/buffer.c ${INCLUDES} -o ./build/helpers/buffer.o -g -c


./build/helpers/vector.o: ./helpers/vector.c
	gcc ./helpers/vector.c ${INCLUDES} -o ./build/helpers/vector.o -g -c

clean:
	rm ./main
	rm -rf ${OBJECTS}