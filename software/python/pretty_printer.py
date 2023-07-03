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
         return ('[%d tert]' % count, elt)

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

def build_pretty_printer():
   pp = MyPrinter("lol")
   pp.add_printer("string",CompileAndPredicate("^Array<char const>$"),StringPrinter)
   pp.add_printer("pair",PredicatePair,PairPrinter)
   pp.add_printer("hashmap",PredicateHashmap,HashmapPrinter)
   pp.add_printer("array",CompileAndPredicate("^Array<.*>$"),ArrayPrinter)
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


