CC := clang
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c23 \
	-Wshadow -Wconversion -Wnull-dereference -Wformat=2 -Wundef \
	-I.
DEBUG_CFLAGS := -g -fsanitize=address,undefined,leak -O0
RELEASE_CFLAGS := -O3

INCLUDES=$(shell find . -name '*.h')
BUILD := bin

debug: CFLAGS := $(CFLAGS) $(DEBUG_CFLAGS)
debug: client server

release: CFLAGS := $(CFLAGS) $(RELEASE_CFLAGS)
release: client server

server: server.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/$@ $<

client: client.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/$@ $<

$(BUILD):
	mkdir -p $@
