#!/usr/bin/python3

import math
import os, sys
import re

POINTS = 20

WH_LVL = 210
BL_LVL = 40

WH_DIFF = 6
GR_DIFF = 2
N = 0
Knorm = 4
CEIL = 0.2

L_DIFF = 5/8.

def eval_coef():
    wh_coef = []
    gr_coef = []
    for x in range(1,POINTS+1):
        x = x/float(POINTS)
        wk = (1/x) + (WH_DIFF -1) + (1-x)*N
        #wk = WH_DIFF*((2-x)**(EXP-x))
        #wk = ((WH_DIFF/x - (WH_DIFF - 1))**EXP) + (WH_DIFF - 1)
        wk = math.ceil(wk - CEIL)
        #gk = GR_DIFF*((2-x)**(EXP-x))
        #gk = ((GR_DIFF/x - (GR_DIFF - 1))**EXP) + (GR_DIFF - 1)
        gk = (1/x) + (GR_DIFF -1) + (1-x)*N
        gk = math.ceil(gk - CEIL)
        wh_coef.append(wk)
        gr_coef.append(gk)
    #wh_coef.insert(0, wh_coef[0]+1)
    #gr_coef.insert(0, gr_coef[0]+1)
    return (wh_coef, gr_coef)

def coefs_to_str(lst):
    lst.reverse()
    coef_str = str(lst)
    coef_str = coef_str[1:(len(coef_str)-1)]
    #print("coefs: ", coef_str)
    return coef_str
    
if __name__ == "__main__":
    if (len(sys.argv) != 3):
        print("Usage: " + sys.argv[0] + " template_file output_file")
        sys.exit(-1)

    if (not os.path.exists(sys.argv[1])):
        print("Error: file " + sys.argv[1] + " does not exist")
        sys.exit(-1)
    
    wh_coef, gr_coef = eval_coef()
    wh_str = coefs_to_str(wh_coef)
    gr_str = coefs_to_str(gr_coef)

    template_file = open(sys.argv[1], 'r')
    template = template_file.read()
    template_file.close()

    wh_pattern = re.compile(r'WHITE_COEFS_TEMPLATE')
    gr_pattern = re.compile(r'GREY_COEFS_TEMPLATE')

    result = wh_pattern.sub(wh_str, template)
    result = gr_pattern.sub(gr_str, result)
    
    output_file = open(sys.argv[2], 'w')
    output_file.write(result)
    output_file.close()

