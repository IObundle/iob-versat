import sys
from dataclasses import dataclass

identity = lambda x : x

DO_SHIFT_AND_INCREMENT_TOGETHER = False

@dataclass
class ValueAndIndex:
    value: int
    index: int

    def __repr__(self):
        return str(self.value)

def Derivative(listOfValues : list[int]):
    return [y - x for x, y in zip(listOfValues[:-1], listOfValues[1:])]

def MostCommonElement(listOfValues : list[ValueAndIndex]):
    d = {}

    for x in listOfValues:
        if x.value in d:
            d[x.value] += 1
        else:
            d[x.value] = 1

    return max(d.items(), key=lambda x: x[1])[0]

def RemoveElement(listOfValues : list[ValueAndIndex], elem):
    newList = []
    for x in listOfValues:
        if x.value != elem:
            newList.append(x)

    return newList

def RemoveElementsAfterIndex(l,cutoff):
    res = []
    for x in l:
        if x.index < cutoff:
            res.append(x)

    return res


def GenerateRecursive(l,start = 0,incr = 1,period = 1,shift = 1,iterations = 1):
    addr = start
    for y in range(iterations):
        for x in range(period):
            l.append(addr)
            addr += incr
        if not DO_SHIFT_AND_INCREMENT_TOGETHER:
            addr -= incr
        addr += shift

def GenerateGenericRecursive(l,start,values):
    if(len(values) > 4):
        incr = values[-4]
        period = values[-3]
        shift = values[-2]
        iterations = values[-1]
        addr = start
        for y in range(iterations):
            for x in range(period):
                GenerateGenericRecursive(l,addr,values[:-4])
                addr += incr
        
            if not DO_SHIFT_AND_INCREMENT_TOGETHER:
                addr -= incr

            addr += shift
    else:
        GenerateRecursive(l,start,*values)

def GenerateGeneric(values):
    l = []
    GenerateGenericRecursive(l,0,values)
    return l

def Generate(incr=1,period=1,shift=1,iterations=1,incr2=1,period2=1,shift2=1,iterations2=1,incr3=1,period3=1,shift3=1,iterations3=1):
    l = []

    addr2 = 0
    addr3 = 0

    for y3 in range(iterations3):
        for x3 in range(period3):
            addr2 = addr3
            for y2 in range(iterations2):
                for x2 in range(period2):
                    addr = addr2
                    for y in range(iterations):
                        for x in range(period):
                            l.append(addr)
                            addr += incr
                        addr += shift
                    addr2 += incr2
                addr2 += shift2
            addr3 += incr3
        addr3 += shift3

    return l

def TagWithIndex(l):
    return [ValueAndIndex(y,x) for x,y in enumerate(l)]

# Approach taken

# Since we are dealing with linear loops contained into loops, we can identify the loops layers by calculating a derivative and 
# shaving off loops based on the most common element (because the increments are always constant, the first loop will run the most,
# then the second loop and so on)

# We do this and identify all the periods and so on. Afterwards we remove all the extra data (since we only need one outermost
# loop iteration to compute the rest and because the full length of the loop is at this point only given by the outermost loop value)
# At this point we have one outermost loop amount of data and can calculate all the data that we need.

# To calculate the values for the higher loops, we basically obtain the perspective of the initial values from the point of view of the third loop
# Basically removing the first two loops as they are already fully calculated and then looking at the other loops individually

# How do we handle missing values?
# We can identify problematic values by looking at the amount of common values that we have.
# We expect to see a proportional amount of common values inside the first derivative, meaning that we can identify probematic values 
# here by looking at any value that is vastly different from 

@dataclass
class Loop:
    listOfAddresses: list[ValueAndIndex]
    commonElement: int
    loopPeriod: int = 0 # Period is not equal to increment. It is how many values exist between each iteration of the loop

    # Final values of the loop
    iterations: int = 0
    increment: int = 0

# TODO: This code could use some cleanup
def GenerateLoops(listOfAddresses):
    index = 0
    loops = []
    l = TagWithIndex(Derivative(listOfAddresses))
    while(len(l)):
        common = MostCommonElement(l)
        loops.append(Loop(l,common))

        l = RemoveElement(l,common)

        # We remove the common value because we are assuming that the iteration and the shift are both added at the same time at the end of a period
        # By removing the common value we basically propagate this information over the entire algorithm
        if DO_SHIFT_AND_INCREMENT_TOGETHER:
            for i in range(len(l)):
                l[i].value -= common

        index += 1

    firstPeriod = loops[-1].listOfAddresses[0].index + 1

    loops[-1].loopPeriod = firstPeriod

    howManyIterations = len(listOfAddresses) // firstPeriod

    maxPeriod = firstPeriod

    periods = []
    for loop in loops[::-1]:
        line = RemoveElementsAfterIndex(loop.listOfAddresses,maxPeriod - 1)
        amount = len(line)

        loopPeriod = maxPeriod // (amount + 1)
        
        loop.loopPeriod = loopPeriod
        periods.append(loopPeriod)
        maxPeriod = loopPeriod

    periods = list(reversed(periods))

    finalPeriod = []
    for x,y in zip(periods[:-1],periods[1:]):
        finalPeriod.append(y // x)

    finalPeriod.append(howManyIterations)

    for i,loop in enumerate(loops):
        loop.iterations = finalPeriod[i]
        loop.increment = loop.commonElement

    return loops

def GeneratePerspective(originalList,period):
    perspective = []
    index = 0
    while(index < len(originalList)):
        perspective.append(originalList[index])
        index += period

    return perspective

def GenerateParameters(listOfAddresses):
    accumulatedLoops = []
    loops = GenerateLoops(listOfAddresses)

    while(len(loops) > 2):
        accumulatedLoops += loops[:2]
        perspective = GeneratePerspective(listOfAddresses,loops[2].loopPeriod)

        loops = GenerateLoops(perspective)

    accumulatedLoops += loops
    accumulatedLoops = list(reversed(accumulatedLoops))

    accum = 1
    for loop in accumulatedLoops:
        accum *= loop.iterations

    parameters = []
    for loop in reversed(accumulatedLoops):
        parameters.append(loop.increment)
        parameters.append(loop.iterations)

    return parameters

def TestList(listOfAddresses):
    parameters = GenerateParameters(listOfAddresses)
    generic = GenerateGeneric(parameters)

    print(generic)
    print(listOfAddresses)
    print(parameters)
    for x,y in zip(generic,listOfAddresses):
        if(x != y):
            print("Different")
            return

    print("Equal")


if __name__ == "__main__":
    #listOfAddresses = [0, 1, 2]
    listOfAddresses = [0,1,2,10,11,12,20,21,22,1,2,3,11,12,13,21,22,23,10,11,12,20,21,22,30,31,32,11,12,13,21,22,23,31,32,33]
    #listOfAddresses = [9,8,10,7,11,6,12,5,13,4,14,3,15,2,16,1,17]

    TestList(listOfAddresses)

    #print(accum,len(listOfAddresses))

