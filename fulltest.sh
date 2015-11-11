#!/bin/bash
cd "$(dirname "$0")"
./dova tests/allinstructions.asm a.out
./dova a.out b.asm -d
./dova b.asm b.out
diff a.out b.out > diff.txt
cat diff.txt
