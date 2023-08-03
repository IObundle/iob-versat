"""
Problem 1: Codeblocks does not display the raw value for any child node.

Example: $1 = Pair<Array<char const>, Array<char const> > = {first = "123" = {data = 0x55dd301bd476 "123", size = 3}, second = "234" = {data = 0x55dd301bd47a "234", size = 3}}

   The first = "123" = ... is the value for the first member. The "123" inside the '=' is useless. Codeblocks does not display that as a raw value, when it would be really good if it did.

p.first gives: $2 = "123" = {data = 0x55dd301bd476 "123", size = 3}
   
   And in that case, Codeblocks does pick on the "123" and displays it as the raw value, as expected.

   Very likely that the problem is more Codeblocks fault than the pretty printers.

Seems to me like the format for a simple struct is:

<VarName> = <VarRawValue> = <VarChildren>

but codeblocks only follows that format for the first and the last children. The middle children, codeblocks just ignores <VarRawValue>

Problem 2: For some reason, code blocks does what I want for hashmaps, except that I need to put a ':' which also gets displayed as the raw value for the second pair.
   At the very least, it "works" 

$1 = std::unordered_map with 3 elements = {["3" = {data = 0x55a162218472 "3", size = 1}] = "4" = {data = 0x55a162218474 "4", size = 1}, ["2" = {data = 0x55a162218470 "2", size = 1}] = "3" = {data = 0x55a162218472 "3", size = 1}, ["1" = {data = 0x55a16221846e "1", size = 1}] = "2" = {data = 0x55a162218470 "2", size = 1}}
> print *map
$1 = Hashmap<Array<char const>, Array<char const> > of size 3 = {["1":"2" = {first = "1" = {data = 0x55c858afb46e "1", size = 1}, second = "2" = {data = 0x55c858afb470 "2", size = 1}}] = "2":"3" = {first = "2" = {data = 0x55c858afb470 "2", size = 1}, second = "3" = {data = 0x55c858afb472 "3", size = 1}}, ["3":"4" = {first = "3" = {data = 0x55c858afb472 "3", size = 1}, second = "4" = {data = 0x55c858afb474 "4", size = 1}}] = }

"""

import gdb
import gdb.types
import gdb.printing
import re
import sys

class MyPrinter(object):
   class Subprinter(gdb.printing.SubPrettyPrinter):
      def __init__(self, name, predicate, gen_printer):
         super(MyPrinter.Subprinter, self).__init__(name)
         self.gen_printer = gen_printer
         self.predicate = predicate

   def __init__(self,name):
      self.name = name
      self.subprinters = []
      self.enabled = True

   def add_printer(self, name, predicate, gen_printer):   
      self.subprinters.append(self.Subprinter(name, predicate,gen_printer))

   def __call__(self,val):
      # Get the type name.
      typename = gdb.types.get_basic_type(val.type).tag
      if not typename:
         typename = val.type.name
      if not typename:
         return None

      # Iterate over table of type regexps to determine
      # if a printer is registered for that type.
      # Return an instantiation of the printer if found.
      for printer in self.subprinters:
         if printer.enabled and printer.predicate(typename):
            return printer.gen_printer(typename,val)

      # Cannot find a pretty printer.  Return None.
      return None

class SimplePrinter(MyPrinter):

   def __init__(self, typename, val):
      self.typename = typename
      self.val = val

   def to_string(self):
      res = "%s of size %d" % (self.typename, int(self.val["size"]))
      return res;

class ArrayPrinter(MyPrinter):
   "Print an Array"

   class _iterator:
      def __init__ (self, start, size):
         self.item = start
         self.size = size
         self.count = 0

      def __iter__(self):
         return self

      def __next__(self):
         count = self.count
         if self.count == self.size:
            raise StopIteration
         self.count = self.count + 1
         elt = self.item.dereference()
         self.item = self.item + 1
         return ('[%d]' % count, elt)

   def __init__(self, typename, val):
      self.typename = typename
      self.val = val

   def children(self):
      return self._iterator(self.val['data'],self.val['size'])

   def to_string(self):
      res = "%s of size %d" % (self.typename, int(self.val["size"]))
      return res;

   def display_hint(self):
      return "array"

def RemoveChilds(repr):
   space = repr.find(' ')
   if(space == -1):
      return repr
   return repr[:repr.find(' ')]

class PairPrinter(MyPrinter):
   def __init__(self, typename, val):
      self.typename = typename
      self.val = val

   def children(self):
      return (("first",self.val["first"]),("second",self.val["second"]))

   def to_string(self):
      first = RemoveChilds(self.val["first"].format_string(raw=False))
      second = RemoveChilds(self.val["second"].format_string(raw=False))
      return "%s:%s" % (first,second)

class HashmapPrinter(MyPrinter):
   "Print an Hashmap"

   class _iterator:
      def __init__ (self, start, size):
         self.item = start
         self.size = size
         self.count = 0

      def __iter__(self):
         return self

      def __next__(self):
         count = self.count
         if self.count == self.size:
            raise StopIteration
         self.count = self.count + 1
         elt = self.item.dereference()
         self.item = self.item + 1
         return ('[%d]' % count, elt)

   def __init__(self, typename, val):
      self.typename = typename
      self.val = val

   def children(self):
      return self._iterator(self.val['data'],self.val['nodesUsed'])

   def to_string(self):
      res = "%s of size %d" % (self.typename, int(self.val["nodesUsed"]))
      return res;

   def display_hint(self):
      return "array"

class StringPrinter(MyPrinter):
   "Print a String"

   def __init__(self, typename, val):
      self.typename = typename
      self.val = val

   def children(self):
      return (("data",str(self.val["data"])),("size",self.val["size"]))

   def to_string(self):
      size = int(self.val["size"])
      data = self.val["data"]
      size = min(size,1024)
      res = "%s" % data.string(encoding = "ascii",errors = "replace",length = size)
      return res;

   def display_hint(self):
      return "string"

def CompileAndPredicate(regexp):
   compiled = re.compile(regexp)

   return (lambda x : compiled.search(x))

def PredicateHashmap(typeStr):
   tokens = [x for x in Tokenize(typeStr,"<,>*")]
   if(len(tokens) > 0 and tokens[0] == "Hashmap"):
      return True
   return False

def PredicatePair(typeStr):
   tokens = [x for x in Tokenize(typeStr,"<,>*")]
   if(len(tokens) > 0 and tokens[0] == "Pair"):
      return True
   return False

class TestPrinter(MyPrinter):
   "Print Test"

   def __init__(self, typename, val):
      self.typename = typename
      self.val = val

   def children(self):
      return (("val",self.val["val"]),) #(("firstArg","1"),("secondArg","2"))

   def to_string(self):
      return "test"
#      size = int(self.val["size"])
#      data = self.val["data"]
#      size = min(size,1024)
#      res = "%s" % data.string(encoding = "ascii",errors = "replace",length = size)
#      return res;

   def display_hint(self):
      return "string"

def build_pretty_printer():
   pp = MyPrinter("lol")
   # Seem fine
   pp.add_printer("string",CompileAndPredicate("^Array<char const>$"),StringPrinter)
   pp.add_printer("string",CompileAndPredicate("^String$"),StringPrinter)
   pp.add_printer("array",CompileAndPredicate("^Array<.*>$"),ArrayPrinter)
   pp.add_printer("test",CompileAndPredicate("^Test$"),TestPrinter)
   pp.add_printer("pair",PredicatePair,PairPrinter)

   # Not working good.
   pp.add_printer("hashmap",PredicateHashmap,HashmapPrinter)
   return pp

gdb.printing.register_pretty_printer(
   gdb.current_objfile(),
   build_pretty_printer())

def Tokenize(string,specialChars):
   start = 0
   for i in range(len(string)):
      if(string[i] in specialChars):
         if(start != i):
            yield string[start:i]
         yield string[i]
         start = i + 1


