#!/bin/bash

IFS=' ' read -a FOR_MEM <<< "1 2 4 8 16 32 64"
rm -Rf mmass.out

for a in "${FOR_MEM[@]}"; do
  for r in $(seq 1 10); do
    LD_PRELOAD="$1" ./mmass ${a} >> mmass.out
  done
done
