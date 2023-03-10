#!/bin/sh
CC="g++-12"

$CC rasterizer_tb.cc -Wall -Wextra -pedantic -O1 -g -std=c++20 $(pkg-config sdl --libs --cflags)
if [ $? -eq 0 ]; then
    ./a.out
else
    echo ">> Compiler Reported Failure $?"
fi