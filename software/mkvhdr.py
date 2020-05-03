#!/usr/bin/python
#Description: converts xversat.json definitions to xversat.vh
#Arguments: xversat.json directory and xversat.vh directory

#Import libraries
import sys
import json

#Check command line arguments
if len(sys.argv) != 3:
    print("Usage: python mkvhdr.py <xversat.json directory> <xversat.vh directory>")
    sys.exit()
    
#Open files
f_in = open(sys.argv[1] + "/xversat.json", "r")
f_out = open(sys.argv[2] + "/xversat.vh", "w")

#Loop to go through json file
versat = json.load(f_in)
for key in versat:
    if(isinstance(versat[key], bool)):
        if(versat[key]):
            f_out.write("`define " + key + '\n')
    else:
        f_out.write("`define " + key + " " + str(versat[key]) + '\n')

#Close files
f_in.close()
f_out.close()