OBJECTS=
INCLUDES= -I./

all: ${OBJECTS}
	gcc main.c ${INCLUDES} ${OBJECTS} -g -o ./main 

clean:
	rm ./main
	rm -rf ${OBJECTS}