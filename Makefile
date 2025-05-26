SOURCES=$(wildcard *.cpp)
HEADERS=$(SOURCES:.cpp=.h)
FLAGS=-DDEBUG -g
EXEC=grannies_program

NUM_GRANNIES := $(shell grep -E '^#define[[:space:]]+NUM_GRANNIES' consts.h | awk '{print $$3}')
NUM_STUDENTS := $(shell grep -E '^#define[[:space:]]+NUM_STUDENTS' consts.h | awk '{print $$3}')
TOTAL_PROCESSES := $(shell echo $$(($(NUM_GRANNIES) + $(NUM_STUDENTS))))


all: $(EXEC)

$(EXEC): $(SOURCES) $(HEADERS)
	mpic++ -std=c++17 $(FLAGS) -o $(EXEC) $(SOURCES)

run: all
	@if [ -z "$(TOTAL_PROCESSES)" ]; then \
		echo "Nie udało się obliczyć TOTAL_PROCESSES z consts.h"; exit 1; \
	fi
	@echo "Uruchamianie z $(TOTAL_PROCESSES) procesami..."
	mpirun -np $(TOTAL_PROCESSES) ./$(EXEC)

clean:
	rm -f $(EXEC) tmp_exec tmp.cpp