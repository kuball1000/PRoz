SOURCES=$(wildcard *.c)
HEADERS=$(SOURCES:.c=.h)
FLAGS=-DDEBUG -g

all:
	mpic++ -std=c++17 -o grannies_program *.cpp