#!/bin/sh

#g++ -std=c++11 -g `pkg-config --cflags UnitTest++` *.cpp ../*.cpp `pkg-config --libs UnitTest++`
g++ -std=c++11 -g `pkg-config --cflags UnitTest++` -DDI_HEADER_ONLY *.cpp `pkg-config --libs UnitTest++`

