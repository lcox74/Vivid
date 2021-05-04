CC := gcc

TARGET_NAME := vivid
TARGET := bin/$(TARGET_NAME)

# INCLUDEDIR := -Iinclude -Ivendor/SDL2-2.0.14/include
# LIBDIR := -Lvendor/SDL2-2.0.14/lib
# LIBS := -lmingw32 -lSDL2main -lSDL2 

LINK :=$(INCLUDEDIR) $(LIBDIR) $(LIBS)

CCFLAGS := -Wall -Wextra -O2 -g 

default: all

$(TARGET): main.c
	$(CC) -std=c17 $(CCFLAGS) main.c -lSDL2  -o $@ 

.PHONY: all
all: $(TARGET)
