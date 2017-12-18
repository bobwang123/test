
EXE  := cjTest
LOG  := $(EXE).log
SRCS := $(wildcard *.cc)
# source files in other directories
#SRCS += 

# header file search paths
#INC_DIRS := ..

# source file search paths
#SRC_DIRS := ..

include ../Makefile.common

clean-cmd += $(LOG)
distclean-cmd += $(LOG)

all: test

.PHONY: test

test: $(EXE)
	./$(EXE) > $(LOG)
	@diff $(LOG) $(LOG).au && echo "\033[32mcJSON tests PASSED\033[0m"
