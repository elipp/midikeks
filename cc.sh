#!/bin/bash

clang -Wall -g -framework Foundation -framework CoreMIDI -framework AudioUnit -framework AudioToolbox miditest.c output.c input.c -o miditest

