#!/bin/bash

clang -Wall -g -framework Foundation -framework CoreMIDI -framework AudioUnit miditest.c output.c -o miditest

