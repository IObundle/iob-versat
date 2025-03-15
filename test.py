
def GenerateValues1():
   res = []
   for z in range(0,2):
      for y in range(0,2):
         for x in range(0,2):
            val = z * 101 + y * 57 + x * 13
            res.append(val)

   return res

def GenerateValues2():
   res = []
   for z in range(0,2):
      for y in range(0,2):
         for x in range(0,14):
            val = z * 101 + y * 57 + x
            res.append(val)

   return res

def IsContained(small,high):
   valList = [False] * 190
   for x in high:
      valList[x] = True

   for x in small:
      if not valList[x]:
         return False
   return True

def GenerateTwoLoops(loopSize,loopAmount,shift):
   val = 0
   res = []
   for y in range(loopAmount):
      for x in range(loopSize):
         if(val > 180):
            return res
         res.append(val)
         val += 1
      val += shift - 1
   return res

#exampleLoop = GenerateTwoLoops(22,4,29)
exampleLoop = GenerateValues2()
print(exampleLoop)
generated = GenerateValues1()
print(generated)
print(IsContained(generated,exampleLoop))

if False:
   smallestValue = 999
   smallZ = 0
   smallY = 0
   shift = 0 
   for z in range(0,130):
      print(z)
      for y in range(0,100):
         for x in range(0,200):
            loop = GenerateTwoLoops(z,y,x)
            if(IsContained(generated,loop)):
               val = z * y
               if(val < smallestValue):
                  smallestValue = val 
                  smallZ = z
                  smallY = y
                  shift = x

   print(smallZ,smallY,shift)