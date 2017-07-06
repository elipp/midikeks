#!/bin/bash

clang -g -Wall -lm -lfftw3 WAV.c fft.c -o fft
