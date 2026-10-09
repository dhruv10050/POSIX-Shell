# Compiler and flags
CXX       := g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra -Wno-sign-compare -MMD -MP
CPPFLAGS  := -Iinclude
LDFLAGS   :=
LDLIBS    :=

# Try pkg-config first for readline (works on Linux and many macOS setups)
READLINE_PKG_CFLAGS := $(shell pkg-config --cflags readline 2>/dev/null)
READLINE_PKG_LIBS   := $(shell pkg-config --libs   readline 2>/dev/null)

# If pkg-config failed, fall back to Homebrew on macOS
ifeq ($(READLINE_PKG_LIBS),)
  ifeq ($(shell uname),Darwin)
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
    ifneq ($(BREW_PREFIX),)
      CPPFLAGS += -I$(BREW_PREFIX)/opt/readline/include
      LDFLAGS  += -L$(BREW_PREFIX)/opt/readline/lib
      LDLIBS   += -lreadline
      # Some macs still need curses for readline termcap symbols
      LDLIBS   += -lncurses
      # Ensure runtime can find Homebrew libs when run on other macs
      LDFLAGS  += -Wl,-rpath,$(BREW_PREFIX)/opt/readline/lib
    else
      $(warning readline not found via pkg-config or Homebrew. Build may fail.)
    endif
  else
    # Non-macOS without pkg-config: try common defaults
    LDLIBS += -lreadline -lncurses
  endif
else
  CPPFLAGS += $(READLINE_PKG_CFLAGS)
  LDLIBS   += $(READLINE_PKG_LIBS)
endif

# Sources
SRC := \
  src/cd.cpp src/echo.cpp src/ls.cpp src/pinfo.cpp src/prompt.cpp src/pwd_local.cpp \
  src/search.cpp src/systemcommands.cpp src/utils.cpp src/history.cpp \
  src/ioredirection.cpp src/autocomplete.cpp src/signals.cpp src/pipeline.cpp \
  src/builtincommands.cpp

OBJ := $(SRC:.cpp=.o)
DEP := $(OBJ:.o=.d)

# Default target
all: main

main: main.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Debug build with sanitizers (use: make debug)
debug: CXXFLAGS := -std=c++17 -O0 -g3 -Wall -Wextra -Wno-sign-compare -fsanitize=address,undefined -fno-omit-frame-pointer -MMD -MP
debug: LDFLAGS  += -fsanitize=address,undefined
debug: clean main

.PHONY: all clean debug
clean:
	rm -f $(OBJ) $(DEP) main

# Auto-rebuild on header changes
-include $(DEP)
