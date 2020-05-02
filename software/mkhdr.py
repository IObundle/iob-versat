#!/usr/bin/python
#Description: converts .vh definitions to .h
#Arguments: .vh files directory and versat.h directory

#Import libraries
import re
import sys
import os

#Check command line arguments
if len(sys.argv) != 3:
    print("Usage: python mkhdr.py <vh_files_directory> <h_file_directory>")
    sys.exit()

#List with defines to ignore
ignore_list = ["FNS_W", "_BITS", "0_B", "A_B", "PERIOD_W", "DATABUS_W", "SHIFT_W"]

#Write include file
versat = open(sys.argv[2] + "/versat.h", "w")
versat.write('//Versat include file\n#include <math.h>\n')
versat.write('\n//MACRO to calculate ceil of log2\n#define clog2(x) ((int)ceil(log2(x)))\n')

#Loop to go through file list
for file in os.listdir(sys.argv[1]):
    if file.endswith(".vh"):
    
        #Open file to read
        versat.write("\n//" + file.replace(".vh", "") + "\n")
        f = open(sys.argv[1] + "/" + file, "r")
        for line in f:
            
            #ignore leading spaces
            line = line.lstrip() 
            
            #replace $clog2 by clog2
            line = line.replace("$", "")
            
            #remove tabs
            line = line.replace("\t", "")
            
            #split line elements in array
            line = re.sub(' +', ' ', line)
            values = line.split("\n")
            values = values[0].split(" ")
                            
            #ignore line in ignore_list
            if(len(values) > 1):
                continue_flag = False
                for val in ignore_list:
                    if(val in values[1]):
                        continue_flag = True
                if(continue_flag):
                    continue
                    
            #parser
            lookup = {}
            if "`define" == values[0]:
                if(len(values) > 2):
                    if values[2].isdigit() == False:
                        if "'d" in line:
                            const = line.split("'d")
                            const = const[1].split(" ")
                            lookup[values[1]] = int(const[0],10)
                        elif "'h" in line:
                            const = line.split("'h")
                            const = const[1].split(" ")
                            lookup[values[1]] = int(const[0],16)
                        elif "(" in line:
                            line = re.sub('`', '', line)
                            if "'b" in line:
                                line = re.sub('1\'b', '', line)
                            if '**' in line:
                                line = re.sub('2\*\*', '(1<<', line)
                                line = re.sub(re.escape(' +'), ') +', line)
                                line = re.sub(re.escape('-1)'), ')-1', line)
                                line = re.sub(re.escape('-2)'), ')-2', line)
                            const = line.split("(", 1)
                            const = const[1].split("//")
                            const = const[0].split('\n')
                            const = "(" + const[0] 
                            lookup[values[1]] = const
                        elif "{" in line:
                            continue
                        else:
                            const = values[2].split("`")
                            const = const[1].split("\n")
                            lookup[values[1]] = lookup[const[0]]
                    else:
                        lookup[values[1]] = int(values[2])
                else:
                    lookup[values[0].replace("`","") + " " + values[1]] = ""
            elif("`ifdef" == values[0]):
                lookup[values[0].replace("`","") + " " + values[1]] = ""
            elif("`else" == values[0] or "`endif" == values[0]):
                lookup[values[0].replace("`","")] = ""
                
            #Write to .h file
            for key, val in lookup.items():
                if (val == ""):
                    versat.write("#" + key + "\n")
                elif not (key == "xor" or key == "and" or key == "or"):
                    versat.write('#define ' + key + ' ' + str(val) + '\n')
                else:
                    versat.write('#define ' + key.upper() + ' ' + str(val) + '\n')
                
        #Close file
        f.close()

#close versat.h
versat.write('\n//End of Versat include file')
versat.close() 
