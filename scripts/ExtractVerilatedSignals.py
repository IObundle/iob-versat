#!/usr/bin/env python3

import sys

content = None
with open(sys.argv[1],"r") as file:
   content = file.readlines()

print(f"#pragma once")
for line in content:
   if(not "VL" in line):
      continue

   splitted = line.split("(")
   if(len(splitted) < 2):
      continue

   splitted2 = splitted[1].split(')')
   if(len(splitted2) < 2):
      continue

   VLType = splitted[0].strip()
   argTotal = splitted2[0].split(',')

   # Needs to have at least name, msb and lsb
   if(len(argTotal) < 3):
      continue

   namePtr = argTotal[0]

   if(namePtr[0] != '&'):
      continue

   name = namePtr[1:]
   msb = argTotal[1]
   lsb = argTotal[2]

   print(f"#define UNIT_MSB_{name} {msb}")
   print(f"#define UNIT_LSB_{name} {lsb}")
