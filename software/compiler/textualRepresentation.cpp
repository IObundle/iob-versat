#include "textualRepresentation.hpp"
#include "memory.hpp"
#include "type.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"
#include <dirent.h>

#include "declaration.hpp"

#if 1

// For now hardcoded for 32 bits.
String BinaryRepr(int number,int bitsize,Arena* out){
  Byte* buffer = PushBytes(out,bitsize);

  for(int i = 0; i < bitsize; i++){
    buffer[i] = GET_BIT(number,31 - i) ? '1' : '0';
  }
  
  String res = {};
  res.data = (char*) buffer;
  res.size = bitsize;
  return res;
}

String UniqueRepr(FUInstance* inst,Arena* out){
  FUDeclaration* decl = inst->declaration;
  String str = PushString(out,"%.*s_%.*s_%d",UNPACK_SS(decl->name),UNPACK_SS(inst->name),inst->id);
  return str;
}

String Repr(FUInstance* inst,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  FUInstance* instance = (FUInstance*) inst;

  bool expl  = format & GRAPH_DOT_FORMAT_EXPLICIT;
  bool name  = format & GRAPH_DOT_FORMAT_NAME;
  bool type  = format & GRAPH_DOT_FORMAT_TYPE;
  bool id    = format & GRAPH_DOT_FORMAT_ID;
  bool delay = format & GRAPH_DOT_FORMAT_DELAY;

  bool buffer = (inst->declaration == BasicDeclaration::buffer || inst->declaration == BasicDeclaration::fixedBuffer);

  if(expl && name){
    PushString(out,"Name:");
  }
  if(name){
    PushString(out,"%.*s",UNPACK_SS(inst->name));
  }
  if(expl && type){
    PushString(out,"\\nType:");
  }
  if(type){
    PushString(out,"%.*s",UNPACK_SS(inst->declaration->name));
  }
  if(expl && id){
    PushString(out,"\\nId:");
  }
  if(id){
    PushString(out,"%d",inst->id);
  }
  if(expl && delay){
    if(buffer){
      PushString(out,"\\nBuffer:");
    } else {
      PushString(out,"\\nDelay:");
    }
  }
  if(delay){
    if(buffer){
      PushString(out,"%d",instance->bufferAmount);
    } else {
      PushString(out,"0");
      //PushString(out,"%d",inst->baseDelay); //TODO: Broken 
    }
  }

  String res = EndString(mark);
  return res;
}

String Repr(FUDeclaration* decl,Arena* out){
  if(decl == nullptr){
    return PushString(out,"(null)");
  }
  String res = PushString(out,"%.*s",UNPACK_SS(decl->name));
  return res;
}

String Repr(FUDeclaration** decl,Arena* out){
  return Repr(*decl,out);
}

String Repr(PortInstance* inPort,PortInstance* outPort,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  bool expl = format & GRAPH_DOT_FORMAT_EXPLICIT;
  bool lat  = format & GRAPH_DOT_FORMAT_LATENCY;

  Repr(inPort,format,out);

  if(expl && lat){
    PushString(out,"\\nLat:");
  }
  if(lat){
    PushString(out,"%d",inPort->inst->declaration->baseConfig.outputLatencies[inPort->port]);
  }

  PushString(out,"\\n->\\n");
  Repr(outPort,format,out);
  if(expl && lat){
    PushString(out,"\\nLat:");
  }
  if(lat){
    PushString(out,"%d",outPort->inst->declaration->baseConfig.inputDelays[outPort->port]);
  }

  String res = EndString(mark);
  return res;
}

String Repr(PortInstance* port,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  Repr(port->inst,GRAPH_DOT_FORMAT_NAME,out);

  bool expl = format & GRAPH_DOT_FORMAT_EXPLICIT;

  if(expl){
    PushString(out,"_Port:");
  }
  PushString(out,":%d",port->port);

  String res = EndString(mark);
  return res;
}

String Repr(Edge* edge,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  format |= GRAPH_DOT_FORMAT_NAME;

  Repr(&edge->units[0],format,out);
  PushString(out," -- ");
  Repr(&edge->units[1],format,out);

  String res = EndString(mark);
  return res;
}

String Repr(MergeEdge* node,GraphDotFormat format,Arena* out){
  auto mark = StartString(out);

  format |= GRAPH_DOT_FORMAT_NAME;

  Repr(node->instances[0],format,out);
  PushString(out," -- ");
  Repr(node->instances[1],format,out);

  String name = EndString(mark);

  return name;
}

String Repr(MappingNode* node,Arena* out){
  String name = {};
  GraphDotFormat format = GRAPH_DOT_FORMAT_NAME;

  if(node->type == MappingNode::NODE){
    name = Repr(&node->nodes,format,out);
  } else if(node->type == MappingNode::EDGE){
    Edge e0 = node->edges[0];
    Edge e1 = node->edges[1];

    auto mark = StartString(out);
    Repr(&e0,format,out);
    PushString(out," // ");
    Repr(&e1,format,out);
    name = EndString(mark);
  } else {
    NOT_IMPLEMENTED("Implement as needed");
  }

  return name;
}

String Repr(Accelerator* accel,Arena* out){
  return PushString(out,"%d",accel->id);
}

String PushIntTableRepresentation(Arena* out,Array<int> values,int digitSize){
  int maxDigitSize = 0;

  for(int val : values){
    maxDigitSize = std::max(maxDigitSize,NumberDigitsRepresentation(val));
  }

  if(digitSize > maxDigitSize){
    maxDigitSize = digitSize;
  }

  int valPerLine = 80 / (maxDigitSize + 1); // +1 for spaces
  auto mark = StartString(out);
  for(int i = 0; i < values.size; i++){
    if((i % valPerLine == 0) && i != 0){
      PushString(out,"\n");
    }

    PushString(out,"%.*d ",maxDigitSize,values[i]);
  }

  String res = EndString(mark);
  return res;
}

String Repr(StaticId* id,Arena* out){
  auto mark = StartString(out);

  Repr(id->parent,out);
  PushString(out," %.*s",UNPACK_SS(id->name));

  String res = EndString(mark);
  return res;
}

String Repr(Wire* wire,Arena* out){
  auto mark = StartString(out);

  PushString(out,wire->name);
  PushString(out,":%d",wire->bitSize);

  String res = EndString(mark);
  return res;
}

String Repr(int* i,Arena* out){
  if(i == nullptr){
    return PushString(out,STRING("(null)"));
  }

  return PushString(out,"%d",*i);
}

String Repr(long int* i,Arena* out){
  if(i == nullptr){
    return PushString(out,STRING("(null)"));
  }
  return PushString(out,"%ld",*i);
}

String Repr(bool* b,Arena* out){
  if(b == nullptr){
    return PushString(out,STRING("(null)"));
  }

  return PushString(out,"%c",*b ? '1' : '0');
}

String Repr(char** ch,Arena* out){
  if(*ch == nullptr){
    return PushString(out,STRING("(null)"));
  }
  return PushEscapedString(out,STRING(*ch),' ');
}

String Repr(String* str,Arena* out){
  return PushString(out,*str);
}

String Repr(ExternalMemoryInterfaceTemplate<int>* ext, Arena* out){
  if(ext == nullptr){
    return PushString(out,STRING("(null)"));
  }

  switch(ext->type){
  case TWO_P:{
    return PushString(out,STRING("TWO_P"));
  } break;
  case DP:{
    return PushString(out,STRING("TWO_P"));
  } break;
  }

  NOT_POSSIBLE("Unreachable");
  return {};
}

String Repr(TypeStructInfoElement* elem,Arena* out){
  NOT_IMPLEMENTED("Implement if needed");
  return {};
  //return PushString(out,"[%.*s]%.*s",UNPACK_SS(elem->type),UNPACK_SS(elem->name));
}

String Repr(TypeStructInfo* info,Arena* out){
  auto mark = StartString(out);
  Repr(&info->name,out);
  PushString(out,"\n");
  for(TypeStructInfoElement& elem : info->entries){
    Repr(&elem,out);
    PushString(out,"\n");
  }
  return EndString(mark);
}

String Repr(FUInstance* node,Arena* out){
  return Repr(node,GRAPH_DOT_FORMAT_NAME,out);
}

String Repr(Opt<int>* opt,Arena* out){
  if(opt->has_value()){
    return Repr(&opt->value(),out);
  } else {
    return PushString(out,"-");
  }
}

void PrintAll(FILE* file,Array<String> fields,Array<Array<String>> content,Arena* temp){
  BLOCK_REGION(temp);

  int fieldSize = fields.size;
  Array<int> maxSize = PushArray<int>(temp,fieldSize);
  
  Memset(maxSize,0);

  // Max size of fields name
  for(int i = 0; i < fields.size; i++){
    maxSize[i] = std::max(maxSize[i],fields[i].size);
  }
  // Max size of content
  for(int i = 0; i < content.size; i++){
    for(int ii = 0; ii < fieldSize; ii++){
      maxSize[ii] = std::max(maxSize[ii],content[i][ii].size);
    }
  }

  fprintf(file,"index ");
  for(int i = 0; i < fieldSize; i++){
    fprintf(file,"%*.*s ",maxSize[i],UNPACK_SS(fields[i]));
  }
  fprintf(file,"\n");

  for(int i = 0; i < content.size; i++){
    fprintf(file,"%*d ",5,i);
    for(int ii = 0; ii < fieldSize; ii++){
      fprintf(file,"%*.*s ",maxSize[ii],UNPACK_SS(content[i][ii]));
    }
    fprintf(file,"\n");
  }

  fprintf(file,"index ");
  for(int i = 0; i < fieldSize; i++){
    fprintf(file,"%*.*s ",maxSize[i],UNPACK_SS(fields[i]));
  }
  fprintf(file,"\n");
}

void PrintAll(Array<String> fields,Array<Array<String>> content,Arena* temp){
  PrintAll(stdout,fields,content,temp);
}

enum ReprInfoType{
  ReprInfoType_STRING,
  ReprInfoType_REF,
  ReprInfoType_COLLECTION,
  ReprInfoType_TABLE_COLLECTION
};

struct ReprInfo{
  String repr;

  // TODO: For now do not use union, do not know which data we might use or not.
  Array<String> names;
  Array<ReprInfo> moreRepr;
  Array<Array<ReprInfo>> table;
  int refIndex;
  Value originalValue;

  ReprInfoType type;
};

static int globalRefIndex = 0;

bool IsContainer(Type* type){
  return IsIterable(type);
}

Type* GetContainerType(Type* type){
  return GetBaseTypeOfIterating(type);
}

bool SpecialRepresentation(Type* type){
  return IsStringLike(type);
}

ReprInfo GetSimplestForm(Value val,Arena* out){
  Type* type = val.type;

  ReprInfo toReturn = {};
  toReturn.originalValue = val;

  void* addr = GetValueAddress(val);

  if(IsStringLike(type)){
    toReturn.repr = GetDefaultValueRepresentation(val,out);
    toReturn.type = ReprInfoType_STRING;
    return toReturn;
  }
  
  if(type == GetType(STRING("FUDeclaration*"))){
    FUDeclaration* res = *(FUDeclaration**) val.custom;
    if(!res){
      toReturn.repr = STRING("null");
      toReturn.type = ReprInfoType_STRING;
      
    } else {
      toReturn.repr = res->name;
      toReturn.type = ReprInfoType_REF;
    }
    return toReturn;
  }

  if(IsContainer(type)){
    int size = IndexableSize(val);

    if(size < 4 && GetContainerType(type)->type == Subtype_BASE){
      DynamicString str = StartString(out);

      PushString(out,"(");
      Iterator iter = Iterate(val);
      bool first = true;
      while(HasNext(iter)){
        Value val = GetValue(iter);

        if(!first){
          PushString(out,",");
        }
        
        GetDefaultValueRepresentation(val,out);

        first = false;
        Advance(&iter);
      }
      PushString(out,")");

      toReturn.repr = EndString(str);
      toReturn.type = ReprInfoType_STRING;
      return toReturn;
    }
  }
  
  switch(val.type->type){
  case Subtype_UNKNOWN:{
    Assert(false);
  } break;
  case Subtype_ENUM: 
  case Subtype_BASE: {
    toReturn.repr = GetDefaultValueRepresentation(val,out);
    toReturn.type = ReprInfoType_STRING;
  } break;
  case Subtype_TEMPLATED_INSTANCE:
  case Subtype_ARRAY:
  case Subtype_POINTER:
  case Subtype_STRUCT:{
    if(addr == nullptr){
      toReturn.repr = STRING("null");
      toReturn.type = ReprInfoType_STRING;
    } else {
      toReturn.repr = PushString(out,"%p",addr);
      toReturn.type = ReprInfoType_REF;
    }
  } break;
  case Subtype_TEMPLATED_STRUCT_DEF:{
    Assert(false);
  } break;
  case Subtype_TYPEDEF:{
    toReturn = GetSimplestForm(RemoveTypedefIndirection(val),out);
  } break;
  }

  return toReturn;
}

ReprInfo GetReprInfo(Value val,Arena* out,Arena* temp){
  bool isTableContainer = false;
  bool isContainer = false;
  Type* type = val.type;
  Type* possibleStruct = nullptr;
  
  if(IsContainer(type)){
    isContainer = true;

    possibleStruct = GetContainerType(type);
    
    if(IsStruct(possibleStruct) && !IsContainer(possibleStruct) && !SpecialRepresentation(possibleStruct)){
      isTableContainer = true;
    }
  }

  ReprInfo toReturn = {};
  toReturn.originalValue = val;

  if(SpecialRepresentation(type)){
    toReturn.repr = GetDefaultValueRepresentation(val,out);
    toReturn.type = ReprInfoType_STRING;
  } else if(isTableContainer){
    ArenaList<Array<ReprInfo>>* list = PushArenaList<Array<ReprInfo>>(temp);

    Iterator iter = Iterate(val);
    while(HasNext(iter)){
      Value structure = GetValue(iter);
      Type* structType = structure.type;
        
      Array<ReprInfo> arr = PushArray<ReprInfo>(out,structType->members.size);

      Assert(structType->members.size > 0);
      
      int index = 0;
      for(Member& m : structType->members){
        Opt<Value> subVal = AccessStruct(structure,&m);
        Assert(subVal.has_value());

        arr[index++] = GetSimplestForm(subVal.value(),out);
      }
      *PushListElement(list) = arr;
      
      Advance(&iter);
    }

    toReturn.table = PushArrayFromList(out,list);
    toReturn.type = ReprInfoType_TABLE_COLLECTION;
    toReturn.names = GetStructMembersName(possibleStruct,out);
  } else if(isContainer){
    ArenaList<ReprInfo>* list = PushArenaList<ReprInfo>(temp);

    Iterator iter = Iterate(val);
    while(HasNext(iter)){
      Value subVal = GetValue(iter);

      *PushListElement(list) = GetSimplestForm(subVal,out);

      Advance(&iter);
    }

    toReturn.type = ReprInfoType_COLLECTION;
    toReturn.moreRepr = PushArrayFromList(out,list);
  } else if(IsStruct(val.type)){
    Array<String> names = GetStructMembersName(type,out);
    Array<ReprInfo> arr = PushArray<ReprInfo>(out,type->members.size);

    int index = 0;
    for(Member& m : type->members){
      Opt<Value> subVal = AccessStruct(val,&m);
      Assert(subVal.has_value());

      arr[index++] = GetSimplestForm(subVal.value(),out);
    }

    toReturn.type = ReprInfoType_COLLECTION;
    toReturn.moreRepr = arr;
    toReturn.names = names;
  } else {
    NOT_IMPLEMENTED("");
  }

  Assert(toReturn.type != ReprInfoType_REF);
  
  return toReturn;
}

struct PrintedEntity{
  void* address;
  Type* type;
};

template<> class std::hash<PrintedEntity>{
public:
   std::size_t operator()(PrintedEntity const& e) const noexcept{
      std::size_t res = std::hash<void*>()(e.address);
      res += std::hash<void*>()(e.type);
      return (std::size_t) res;
   }
};

bool operator==(const PrintedEntity& e0,const PrintedEntity& e1){
  bool res = (e0.address == e1.address && e0.type == e1.type);
  return res;
}

static FILE* globalTest = nullptr;
void Print(ReprInfo info,TrieSet<PrintedEntity>* seen,Arena* temp,ArenaList<ReprInfo>* notPrinted){
  BLOCK_REGION(temp);

  Value val = info.originalValue;
  void* valueAddress = GetValueAddress(val);

  PrintedEntity printed = {};
  printed.address = valueAddress;
  printed.type = val.type;
  
  if(seen->ExistsOrInsert(printed)){
    return;
  }
  
  switch(info.type){
  case ReprInfoType_TABLE_COLLECTION:{
    for(Array<ReprInfo> arr : info.table){
      for(ReprInfo sub : arr){
        if(sub.type == ReprInfoType_REF){
          *PushListElement(notPrinted) = sub;
        }
      }
    }      
  } break;
  case ReprInfoType_COLLECTION:{
    for(ReprInfo sub : info.moreRepr){
      if(sub.type == ReprInfoType_REF){
        *PushListElement(notPrinted) = sub;
      }
    }
  } break;
  default:{};
  }
  
  switch(info.type){
  case ReprInfoType_STRING: {
    fprintf(globalTest,"0 %p %.*s",valueAddress,UNPACK_SS(info.repr));
  } break;
  case ReprInfoType_TABLE_COLLECTION:{
    Assert(info.names.size);
    fprintf(globalTest,"\n1[%.*s] %p:\n",UNPACK_SS(info.originalValue.type->name),valueAddress);
      
    int outerIndex = 0;
    Array<Array<String>> allData = PushArray<Array<String>>(temp,info.table.size);
    
    for(Array<ReprInfo> subData : info.table){
      Array<String> result = PushArray<String>(temp,subData.size);
      int index = 0;
      for(ReprInfo in : subData){
        result[index++] = in.repr;
      }
        
      allData[outerIndex++] = result;
    }

    Array<String> fields = info.names;
    PrintAll(globalTest,fields,allData,temp);
  } break;
  case ReprInfoType_COLLECTION: {
    if(info.names.size){
      fprintf(globalTest,"2 [%.*s] %p:\n",UNPACK_SS(info.originalValue.type->name),valueAddress);
      int maxFieldSize = 0;
      int maxValueSize = 0;
    
      int index = 0;
      for(ReprInfo subInfo : info.moreRepr){
        String field = info.names[index++];
        maxFieldSize = std::max(maxFieldSize,field.size);
        maxValueSize = std::max(maxValueSize,subInfo.repr.size);
      }

      index = 0;
      for(ReprInfo subInfo : info.moreRepr){
        String name = info.names[index++];
        
        fprintf(globalTest,"%-*.*s: ",maxFieldSize,UNPACK_SS(name));
        fprintf(globalTest,"%*.*s",maxValueSize,UNPACK_SS(subInfo.repr));
        fprintf(globalTest,"\n");
      }
    } else {
      //Assert(valueAddress != nullptr);
      fprintf(globalTest,"3 [%.*s] %p: ",UNPACK_SS(val.type->name),valueAddress);

      if(info.moreRepr.size > 0){
        fprintf(globalTest,"(");
        bool first = true;
        for(ReprInfo subInfo : info.moreRepr){
          if(!first){
            fprintf(globalTest,",");
          }
          fprintf(globalTest,"%.*s",UNPACK_SS(subInfo.repr));
          first = false;
        }
        fprintf(globalTest,")");
      }
    }
  } break;
  case ReprInfoType_REF:{
    Assert(false);
    //fprintf(globalTest,"4 %p",valueAddress);
  } break;
  }
  fprintf(globalTest,"\n");
}

void PrintRepr(FILE* file,Value val,Arena* temp,Arena* temp2,int depth){
  BLOCK_REGION(temp);
  BLOCK_REGION(temp2);
  BLOCK_REGION(debugArena);

  globalTest = file;
  
  auto mark = MarkArena(temp);

  Array<ReprInfo> toPrint = PushArray<ReprInfo>(temp,1);
  toPrint[0] = GetReprInfo(val,temp,temp2);

  TrieSet<PrintedEntity>* alreadyPrinted = PushTrieSet<PrintedEntity>(debugArena);
  
  int iterations = 0;
  while(toPrint.size > 0){
    BLOCK_REGION(temp2);

    ArenaList<ReprInfo>* notPrinted = PushArenaList<ReprInfo>(temp2);
    for(ReprInfo info : toPrint){
      Print(info,alreadyPrinted,temp,notPrinted);
    }
    
    if(iterations > depth){
      break;
    }

    Array<ReprInfo> nextPrint = PushArrayFromList<ReprInfo>(temp2,notPrinted);

    PopMark(mark);
    toPrint = PushArray<ReprInfo>(temp,nextPrint.size);
    int index = 0;

    for(ReprInfo sub : nextPrint){
      Value actualValue = CollapsePtrIntoStruct(sub.originalValue);

      if(!(IsStruct(actualValue.type) || IsContainer(actualValue.type)))
        continue;
      
      toPrint[index++] = GetReprInfo(actualValue,temp,temp2);
    }
    toPrint.size = index;

    iterations += 1;
  }
}

#endif
