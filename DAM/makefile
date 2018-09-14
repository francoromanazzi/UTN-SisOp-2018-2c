APPLICATION=DAM
CC=gcc

BUILD_DIR=_build

SOURCES=$(shell find . -iname "*.c" | tr '\n' ' ')
OBJS=$(SOURCES:./src/%.c=$(BUILD_DIR)/%.o)

all: $(APPLICATION)

$(APPLICATION): create-dirs $(OBJS)
	$(CC)  -o $(APPLICATION) $(OBJS) -lshared -lcommons

$(BUILD_DIR)/%.o: src/%.c
	$(CC) -c -o "$@" "$<"

create-dirs:
	mkdir --parents $(BUILD_DIR)

debug: CC += -DDEBUG -g3 -Wall -O0
debug: all

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(APPLICATION)

.PHONY: all create-dirs clean debug