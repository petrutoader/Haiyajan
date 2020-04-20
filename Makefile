CFLAGS := -std=c99 -g3 -fPIE -Wall -Wextra -pipe -I./inc $(shell sdl2-config --cflags)
CFLAGS += $(OPT)

ifeq ($(DEBUG),1)
	CFLAGS += -D DEBUG=1 -D SDL_ASSERT_LEVEL=3
	OPT ?= -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -Werror -D SDL_ASSERT_LEVEL=1 -fPIE -flto=auto -fno-fat-lto-objects
	OPT ?= -O2
endif

ifeq ($(STATIC),1)
	LDLIBS := $(shell sdl2-config --static-libs)
	CFLAGS += -static
else
	LDLIBS := $(shell sdl2-config --libs)
endif

GIT_VERSION := $(shell git rev-parse --short HEAD 2>/dev/null)
REL_VERSION := $(shell git describe --tags 2>/dev/null)
ifeq ($(GIT_VERSION),)
	GIT_VERSION := NO_GIT
endif
ifeq ($(REL_VERSION),)
	REL_VERSION := UNRELEASED
endif
CFLAGS += -D GIT_VERSION=\"$(GIT_VERSION)\" -D REL_VERSION=\"$(REL_VERSION)\"

.PHONY: test

all: haiyajan haiyajan.debug
haiyajan: ./src/haiyajan.o ./src/load.o ./src/play.o
	+$(CC) $(CFLAGS) -s -o $@ $^ $(LDLIBS)

# Produces a separate executable with debug symbols intact, and strips the
# main executable.
# To get information from stack trace: `addr2line -e haiyajan.debug addr`
haiyajan.debug: ./src/haiyajan.o ./src/load.o ./src/play.o
	+$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: haiyajan
	$(MAKE) -C ./test run

clean:
	$(RM) ./src/*.o
	$(RM) ./haiyajan
	$(MAKE) -C ./test clean

help:
	@echo "Options:"
	@echo "  DEBUG=1    Enables all asserts and reduces optimisation"
	@echo "  STATIC=1   Enables static build"
	@echo "  OPT=\"\"     Set custom optimisation options"
	@echo
	@echo "  Example: make DEBUG=1 OPT=\"-Ofast -march=native\""
	@echo
	@echo "Copyright (C) 2020 Mahyar Koshkouei"
	@echo "Haiyajan is free software; see the LICENSE file for copying conditions. There is "
	@echo "NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
	@echo ""
