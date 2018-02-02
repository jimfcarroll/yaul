#!/bin/sh

g++ -std=c++11 -g `pkg-config --cflags unittest++` *.cpp ../*.cpp `pkg-config --libs unittest++`

