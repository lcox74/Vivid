CC := gcc

TARGET_NAME := vivid
TARGET := bin/$(TARGET_NAME)

LINK :=$(INCLUDEDIR) $(LIBDIR) $(LIBS)

CCFLAGS := -Wall -Wextra -O2 -g -Iinclude

default: all

$(TARGET): main.c
	$(CC) -std=c17 $(CCFLAGS) main.c -lSDL2 -o $@ 

.PHONY: all
all: $(TARGET)
