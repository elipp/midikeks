#!/bin/bash

clang -Wall -O2 -framework Foundation -framework CoreMIDI -framework AudioUnit miditest.c output.c -o miditest

