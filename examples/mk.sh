#!/bin/sh

#g++ -g -I../.. *.cpp ../*.cpp && ./a.out

g++ -g -DDI_HEADER_ONLY -I../.. *.cpp && ./a.out


