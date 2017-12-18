# Vehicle Scheduling Problem
EXE  := vsp
SRCS := $(wildcard *.cc)
SRCS += cJSON.cc

# header file search paths
INC_DIRS := cJSON

# source file search paths
SRC_DIRS := cJSON

include Makefile.common

# ignore compiler warnings for STATIC_ASSERT
CFLAGS += -Wno-unused-local-typedefs


TEST_DIRS := cJSON unit_test
UTILITY_DIRS := 

.PHONY: test $(TEST_DIRS) $(UTILITY_DIRS)

test: $(TEST_DIRS)

clean-cmd+=
clean: $(TEST_DIRS) $(UTILITY_DIRS)

distclean-cmd+=$(JSON_TEMPLATE)
distclean: $(TEST_DIRS) $(UTILITY_DIRS)

$(TEST_DIRS) $(UTILITY_DIRS):
	$(MAKE) --directory=$@ $(MAKECMDGOALS)
