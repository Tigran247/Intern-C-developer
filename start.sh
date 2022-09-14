#!/bin/bash
g++ -c ./1/program_1.cpp
g++ -o program_1 program_1.o -lpthread
g++ -c ./2/program_2.cpp
g++ -o program_2 program_2.o -lpthread