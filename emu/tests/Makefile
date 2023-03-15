CC	:= g++-12
C_FLAGS := -O1 -g -std=c++20 -Wall -Wextra $(shell pkg-config sdl --cflags)
BINDIR	:= ./
OBJDIR	:= ./
LIBRARIES	:= $(shell pkg-config sdl --libs) -lm /usr/local/lib/libSDL_draw.a
INCLUDE	:= -I../ -I../s3d  -I/usr/local/include/ -I.
SRC := \
	rasterizer_tb.cc
    # clipper_tb.cc

EXECUTABLE := a.out

all: $(EXECUTABLE)
	
clean: 
	rm $(EXECUTABLE)
	rm *.o

run: all
	./$(EXECUTABLE)

$(EXECUTABLE): $(SRC)
	$(CC) -o "$@" "$<" $(C_FLAGS) $(INCLUDE) $(LIBRARIES)
