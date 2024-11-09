import clang
from clang.cindex import Cursor, Index, Config, CursorKind, TranslationUnit, TypeKind
import sys
import copy

import traceback
from inspect import currentframe

def LineNumber():
    return currentframe().f_back.f_lineno

UNKNOWN = "UNKNOWN"
BASE = "BASE"
STRUCT = "STRUCT"
UNION = "UNION"
POINTER = "POINTER"
ARRAY = "ARRAY"
TEMPLATED_STRUCT_DEF = "TEMPLATED_STRUCT_DEF" 
TEMPLATED_INSTANCE = "TEMPLATED_INSTANCE"
ENUM = "ENUM"
TYPEDEF = "TYPEDEF"
ELABORATED = "ELABORATED"

TEMPLATE_ARGUMENT = "TEMPLATE_ARGUMENT"

ENABLE_PRINT = False
ENABLE_PRINT_AST = False
ENABLE_DEBUG_PRINT = False

allTypes = {}

def RemoveConst(string):
    return string.replace("const","").strip()

def DebugPrint(*args,**kargs):
    if(ENABLE_DEBUG_PRINT):
        print(*args,**kargs)

def GetEntireSourceText(c: Cursor):
    start = c.extent.start.offset
    end = c.extent.end.offset

    # TODO(performance): Slow to keep opening and closing
    file = open(c.extent.start.file.name,'r')
    file.seek(start)
    
    content = file.read(end - start + 1)
    file.close()

    return content

class TemplateArgument:
    def __init__(self,name):
        self.name = name

    def Type(self):
        return TEMPLATE_ARGUMENT

    def __repr__(self):
        return self.Type() + ":" + str(self.name)

def TransformCTypeIntoType(cType):
    name = RemoveConst(cType.spelling)
    try:
        return allTypes[name]
    except:
        t = GetBaseTypeOfType(cType)

        if(t == BASE):
            res = Base(name)
            allTypes[name] = res
            return res
        elif(t == TYPEDEF):
            res = Typedef(name,TransformCTypeIntoType(cType.get_canonical()))
            allTypes[name] = res
            return res
        elif(t == TEMPLATE_ARGUMENT):
            res = TemplateArgument(name)
            return res
        elif(t == POINTER):
            res = Pointer(name,TransformCTypeIntoType(cType.get_pointee()))
            allTypes[name] = res
            return res
        elif(t == ARRAY):
            res = Array(name,cType.element_type,cType.get_array_size())
            allTypes[name] = res
            return res
        elif(t == ELABORATED):
            if("enum" in cType.spelling):
                values = []
                for n in cType.get_declaration().get_children():
                    values.append(n.spelling)
                return Enum(cType.spelling,values,GetEntireSourceText(cType.get_declaration()))
            else:
                return Base(cType.spelling)
        else:
            if(cType.kind in [TypeKind.FUNCTIONPROTO,TypeKind.INVALID,TypeKind.VOID]):
                return Base(cType.kind.spelling)

            print("[TransformCTypeIntoType]",cType.spelling,cType.kind,t)
            sys.exit(0)
            return Base("Nil")

def GetBaseTypeOfType(cIndexType: clang.cindex.Type):
    t = cIndexType
    DebugPrint("[GetBaseTypeOfType]:",t.spelling,t.kind)

    if(t.kind in [TypeKind.INT,TypeKind.FLOAT,TypeKind.BOOL,TypeKind.UINT]):
        return BASE
    elif(t.kind == TypeKind.ELABORATED):
        return ELABORATED
    elif(t.kind == TypeKind.RECORD):
        return STRUCT
    elif(t.kind == TypeKind.POINTER):
        return POINTER
    elif(t.kind == TypeKind.ENUM):
        return ENUM
    elif(t.kind == TypeKind.TYPEDEF):
        return TYPEDEF
    elif(t.kind == TypeKind.UNEXPOSED):
        return TEMPLATE_ARGUMENT
    elif(t.kind == TypeKind.CHAR_S):
        return POINTER
    elif(t.kind == TypeKind.CONSTANTARRAY):
        return ARRAY
    else:
        DebugPrint("[GetBaseTypeOfType] Not recognized: " + str(cIndexType.spelling) + "/" + str(t.kind))
        return UNKNOWN

def GetBaseTypeFromCursor(c: Cursor):
    if(c.kind == CursorKind.STRUCT_DECL or c.kind == CursorKind.CLASS_DECL):
        return STRUCT
    elif(c.kind == CursorKind.UNION_DECL):
        return UNION
    elif(c.kind == CursorKind.CLASS_TEMPLATE):
        return TEMPLATED_STRUCT_DEF
    elif(c.kind == CursorKind.ENUM_DECL):
        return ENUM
    elif(c.kind == CursorKind.TYPEDEF_DECL):
        return TYPEDEF
    else:
        DebugPrint("[GetBaseTypeFromCursor] not recognized:",c.spelling,c.kind.spelling)

class Base:
    def __init__(self,name):
        self.name = name
        if("enum" in name):
            print("\n\n")
            traceback.print_stack()
            print("\n\n")

    def Type(self):
        return BASE

    def __repr__(self):
        return self.Type() + ":" + str(self.name)

class Typedef:
    def __init__(self,name,oldType):
        self.name = name
        self.oldType = oldType

    def Type(self):
        return TYPEDEF

    def __repr__(self):
        return self.Type() + ":" + str(self.name) + "=" + str(self.oldType.name)

class Enum:
    def __init__(self,name,values,entireText):
        self.name = name
        self.values = values
        self.entireText = entireText

    def Type(self):
        return ENUM

    def IsUnnamed(self):
        return ("unnamed" in self.name)

    def __repr__(self):
        return self.Type() + ":" + str(self.name) + ":" + str(self.values)

class Pointer:
    def __init__(self,name,type):
        self.name = name
        self.type = type

    def Type(self):
        return POINTER

    def __repr__(self):
        return self.Type() + ":" + str(self.name) + ":" + str(self.type)

class Array:
    def __init__(self,name,baseType,size):
        self.name = name
        self.baseType = baseType
        self.size = size

    def Type(self):
        return ARRAY

    def __repr__(self):
        return self.Type() + ":" + str(self.name) + "[" + str(self.baseType) + "]"

class Member:
    def __init__(self,name,t,position):
        self.name = name
        self.type = t
        self.position = position

    def __repr__(self):
        toReturn = "Member:\n"
        for x,y in self.__dict__.items():
            toReturn += str(x) + ":" + str(y) + "\n"
        return toReturn

class Struct:
    def __init__(self,name,members,templateParameters,inheritBase,comments,entireText,isPOD,isUnion):
        self.name = name
        self.members = members
        self.templateParameters = templateParameters
        self.inheritBase = inheritBase
        self.comments = comments
        self.entireText = entireText
        self.isPOD = isPOD
        self.isUnion = isUnion

    def Type(self):
        if(self.isUnion):
            return UNION
        else:
            return STRUCT

    def IsTemplated(self):
        return (len(self.templateParameters) > 0)

    def __repr__(self):
        toReturn = "Struct:\n"
        for x,y in self.__dict__.items():
            if(x == "entireText"):
                continue

            toReturn += str(x) + ":" + str(y) + "\n"
        return toReturn

def IteratorToList(childrenIterator):
    l = []
    for elem in childrenIterator:
        l.append(elem)
    return l

def MakeTypeFromCursor(c: Cursor):
    global allTypes
    baseType = GetBaseTypeFromCursor(c)

    if(baseType == STRUCT or baseType == UNION or baseType == TEMPLATED_STRUCT_DEF):
        isUnion = (baseType == UNION)

        #print(c.spelling,[x.kind for x in c.get_children()])

        amountOfFields = sum(1 if (x.kind == CursorKind.FIELD_DECL or x.kind == CursorKind.UNION_DECL) else 0 for x in c.get_children())

        if(amountOfFields == 0): # Skips non definitions of structures
            return

        name = c.spelling
        if(name == "WireInformation"):
            print("here")
        members,templateParameters,inheritBase = ParseMembers(c,0,isUnion)

        parsedStruct = Struct(name,members,templateParameters,inheritBase,c.brief_comment,GetEntireSourceText(c),c.type.is_pod(),isUnion)
        allTypes[name] = parsedStruct

        return parsedStruct
    elif(baseType == TYPEDEF):
        name = c.spelling

        res = Typedef(name,TransformCTypeIntoType(c.underlying_typedef_type))
        allTypes[name] = res

        return res
    elif(baseType == ENUM):
        name = c.spelling
        
        values = []
        for n in c.get_children():
            values.append(n.spelling)

        res = Enum(name,values,GetEntireSourceText(c))
        allTypes[name] = res
        return res
    else:
        DebugPrint(f"[{LineNumber()}] Did not expect following type: {baseType}")

def GoodLocation(c: Cursor):
    if(not c.location):
        return None
    if(not c.location.file):
        return None
    if(not c.location.file.name):
        return None

    result = (not "usr" in c.location.file.name and not "lib" in c.location.file.name)

    return result

def PrintRecurse(c: Cursor,indent = 0):
    if(c.raw_comment):
        print(' ' * indent, c.spelling, c.kind,c.brief_comment)
    else:
        print(' ' * indent, c.spelling, c.kind)

    for n in c.get_children():
        PrintRecurse(n, indent + 2)

def GetIndex(iter,index):
    i = 0
    for d in iter:
        if i == index:
            return d
        else:
            i += 1
    return None

def ParseMembers(c,startPosition,isUnion):
    members = []
    templateParameters = []
    inheritBase = None
    index = 0
    for n in c.get_children():
        if(n.kind == CursorKind.UNION_DECL):
            subMembers,_,_ = ParseMembers(n,startPosition + index,True)
            members += subMembers
            if(not isUnion):
                index += 1
        if(n.kind in [CursorKind.CLASS_DECL,CursorKind.STRUCT_DECL]):
            subMembers,_,_ = ParseMembers(n,startPosition + index,False)
            members += subMembers
        if(n.kind == CursorKind.TEMPLATE_TYPE_PARAMETER):
            templateParameters.append(n.spelling)
        if(n.kind == CursorKind.CXX_BASE_SPECIFIER):
            inheritBase = GetIndex(n.get_children(),0).spelling
        if(n.kind == CursorKind.FIELD_DECL):
            if ENABLE_PRINT:
                print("Field Decl:",n.type.spelling, n.spelling, n.type.spelling,n.result_type.spelling,GetBaseTypeOfType(n.type))
            members.append(Member(n.spelling,TransformCTypeIntoType(n.type),startPosition + index))
            #InsertIfNotExists(GetSimplestType(n.type).spelling,UNKNOWN)

            if(not isUnion):
                index += 1

    return members,templateParameters,inheritBase

def Recurse(c: Cursor, indent=0):
    if ENABLE_PRINT_AST:
        print("Top",' ' * indent, c.spelling, c.kind,c.type.kind)
    if(GoodLocation(c)):
        if(c.kind in [CursorKind.STRUCT_DECL,CursorKind.CLASS_DECL,CursorKind.CLASS_TEMPLATE]):
            MakeTypeFromCursor(c)
        if(c.kind == CursorKind.ENUM_DECL):
            MakeTypeFromCursor(c)
        if(c.kind == CursorKind.UNION_DECL):
            MakeTypeFromCursor(c)
        elif(c.kind == CursorKind.TYPEDEF_DECL):
            MakeTypeFromCursor(c)
        elif ENABLE_PRINT_AST:
            for n in c.get_children():
                PrintRecurse(n, indent + 2)

def Recurse2(c: Cursor, indent=0):
    goodCursors = []
    if(GoodLocation(c)):
        if(c.kind in [CursorKind.STRUCT_DECL,CursorKind.CLASS_DECL,CursorKind.CLASS_TEMPLATE,CursorKind.ENUM_DECL,CursorKind.UNION_DECL,CursorKind.TYPEDEF_DECL]):
            goodCursors.append(c)

    return goodCursors

class Counter:
    def __init__(self):
        self.counter = 0

    def Next(self):
        val = self.counter
        self.counter += 1
        return val

def Unique(l,comp):
    toRemoveIndex = []
    for i,x in enumerate(l):
        for j,y in enumerate(l[i+1:]):
            if(comp(x,y)):
                #print(x.spelling,y.spelling,i+j,l[i].spelling,l[i+1+j].spelling)
                toRemoveIndex.append(i+j+1)

    copied = copy.copy(l)
    for i in list(sorted(list(set(toRemoveIndex))))[::-1]:
        #print(i)
        del copied[i]

    return copied

if __name__ == "__main__":
    output_path = sys.argv[1]
    files = sys.argv[2:]

    headers = [x.split("/")[-1] for x in files]
    index = Index.create()
    # TODO: Need to receive args from above
    args = ['-x', 'c++', '-std=c++17', '-fparse-all-comments', '-I../../software/common', '-I../../software']

    allCursors = []
    for file in files:
        tu = TranslationUnit.from_source(file, args,options=TranslationUnit.PARSE_INCLUDE_BRIEF_COMMENTS_IN_CODE_COMPLETION)

        for n in tu.cursor.get_children():
            allCursors += Recurse2(n)

    allCursors = Unique(allCursors,lambda x,y: (x.spelling == y.spelling and x.kind == y.kind))

    #for c in allCursors:
    #    print(c.spelling,c.kind)
    #sys.exit()

    for c in allCursors:
        MakeTypeFromCursor(c)

    if False:
        for file in files:
            tu = TranslationUnit.from_source(file, args,options=TranslationUnit.PARSE_INCLUDE_BRIEF_COMMENTS_IN_CODE_COMPLETION)

            for n in tu.cursor.get_children():
                Recurse(n)
            #break

    intType = allTypes["int"]
    pointerT = Pointer("T*",TemplateArgument("T"))

    allTypes["std::vector"] = Struct("std::vector",[Member("mem",pointerT,0),Member("size",intType,1),Member("allocated",intType,2)],["T"],None,None,"",False,False)

    unknownStructures = []

    for name,data in allTypes.items():
        if(data == UNKNOWN):
            unknownStructures.append(data)

    for name,s in allTypes.items():
        if(s.Type() == STRUCT and s.inheritBase):
            if(s.inheritBase == "T"): continue
            name = s.inheritBase
            if("struct" in s.inheritBase):
                name = s.inheritBase.replace("struct","").strip()
            elem = allTypes[name]

            maxSize = max(x.position for x in elem.members)

            for member in s.members:
                member.position += maxSize + 1

            s.members = elem.members + s.members

    enumStructures = {}
    structStructures = {}
    typedefStructures = {}
    templatedStructures = {}
    for name,data in allTypes.items():
        if(data.Type() == STRUCT and data.IsTemplated()):
            templatedStructures[name] = data
        elif(data.Type() == ENUM):
            enumStructures[name] = data
        elif(data.Type() == TYPEDEF):
            typedefStructures[name] = data
        elif((data.Type() == STRUCT or data.Type() == UNION) and len(data.templateParameters) == 0 and len(data.members) > 0):
            structStructures[name] = data
        elif(data.Type() in [BASE,POINTER,ARRAY]):
            pass
        else:
            print(f"Missing a type: {name}:{data.Type()}\n")
            #sys.exit(-1)

    #print(structStructures["MuxInfo"])
    #print()
    #print()
    print(structStructures["WireInformation"])
    sys.exit()

    typeInfoFile = open(output_path + "/typeInfo.cpp","w")

    typeInfoFile.write("#pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n\n")
    for name in headers:
        typeInfoFile.write(f"#include \"{name}\"\n")
    typeInfoFile.write('\n')

    typeInfoFile.write("void RegisterParsedTypes(){\n")
    for name in unknownStructures:
        typeInfoFile.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Subtype_UNKNOWN,sizeof({name}),alignof({name}));\n")
    for name,t in structStructures.items():
        typeInfoFile.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Subtype_STRUCT,sizeof({name}),alignof({name}));\n")
    for name,t in typedefStructures.items():
        typeInfoFile.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Subtype_TYPEDEF,sizeof({name}),alignof({name}));\n")
    typeInfoFile.write(f"  RegisterOpaqueType(STRING(\"String\"),Subtype_TYPEDEF,sizeof(String),alignof(String));\n") #HACK

    for name,t in allTypes.items():
        if(allTypes[name] == TEMPLATED_INSTANCE):
            typeInfoFile.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Subtype_TEMPLATED_INSTANCE,sizeof({name}),alignof({name}));\n")
    typeInfoFile.write("\n")

    for name,enum in enumStructures.items():
        if(enum.IsUnnamed()):
            continue

        enumString = ",\n    ".join(["{" + f"STRING(\"{val}\"),(int) {name}::{val}" + "}" for val in enum.values])
        typeInfoFile.write(f"static Pair<String,int> {name}Data[] = " + "{" + enumString + "};\n")
        typeInfoFile.write("\n")

        typeInfoFile.write(f"RegisterEnum(STRING(\"{name}\"),sizeof({name}),alignof({name}),C_ARRAY_TO_ARRAY({name}Data));\n")
        typeInfoFile.write("\n")

    counter = Counter()
    accumTemplateArgs = []   
    for name,data in templatedStructures.items():
        accumTemplateArgs += data.templateParameters
    typeInfoFile.write("  static String templateArgs[] = { " + ",\n    ".join([f"STRING(\"{x}\"" + ")" + f" /* {counter.Next()} */" for x in accumTemplateArgs]) + "\n  };")
    typeInfoFile.write("\n")
    typeInfoFile.write("\n")

    index = 0
    for name,data in templatedStructures.items():
        params = len(data.templateParameters)
        typeInfoFile.write(f"  RegisterTemplate(STRING(\"{name}\"),(Array<String>)" + "{" + f"&templateArgs[{index}],{params}" + "}" + ");\n")
        index += params
    typeInfoFile.write("\n")

    for name,data in typedefStructures.items():
        oldName = data.oldType.name
        newName = data.name
        if("<" in oldName):
            continue
        if("(" in oldName):
            continue

        typeInfoFile.write(f"  RegisterTypedef(STRING(\"{oldName}\"),STRING(\"{newName}\"));\n")
    typeInfoFile.write("\n")

    accumTemplateMembers = []   
    for name,data in templatedStructures.items():
        members = data.members
        for m in members:
            accumTemplateMembers.append([m.type.name,m.name,m.position])

    counter = Counter()
    temMembers = ",\n    ".join(["(TemplatedMember)" + "{" f"STRING(\"{x[0]}\"),STRING(\"{x[1]}\"),{x[2]}" + "}" + f" /* {counter.Next()} */" for x in accumTemplateMembers])
    typeInfoFile.write("  static TemplatedMember templateMembers[] = { " + temMembers + "\n  };")
    typeInfoFile.write("\n")
    typeInfoFile.write("\n")

    index = 0
    for name,data in templatedStructures.items():
        size = len(data.members)
        typeInfoFile.write(f"  RegisterTemplateMembers(STRING(\"{name}\"),(Array<TemplatedMember>)" + "{" + f"&templateMembers[{index}],{size}" + "}" + ");\n")
        index += size
    typeInfoFile.write("\n")

    allMembers = []
    for name,data in structStructures.items():
        allMembers += [(data.name,y.type.name,y.name) for y in data.members]

    def CleanEnum(x):
        if("unnamed" in x):
            return "int"
        else:
            return x

    counter = Counter()
    memb = ",\n    ".join(["(Member)" + "{" f"GetTypeOrFail(STRING(\"{CleanEnum(x[1])}\")),STRING(\"{x[2]}\"),offsetof({x[0]},{x[2]})" + "}" + f" /* {counter.Next()} */" for x in allMembers])
    typeInfoFile.write("  static Member members[] = {" + memb + "\n  };")
    typeInfoFile.write("\n")
    typeInfoFile.write("\n")

    #RegisterStructMembers(STRING("Time"),(Array<Member>){&members[0],2});

    index = 0
    for name,data in structStructures.items():
        size = len(data.members)
        typeInfoFile.write(f"  RegisterStructMembers(STRING(\"{data.name}\"),(Array<Member>)" + "{" + f"&members[{index}],{size}" + "})" + ";\n")
        index += size
    typeInfoFile.write("\n")

    for name,data in allTypes.items():
        if(data == TEMPLATED_INSTANCE):
            typeInfoFile.write(f"  InstantiateTemplate(STRING(\"{name}\"));\n")
    typeInfoFile.write("\n")

    # TODO: Hack because the typedef was inserted before template info was inserted
    #typeInfoFile.write("  RegisterTypedef(STRING(\"Array<char>\"),STRING(\"String\"));\n")

    for name,data in typedefStructures.items():
        oldName = data.oldType.name
        newName = data.name
        if(not "<" in oldName):
            continue
        if("(" in oldName):
            continue

        typeInfoFile.write(f"  RegisterTypedef(STRING(\"{oldName}\"),STRING(\"{newName}\"));\n")
    typeInfoFile.write("\n")

    typeInfoFile.write("}\n")

    typeInfoFile.close()

    #
    #
    #

    reprSource = open(output_path + "/autoRepr.cpp","w")
    reprHeader = open(output_path + "/autoRepr.hpp","w")

    reprSource.write("// File auto generated by structParser.py\n")
    reprHeader.write("// File auto generated by structParser.py\n")

    reprHeader.write("#pragma once\n\n")
    
    for file in files:
        lastSep = file.rfind("/")
        reprHeader.write(f"#include \"{file[lastSep+1:]}\"\n")
        #reprSource.write(f"#include \"{file}\"\n")
    reprSource.write("#include \"autoRepr.hpp\"\n")
    reprSource.write("\n")

    '''
    def CheckDeriveFormat(struct):
        comment = struct.comments
        if not comment:
            return False
        if("Derive" in comment and "Format" in comment):
            return True
        return False

    reprSource.write("template<typename T>\n")
    reprSource.write("String GetRepr(T** pointer,Arena* out){\n")
    reprSource.write("  return PushString(out,\"%p\",pointer);\n")
    reprSource.write("}\n")

    simpleFormats = [("int","d"),
                     ("long int","ld"),
                     ("size_t","zd"),
                     ("unsigned int","u"),
                     ("float","f"),
                     ("double","f"),
                     ("char","c")]

    for formType,form in simpleFormats:
        reprSource.write(f"String GetRepr({formType}* val,Arena* out)" + "{\n")
        reprSource.write(f"  return PushString(out,\"%{form}\",*val);\n")
        reprSource.write("}\n")

    reprSource.write("String GetRepr(bool* boolean,Arena* out){\n")
    reprSource.write("  return PushString(out,\"%d\",*boolean ? 1 : 0);\n")
    reprSource.write("}\n")

    reprSource.write("String GetRepr(String* str,Arena* out){\n")
    reprSource.write("  return PushString(out,*str);\n")
    reprSource.write("}\n")

    reprSource.write("template<typename T>\n")
    reprSource.write("String GetRepr(Opt<T>* optional,Arena* out){\n")
    reprSource.write("  Opt<T> opt = *optional;\n")
    reprSource.write("  if(opt.has_value()){\n")
    reprSource.write("    return GetRepr(&opt.value(),out);\n")
    reprSource.write("  } else {\n")
    reprSource.write("    return PushString(out,STRING(\"-\"));\n")
    reprSource.write("  }\n")
    reprSource.write("}\n")

    reprSource.write("""template<typename T>
String GetRepr(Pool<T>* pool,Arena* out){
  auto mark = StartString(out);
  PushString(out,"(");
  bool first = true;
  for(T* t : *pool){
    if(first){
      first = false;
    } else {
      PushString(out,",");
    }
    GetRepr(t,out);
  }
  return EndString(mark);
}
""")

    reprSource.write("""String GetRepr(FUDeclaration** decl,Arena* out){
        if(*decl){
            return PushString(out,(*decl)->name);
        } else{
            return PushString(out,"-");
        }
    }
""")

    reprSource.write("""String GetRepr(Array<int>* arr,Arena* out){
      auto mark = StartString(out);
      PushString(out,"(");
      bool first = true;
      for(int t : *arr){
        if(first){
          first = false;
        } else {
          PushString(out,",");
        }
        GetRepr(&t,out);
      }
      PushString(out,")");
      return EndString(mark);
    }
""")

    reprSource.write("template<typename T>\n")
    reprSource.write("String GetRepr(Array<T>* arr,Arena* out){\n")
    reprSource.write("  return PushString(out,\"(array:%d)\",arr->size);\n")
    reprSource.write("}\n")


    reprSource.write("template<typename T>\n")
    reprSource.write("String GetRepr(Range<T>* range,Arena* out){\n")
    reprSource.write("  return PushString(out,STRING(\"(range)\"));\n")
    reprSource.write("}\n")

    structsToOutput = []
    typesToSee = set()

    notPrintingList = ["SpecExpression","MappingNode","ConsolidationResult",
                       "MergeEdge","MemoryAddressMask","Type",
                       "TypeIterator","Iterator","Expression",
                       "Trie","CommandDefinition","Block","TemplateRecord",
                       "IndividualBlock","PortDeclaration","ExternalMemoryID","ExternalMemoryInfo",
                       "PortEdge","MappingEdge","Accelerator","InstanceNode","DelayToAdd","Edge","AcceleratorMapping"]

    def CheckIfPrinting(struct):
        if(struct.name in notPrintingList):
            return False

        if(CheckDeriveFormat(struct) or struct.isPOD):
            return True
        return False

    for name,s in structStructures.items():
        if(CheckIfPrinting(s)):
            structsToOutput.append(s)

    for s in structsToOutput:
        for member in s.members:
            typesToSee.add(member.type)

    reprHeader.write("\n")
    '''

    for name,s in enumStructures.items():
        if(s.IsUnnamed()):
            continue

        reprHeader.write(f"String GetRepr({name}* e,Arena* out);\n")

        reprSource.write(f"String GetRepr({name}* e,Arena* out)" + "{\n")
        reprSource.write("  switch(*e){\n")

        for value in s.values:
            reprSource.write(f"  case {name}::{value}: return PushString(out,STRING(\"{value}\"));\n")
        reprSource.write("  }\n")
        reprSource.write("  return STRING(\"NOT POSSIBLE ENUM VALUE\");\n")
        reprSource.write("}\n")

    '''
    def NormalRepr(structure,header,source):
        header.write(f"String GetRepr({name}* s,Arena* out);\n")

        source.write(f"String GetRepr({name}* s,Arena* out)" + "{\n")
        source.write("  auto mark = StartString(out);\n")

        for member in s.members:
            if(member.type.Type() == ENUM and member.type.IsUnnamed()):
                continue
            source.write(f"  GetRepr(&s->{member.name},out);\n")

        source.write("  String result = EndString(mark);\n")
        source.write("  return result;\n}\n")

    for name,s in structStructures.items():
        if(CheckIfPrinting(s)):
            NormalRepr(s,reprHeader,reprSource)

    reprHeader.write("\n")

    for name,s in structStructures.items():
        if(CheckIfPrinting(s)):
            reprHeader.write(f"Array<String> GetAllRepr({name}* s,Arena* out);\n")

            reprSource.write(f"Array<String> GetAllRepr({name}* s,Arena* out)" + "{\n")
            reprSource.write(f"  Array<String> arr = PushArray<String>(out,{len(s.members)});\n")

            index = 0
            for member in s.members:
                if(member.type.Type() == ENUM and member.type.IsUnnamed()):
                    continue
                reprSource.write(f"  arr[{index}] = GetRepr(&s->{member.name},out);\n")
                index += 1

            reprSource.write("  return arr;\n}\n")

    reprHeader.write("\n")

    for name,s in structStructures.items():
        if(CheckIfPrinting(s)):
            reprHeader.write(f"String GetFullRepr({name}* s,Arena* out);\n")

            reprSource.write(f"String GetFullRepr({name}* s,Arena* out)" + "{\n")
            reprSource.write("  auto mark = StartString(out);\n")

            index = 0
            for member in s.members:
                if(member.type.Type() == ENUM and member.type.IsUnnamed()):
                    continue
                reprSource.write(f"  PushString(out,\"{member.name}:\");\n")
                reprSource.write(f"  GetRepr(&s->{member.name},out);\n")
                reprSource.write(f"  PushString(out,\"\\n\");\n")
                index += 1

            reprSource.write("  String result = EndString(mark);\n")
            reprSource.write("  return result;\n}\n")

    reprHeader.write("\n")

    for name,s in structStructures.items():
        if(CheckIfPrinting(s)):
            reprHeader.write(f"Array<String> GetFields({name}* marker,Arena* out);\n")

            reprSource.write(f"Array<String> GetFields({name}* marker,Arena* out)" + "{\n")
            reprSource.write(f"  Array<String> arr = PushArray<String>(out,{len(s.members)});\n")

            index = 0
            for member in s.members:
                if(member.type.Type() == ENUM and member.type.IsUnnamed()):
                    continue
                reprSource.write(f"  arr[{index}] = STRING(\"{member.name}\");\n")
                index += 1

            reprSource.write("  return arr;\n}\n")

    reprHeader.write("\n")

    reprSource.close()
    reprHeader.close()

    for name,x in allTypes.items():
        if(x.Type() == STRUCT and not x.isPOD):
            print(x.name)
    '''
    #print(allTypes)

# TODO() - Removing the printing of unnamed enums is not needed (we could have code that prints it) and it messes the allocated array

