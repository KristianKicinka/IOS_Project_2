TARGET=proj2

default: all

all:
	gcc $(TARGET).c -std=gnu99 -Wall -Wextra -Werror -pedantic -o $(TARGET) -pthread

run: all
	./$(TARGET) 5 4 100 100
