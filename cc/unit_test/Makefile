
EXE  := unitTest
LOG  := $(EXE).log
SRCS := $(wildcard *.cc)
# source files in other directories
SRCS += cJSON.cc

# header file search paths
INC_DIRS := .. ../cJSON

# source file search paths
SRC_DIRS := .. ../cJSON

include ../Makefile.common

include Makefile.env

# ignore compiler warnings for STATIC_ASSERT
CFLAGS += -Wno-unused-local-typedefs

clean-cmd += $(LOG)
distclean-cmd += $(LOG)

all: test

.PHONY: test

test: $(EXE)
	./$(EXE) > $(LOG)
