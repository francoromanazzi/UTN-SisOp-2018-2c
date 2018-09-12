CC=gcc

BUILD_DIR=_build
SOURCES=socket.c protocol.c msg.c util.c
HEADERS=socket.h protocol.h msg.h util.h
OBJS=$(SOURCES:%.c=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/libshared.so

$(BUILD_DIR)/libshared.so: create-dirs $(OBJS)
	$(CC) -shared -o $(BUILD_DIR)/libshared.so $(OBJS)

$(BUILD_DIR)/%.o: %.c
	$(CC) -c -fPIC -o "$@" "$<"

create-dirs:
	mkdir --parents $(BUILD_DIR)

debug: CC += -DDEBUG -g
debug: all

clean:
	rm -rf $(BUILD_DIR)

install: all
	cp -vu $(BUILD_DIR)/libshared.so /usr/lib
	mkdir --parents /usr/include/shared
	cp -vu $(HEADERS) /usr/include/shared

uninstall:
	rm -v -f /usr/lib/libshared.so
	rm -v -rf /usr/include/shared

.PHONY: all create-dirs clean debug install uninstall