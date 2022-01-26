#!/bin/python3

# Shape "curve" "string type" "cylinder" "point P" [ 0.005968 0.095212 -0.003707 0.006467 0.099219 -0.005693 0.007117 0.102842 -0.008281 0.007995 0.105534 -0.011779 ] "float width0" 0.000180 "float width1" 0.000015

import os
import sys


if(len(sys.argv) < 2):
    print(f'Usage: {sys.argv[0]} [file.pbrt]')

with open(sys.argv[1], 'r') as pbrt_file:
    curves_count = 0
    final_str = ""
    while True:
        l = pbrt_file.readline()
        if not l: break
        points_str = l[str.find(l, "\"point P\" ")+len("\"point P\" "):]
        points_str = points_str[1: str.find(points_str, ']')].strip()
        points = points_str.split(" ")
        width0_str = l[str.find(l, "\"float width0\" ")+len("\"float width0\" "):]
        if width0_str[0] == '[':
            width0 = width0_str[1:width0_str.find("]")-1].strip()
        else:
            width0 = width0_str[0:width0_str.find(" ")-1].strip()
        width1_str = l[str.find(l, "\"float width1\" ")+len("\"float width1\" "):]
        if width1_str[0] == '[':
            width1 = width1_str[1:width1_str.find("]")-1].strip()
        else:
            width1 = width1_str[0:width1_str.find(" ")-1].strip()

        curves_count += 1
        i = 0
        final_str += f'{width0} {width1}\n'
        while i < len(points):
            final_str += f'{points[i]} {points[i+1]} {points[i+2]}\n'
            i += 3

    with open(sys.argv[1][:-4]+"bez", 'w') as out:
        out.write(f'{curves_count}\n{final_str}')

