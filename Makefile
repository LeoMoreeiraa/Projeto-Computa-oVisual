# Makefile — Proj1: Processamento de Imagens
# Compilador: g++ (C++17)
# Dependências: SDL3, SDL3_image, SDL3_ttf

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 \
            $(shell pkg-config --cflags sdl3 sdl3-image sdl3-ttf 2>/dev/null)
LDFLAGS  := $(shell pkg-config --libs   sdl3 sdl3-image sdl3-ttf 2>/dev/null)

# Fallback manual flags if pkg-config is unavailable
ifeq ($(strip $(CXXFLAGS)),$(shell echo "-std=c++17 -Wall -Wextra -O2"))
CXXFLAGS += -I/usr/local/include/SDL3
LDFLAGS  += -lSDL3 -lSDL3_image -lSDL3_ttf
endif

TARGET  := programa
SRCDIR  := src
SRCS    := $(wildcard $(SRCDIR)/*.cpp)
OBJS    := $(SRCS:$(SRCDIR)/%.cpp=%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) output_image.png
