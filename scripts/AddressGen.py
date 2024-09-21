import sys
from dataclasses import dataclass

identity = lambda x : x

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

DO_SHIFT_AND_INCREMENT_TOGETHER = False

@dataclass
class SinglePolynomial:
    first: int
    second: int

    def __add__(self,other):
        assert(isinstance(other,SinglePolynomial))
        return SinglePolynomial(self.first + other.first,self.second + other.second)

    def __sub__(self,other):
        assert(isinstance(other,SinglePolynomial))
        return SinglePolynomial(self.first - other.first,self.second - other.second)

    def __rsub__(self,other):
        assert(isinstance(other,SinglePolynomial))
        return SinglePolynomial(other.first - self.first,other.second - self.second)

    def __mul__(self,other):
        assert(isinstance(other,SinglePolynomial))
        assert(other.second == 0)
        return SinglePolynomial(self.first * other.first,self.second * other.first)

    __radd__ = __add__
    __rmul__ = __mul__

    def __hash__(self):
        return hash(self.first) * hash(self.second)

    def __repr__(self):
        if(self.first == 0 and self.second == 0):
            return "0"

        firstPart = f"{self.first}"
        if(self.first == 0):
            firstPart = ""

        if(self.second == 0):
            return firstPart

        sign = "+"
        secondPart = f"{abs(self.second)}x"

        if(self.second == 1):
            secondPart = "x"

        if(self.second == 0):
            sign = ""
        elif(self.second < 0):
            sign = "-"

        if(self.first == 0): # Removes sign because firstPart is zero
            sign = ""

        return firstPart + sign + secondPart

ZERO = SinglePolynomial(0,0)
ONE = SinglePolynomial(1,0)
X = SinglePolynomial(0,1)

@dataclass
class ValueAndIndex:
    value: SinglePolynomial
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


def GenerateRecursive(l,start = ZERO,incr = ONE,period = 1,shift = ONE,iterations = 1):
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
    start = values[0]
    GenerateGenericRecursive(l,start,values[1:])
    return l

def TagWithIndex(l):
    return [ValueAndIndex(y,x) for x,y in enumerate(l)]

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

    start = listOfAddresses[0]
    listOfAddresses = [x - start for x in listOfAddresses]

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

    parameters = [start]
    for loop in reversed(accumulatedLoops):
        parameters.append(loop.increment)
        parameters.append(loop.iterations)

    return parameters

def TestList(listOfAddresses):
    parameters = GenerateParameters(listOfAddresses)
    generic = GenerateGeneric(parameters)

    print("Parameters:  ")
    PrintParameters(parameters)
    print()
    print("Generic:     ",generic)
    print("Correct List:",listOfAddresses)
    for x,y in zip(generic,listOfAddresses):
        if(x != y):
            print("Different")
            return

    print("Equal")


def EvaluatePolynomial(poly,value):
    if(isinstance(poly,int)):
        return poly
    elif(isinstance(poly,SinglePolynomial)):
        return poly.first + poly.second * value

def EvaluatePolynomialList(l,value):
    return [EvaluatePolynomial(x,value) for x in l]

def IsWhitespace(ch):
    return ch in [" ", "\n", "\t"]

@dataclass
class Token:
    startIndex: int
    content: str
   
    __slots__ = ["startIndex","content"]
   
    def __eq__(self,other):
        if(isinstance(other, Token)):
            return self.content == other.content
        return self.content == other

    def __repr__(self):
        return self.content

class Tokenizer:
    __slots__ = ["content", "line", "index","storedTokens"]

    def __init__(self, content):
        self.content = content.strip()
        self.index = 0
        self.line = 0
        self.storedTokens = []

    def ClearWhitespace(self):
        size = len(self.content)
        while self.index < size:
            if(IsWhitespace(self.content[self.index])):
                if(self.content[self.index] == '\n'):
                    self.line += 1
                self.index += 1
                continue

            # Single line comment
            if(self.content[self.index] == '/' and self.index + 1 < size and self.content[self.index + 1] == "/"):
                while(self.index < size and self.content[self.index] != '\n'):
                    self.index += 1
                if(self.index < size and self.content[self.index] == '\n'):
                    self.index += 1
                    self.line += 1
                continue

            # Multi line
            if(self.content[self.index] == '/' and self.index + 1 < size and self.content[self.index + 1] == "*"):
                while(self.index + 1 < size and not (self.content[self.index] == '*' and self.content[self.index + 1] == '/')):
                    if(self.content[self.index] == "\n"):
                        self.line += 1
                    self.index += 1
                self.index += 2
                continue

            break

    def HasToken(self):
        if(len(self.storedTokens)):
            return True
        self.ClearWhitespace()
        return self.index < len(self.content)

    def NextCurrentLine(self):
        currentToken = self.Peek(0)

        start = currentToken.startIndex
        end = start

        while(start >= 0 and self.content[start] != '\n'):
            start -= 1
        start += 1

        while(end < len(self.content) and self.content[end] != '\n'):
            end += 1

        line = self.content[start : end]
        self.storedTokens.clear()
        self.index = end + 1
        return line

    def AssertNext(self, tokenToAssert):
        tok = self.Next()
        if(tok != tokenToAssert):
            print(tok,tokenToAssert)   

        assert tok == tokenToAssert
        return tok

    def ProcessOneToken(self):
        def IsSpecial(ch):
            return ch in "[],+-*"

        def CanProgress(index):
            return index < len(self.content)

        self.ClearWhitespace()

        startIndex = self.index
        ch = self.content[self.index]

        if IsSpecial(ch):
            self.index += 1
            self.storedTokens.append(Token(startIndex,ch))
        else:
            while CanProgress(self.index):
                ch = self.content[self.index]

                if IsSpecial(ch):
                    break

                if IsWhitespace(ch):
                    break

                self.index += 1

            token = self.content[startIndex : self.index]
            self.storedTokens.append(Token(startIndex,token))

    def NextUntil(self,*kargs):
        tokenList = []
        while(self.HasToken()):
            tok = self.Peek()

            if(tok in kargs):
                break
            else:
                tokenList.append(self.Next())

        return tokenList

    def Peek(self,amount = 0):
        while(len(self.storedTokens) < amount + 1):
            self.ProcessOneToken()
        return self.storedTokens[amount]

    def Next(self):
        if(len(self.storedTokens) == 0):
            self.ProcessOneToken()

        token = self.storedTokens.pop()
        return token


def EvaluateString(content):
    tok = Tokenizer(content)

    tok.AssertNext("[")

    def IsPoly(token):
        return "x" in token.content or "X" in token.content

    def GetNumberOnly(token):
        def IsDigit(ch):
            return "0" <= ch <= "9"

        if(token.content[0] == "x"):
            return 1

        index = 0
        while index < len(token.content):
            if(not IsDigit(token.content[index])):
                break
            index += 1

        return int(token.content[:index])

    def IsOp(token):
        assert len(token.content) == 1
        return token.content[0] in "+-"

    valueList = []
    while(tok.HasToken()):
        first = tok.Next()
        possibleOp = tok.Peek()

        if(not IsOp(possibleOp)):
            if(IsPoly(first)):
                valueList.append(SinglePolynomial(0,GetNumberOnly(first)))
            else:
                valueList.append(SinglePolynomial(int(first.content),0))

            if(possibleOp == ","):
                tok.Next()
                continue
            elif(possibleOp == "]"):
                break
            else:
                assert False

        possibleOp = tok.Next()
        second = tok.Next()

        assert not (IsPoly(first) and IsPoly(second))
        assert IsPoly(first) or IsPoly(second)

        if(IsPoly(first)):
            first,second = second,first

        sign = 1
        if(possibleOp == "-"):
            sign = -1

        valueList.append(SinglePolynomial(int(first.content),sign * GetNumberOnly(second)))

        decidier = tok.Peek()
        if(decidier == ","):
            tok.Next()
            continue
        else:
            break

    tok.AssertNext("]")
    return valueList


def Pre(l):
    return [SinglePolynomial(x,0) for x in l]

def PrintParameters(parameterList):
    base = ["incr","period","shift","iterations"]
    start = parameterList[0]

    print(f"start: {start}")
    for index,param in enumerate(parameterList[1:]):
        if index < 4:
            print(f"{base[index%4]}: {param}")
        else:
            print(f"{base[index%4]}{(index // 4) + 1}: {param}")

# TODO: While this works for polynomials with one variable, we could technically have unlimited variables.
#       x,y,z and so on.

#       One variable is enough for now

if __name__ == "__main__":
    match 3:
        case 1: listOfAddresses = Pre([0, 1, 2])
        case 2: listOfAddresses = Pre([0,1,2,10,11,12,20,21,22])
        case 3: listOfAddresses = Pre([0,1,2,10,11,12,20,21,22,1,2,3,11,12,13,21,22,23,10,11,12,20,21,22,30,31,32,11,12,13,21,22,23,31,32,33])
        case 4: listOfAddresses = EvaluateString("[x,x+1,x+2]")
        case 5: listOfAddresses = EvaluateString("[0,1,2,x,1+x,2+x,2x,1+2x,2+2x]")
    
    if(True): 
        parameters = GenerateParameters(listOfAddresses)
        print("Parameters:")
        PrintParameters(parameters)
        print()
        #print(EvaluatePolynomialList(listOfAddresses,10))
        #print(EvaluatePolynomialList(parameters,10))
    else:
        TestList(listOfAddresses)

