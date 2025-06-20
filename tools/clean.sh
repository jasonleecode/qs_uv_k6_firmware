#!/bin/bash
find . -type f \( -name "*.o" -o -name "*.d" \) -exec rm -f {} +