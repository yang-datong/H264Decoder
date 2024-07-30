#!/bin/bash

gdb ./a.out \
	-ex "b parseSliceData" \
	-ex "r"
