#!/bin/bash
# Runs a Pudl source file with the built compiler.
# Usage: scripts/run.sh [file.pudl]   (defaults to examples/main.pudl)

src="${1:-examples/main.pudl}"

./build/pudl "$src"
