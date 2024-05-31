from clang.cindex import Cursor, Index, Config, CursorKind
import sys

# python3 structParser.py ../../build ../../software/common/utils.hpp ../../software/common/utilsCore.hpp ../../software/common/memory.hpp ../../software/common/templateEngine.hpp ../../software/common/parser.hpp ../../software/common/verilogParsing.hpp ../../software/compiler/versat.hpp ../../software/compiler/declaration.hpp ../../software/compiler/accelerator.hpp ../../software/compiler/configurations.hpp ../../software/compiler/codeGeneration.hpp ../../software/compiler/versatSpecificationParser.hpp

TYPEDEF = "TYPEDEF"
STRUCT = "STRUCT"
UNKNOWN = "UNKNOWN"
TEMPLATED_INSTANCE = "TEMPLATED_INSTANCE"
ENUM = "ENUM"

ENABLE_PRINT = False

class Typedef:
    def __init__(self,oldName,newName):
        self.oldName = oldName
        self.newName = newName

    def Type(self):
        return TYPEDEF

    def __repr__(self):
        return str(self.newName) + "=" + str(self.oldName)

class Member:
    def __init__(self,typeName,name,position):
        self.typeName = typeName
        self.name = name
        self.position = position

    def __repr__(self):
        return str(self.typeName) + " " + str(self.name) + ":" + str(self.position)

class Enum:
    def __init__(self,name,values):
        self.name = name
        self.values = values

    def Type(self):
        return ENUM

    def __repr__(self):
        return str(self.name) + ":" + str(self.values)

class Struct:
    def __init__(self,name,members,templateParameters,inheritBase):
        self.name = name
        self.members = members
        self.templateParameters = templateParameters
        self.inheritBase = inheritBase

    def Type(self):
        return STRUCT

    def __repr__(self):
        return str(self.name) + ":" + str(self.inheritBase) + "=" + str(self.members) + ":" + str(self.templateParameters)

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
    print(' ' * indent, c.spelling, c.kind)
    for n in c.get_children():
        PrintRecurse(n, indent + 2)

def InsertIfNotExists(key,data):
    global allTypes
    try:
        val = allTypes[key]
        if val == None or val == UNKNOWN:
            if val != UNKNOWN and data == UNKNOWN:
                print(f"Trying to insert unkown for type that already exists {key}:{val}")
            allTypes[key] = data
    except:
        allTypes[key] = data

def GetSimplestType(t):
    return t.get_canonical().get_class_type()

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
            #inheritBase = n.spelling
            #print(inheritBase)
            inheritBase = GetIndex(n.get_children(),0).spelling
        if(n.kind == CursorKind.FIELD_DECL):
            if ENABLE_PRINT:
                print(n.type.spelling, n.spelling)
            members.append(Member(n.type.spelling,n.spelling,startPosition + index))
            InsertIfNotExists(GetSimplestType(n.type).spelling,UNKNOWN)

            if(not isUnion):
                index += 1

    return members,templateParameters,inheritBase

def ParseStruct(c: Cursor,isUnion):
    global structures

    if(sum(1 for _ in c.get_children()) == 0): # Skips non definitions of structures
        return

    name = c.spelling
    #print(name)
    members,templateParameters,inheritBase = ParseMembers(c,0,isUnion)

    parsedStruct = Struct(name,members,templateParameters,inheritBase)
    if(len(parsedStruct.members) > 0): # Difference between struct declaration or definition
        structures[name] = parsedStruct

    InsertIfNotExists(name,STRUCT)

def ParseTypedef(c: Cursor):
    global structures

    name = c.spelling
    value = c.underlying_typedef_type.spelling
    structures[name] = Typedef(value,name)
    InsertIfNotExists(name,TYPEDEF)
    InsertIfNotExists(value,UNKNOWN)

def ParseEnum(c: Cursor):
    global structures
    name = c.spelling
    
    values = []
    for n in c.get_children():
        values.append(n.spelling)

    structures[name] = Enum(name,values)

def Recurse(c: Cursor, indent=0):
    if ENABLE_PRINT:
        print("Top",' ' * indent, c.spelling, c.kind)
    if(GoodLocation(c)):
        if(c.kind in [CursorKind.STRUCT_DECL,CursorKind.CLASS_TEMPLATE]):
            ParseStruct(c,False)
        if(c.kind == CursorKind.ENUM_DECL):
            ParseEnum(c)
        if(c.kind == CursorKind.UNION_DECL):
            ParseStruct(c,True)
        elif(c.kind == CursorKind.TYPEDEF_DECL):
            #print(c.location)
            ParseTypedef(c)
        elif ENABLE_PRINT:
            for n in c.get_children():
                PrintRecurse(n, indent + 2)

class Counter:
    def __init__(self):
        self.counter = 0

    def Next(self):
        val = self.counter
        self.counter += 1
        return val

if __name__ == "__main__":
    global structures
    global allTypes

    structures = {}
    allTypes = {}
    output_path = sys.argv[1]
    files = sys.argv[2:]

    headers = [x.split("/")[-1] for x in files]
    index = Index.create()
    # TODO: Need to receive args from above
    args = ['-x', 'c++', '-std=c++17', '-I../../software/common', '-I../../software']

    for file in files:
        tu = index.parse(file, args)

        for n in tu.cursor.get_children():
            Recurse(n)

    extra_code = "namespace std{ template<typename T> struct vector{T* mem; int size; int allocated;};}"

    structures["std::vector"] = Struct("std::vector",[Member("T*","mem",0),Member("int","size",1),Member("int","allocated",2)],["T"],None)

    for name,t in allTypes.items():
        if("<" in name and t == UNKNOWN):
            allTypes[name] = TEMPLATED_INSTANCE

    toDelete = []
    for name,t in allTypes.items():
        if(len(name) <= 0 or '(' in name):
            toDelete.append(name)
    for name in toDelete:
        del allTypes[name]

    # TODO: Hack because the typedef was inserted before template info was inserted
    #       Basically need to output following the order in which the types are declared
    for name,t in structures.items():
        if(t.Type() == TYPEDEF and t.newName == "String"):
            del structures[name]
            break

    toDelete = []
    for name,t in structures.items():
        if(t.Type() == TYPEDEF):
            if(len(t.oldName) <= 0 or '(' in t.oldName):
                toDelete.append(name)
            if(len(t.newName) <= 0 or '(' in t.newName):
                toDelete.append(name)

        if(len(name) <= 0 or '(' in name):
            toDelete.append(name)
    for name in toDelete:
        del structures[name]

    unknownStructures = []

    for name,data in allTypes.items():
        if(data == UNKNOWN):
            unknownStructures.append(name)

    for name,s in structures.items():
        if(s.Type() == STRUCT and s.inheritBase):
            if(s.inheritBase == "T"): continue
            elem = structures[s.inheritBase]

            maxSize = max(x.position for x in elem.members)

            for member in s.members:
                member.position += maxSize + 1

            s.members = elem.members + s.members

    for name,data in structures.items():
        if(data.Type() == STRUCT):
            data.name = data.name.replace("const ","")
            for m in data.members:
                m.typeName = m.typeName.replace("const ","")
        if(data.Type() == TYPEDEF):
            data.oldName = data.oldName.replace("const ","")

    templatedStructures = {}
    typedefStructures = {}
    structStructures = {}
    enumStructures = {}
    for name,data in structures.items():
        if(data.Type() == STRUCT):
            if(len(data.templateParameters) > 0):
                templatedStructures[name] = data
        if(data.Type() == ENUM):
            enumStructures[name] = data
        if(data.Type() == TYPEDEF):
            typedefStructures[name] = data
        if(data.Type() == STRUCT and len(data.templateParameters) == 0 and len(data.members) > 0):
            structStructures[name] = data

    f = open(output_path + "/typeInfo.cpp","w")

    f.write("#pragma GCC diagnostic ignored \"-Winvalid-offsetof\"\n\n")
    for name in headers:
        f.write(f"#include \"{name}\"\n")
    f.write('\n')

    f.write("void RegisterParsedTypes(){\n")
    for name in unknownStructures:
        f.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Type::UNKNOWN,sizeof({name}),alignof({name}));\n")
    for name,t in structStructures.items():
        f.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Type::STRUCT,sizeof({name}),alignof({name}));\n")
    for name,t in typedefStructures.items():
        f.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Type::TYPEDEF,sizeof({name}),alignof({name}));\n")
    f.write(f"  RegisterOpaqueType(STRING(\"String\"),Type::TYPEDEF,sizeof(String),alignof(String));\n") #HACK
    for name,t in allTypes.items():
        if(allTypes[name] == TEMPLATED_INSTANCE):
            f.write(f"  RegisterOpaqueType(STRING(\"{name}\"),Type::TEMPLATED_INSTANCE,sizeof({name}),alignof({name}));\n")
    f.write("\n")

    for name,enum in enumStructures.items():
        enumString = ",\n    ".join(["{" + f"STRING(\"{val}\"),(int) {name}::{val}" + "}" for val in enum.values])
        f.write(f"static Pair<String,int> {name}Data[] = " + "{" + enumString + "};\n")
        f.write("\n")

        f.write(f"RegisterEnum(STRING(\"{name}\"),C_ARRAY_TO_ARRAY({name}Data));\n")
        f.write("\n")

    counter = Counter()
    accumTemplateArgs = []   
    for name,data in templatedStructures.items():
        accumTemplateArgs += data.templateParameters
    f.write("  static String templateArgs[] = { " + ",\n    ".join([f"STRING(\"{x}\"" + ")" + f" /* {counter.Next()} */" for x in accumTemplateArgs]) + "\n  };")
    f.write("\n")
    f.write("\n")

    index = 0
    for name,data in templatedStructures.items():
        params = len(data.templateParameters)
        f.write(f"  RegisterTemplate(STRING(\"{name}\"),(Array<String>)" + "{" + f"&templateArgs[{index}],{params}" + "}" + ");\n")
        index += params
    f.write("\n")

    for name,data in typedefStructures.items():
        oldName = data.oldName
        newName = data.newName
        f.write(f"  RegisterTypedef(STRING(\"{oldName}\"),STRING(\"{newName}\"));\n")
    f.write("\n")

    accumTemplateMembers = []   
    for name,data in templatedStructures.items():
        members = data.members
        for m in members:
            accumTemplateMembers.append([m.typeName,m.name,m.position])

    counter = Counter()
    temMembers = ",\n    ".join(["(TemplatedMember)" + "{" f"STRING(\"{x[0]}\"),STRING(\"{x[1]}\"),{x[2]}" + "}" + f" /* {counter.Next()} */" for x in accumTemplateMembers])
    f.write("  static TemplatedMember templateMembers[] = { " + temMembers + "\n  };")
    f.write("\n")
    f.write("\n")

    index = 0
    for name,data in templatedStructures.items():
        size = len(data.members)
        f.write(f"  RegisterTemplateMembers(STRING(\"{name}\"),(Array<TemplatedMember>)" + "{" + f"&templateMembers[{index}],{size}" + "}" + ");\n")
        index += size
    f.write("\n")

    allMembers = []
    for name,data in structStructures.items():
        allMembers += [(data.name,y.typeName,y.name) for y in data.members]

    counter = Counter()
    memb = ",\n    ".join(["(Member)" + "{" f"GetType(STRING(\"{x[1]}\")),STRING(\"{x[2]}\"),offsetof({x[0]},{x[2]})" + "}" + f" /* {counter.Next()} */" for x in allMembers])
    f.write("  static Member members[] = {" + memb + "\n  };")
    f.write("\n")
    f.write("\n")

    #RegisterStructMembers(STRING("Time"),(Array<Member>){&members[0],2});

    index = 0
    for name,data in structStructures.items():
        size = len(data.members)
        f.write(f"  RegisterStructMembers(STRING(\"{data.name}\"),(Array<Member>)" + "{" + f"&members[{index}],{size}" + "})" + ";\n")
        index += size
    f.write("\n")

    for name,data in allTypes.items():
        if(data == TEMPLATED_INSTANCE):
            f.write(f"  InstantiateTemplate(STRING(\"{name}\"));\n")
    f.write("\n")

    # TODO: Hack because the typedef was inserted before template info was inserted
    f.write("  RegisterTypedef(STRING(\"Array<char>\"),STRING(\"String\"));\n")

    f.write("}\n")


    f.close()
