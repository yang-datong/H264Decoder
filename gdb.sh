#!/bin/bash

gdb ./build/a.out \
	-ex "b parseSliceData" \
	-ex "r"
