#!/bin/bash

clang -O2 -Wall -g -framework Foundation -framework CoreMIDI -framework AudioUnit -framework AudioToolbox miditest.c output.c input.c harmony.c -o miditest

