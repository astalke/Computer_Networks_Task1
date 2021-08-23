CC=gcc
INCLUDE=./src/error.h ./src/common.h ./src/tcp_handler.h ./src/http.h \
	./src/buffer.h ./src/correlated.h
OBJ=./obj/main.o ./obj/error.o ./obj/common.o ./obj/tcp_handler.o ./obj/http.o\
    ./obj/buffer.o ./obj/correlated.o
FLAGS=-Wall -Wextra -std=c11 -O2 -g

serwer: $(OBJ)
	$(CC) $^ $(FLAGS) -o $@

./obj/%.o: ./src/%.c $(INCLUDE)
	$(CC) $< -c $(FLAGS) -o $@

.PHONY: clean
clean:
	-rm ./obj/*
	-rm ./serwer
