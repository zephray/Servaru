CC	:= gcc
C_FLAGS := -O1 -g -Wall -Wextra $(shell pkg-config sdl --cflags)

BINDIR	:= bin
OBJDIR	:= obj

LIBRARIES	:= $(shell pkg-config sdl --libs) -lm

ifeq ($(OS),Windows_NT)
EXECUTABLE	:= main.exe
else
EXECUTABLE	:= main
endif

INCLUDE	:= -I. -I./S3D

SRC := \
	camera.c \
	mesh.c \
	shader.c \
	simple_shaders.c \
	texture.c \
	utils.c \
	vecmath.c \
	main.c \
	s3d/s3d.c \
	s3d/fsg.c \
	s3d/rasterizer.c \
	s3d/setup.c \
	s3d/tmu.c

OBJ := $(addprefix $(OBJDIR)/, $(SRC:.c=.o))

all: $(BINDIR)/$(EXECUTABLE)

clean:
	rm -rf $(OBJDIR)/
	rm $(BINDIR)/$(EXECUTABLE)

run: all
	./$(BINDIR)/$(EXECUTABLE)

$(OBJDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(INCLUDE) -c $< -o $@

$(BINDIR)/$(EXECUTABLE): $(OBJ)
	mkdir -p $(BINDIR)
	$(CC) $(C_FLAGS) $(INCLUDE) $^ -o $@ $(LIBRARIES)
	objdump -s -l -d $@ > $(BINDIR)/disasm.s
