#!/bin/bash

clang -g -O2 -Wall -lm -lfftw3 WAV.c fft.c -o fft
