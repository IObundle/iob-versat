#!/usr/bin/python
#
#    d a t a G e n
#

import sys
import math
from random import randint

# revert bits function
def invert(x): return ~x & 0xffffffff

def genData(N):
    fw = open('./memdata.hex', 'w')
    
    # Generate N random numbers
    for i in xrange(1,N+1):
        #x = randint(0,2**31/N-1)
        x = 2*i
        
        if(i < N):
            fw.write(format(x,'08x')+'\n')
        else:
            fw.write(format(x,'08x'))
    
    fw.close()
    
    return 0

def usage (message) :
    print "usage: %s" % message
    print "       ./dataGen -n <file size>"
    sys.exit(1)

def main () :
    N = 0
    ret = -1
    
    # Process input arguments
    if (len(sys.argv) <= 2) : usage("too few arguments")
    
    i = 1
    while(i < len(sys.argv)):
        if (sys.argv[i] == "-n") : N = int(sys.argv[i+1])
        else : usage("unexpected argument '%s'" % sys.argv[i])
        i += 2
    
    # Generate data file
    ret = genData(N)
    
    sys.exit(ret)

if __name__ == "__main__" : main ()
