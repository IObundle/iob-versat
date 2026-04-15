#include "templateEngine.hpp"

struct Frame{
  ArenaMark mark;
  TrieMap<String,Value>* table;
  Frame* previousFrame;
};

static Opt<Value> GetValue(Frame* frame,String var){
  Frame* ptr = frame;

  while(ptr){
    Value* possible = ptr->table->Get(var);
    if(possible){
      return *possible;
    } else {
      ptr = ptr->previousFrame;
    }
  }
  return {};
}

static Value* ValueExists(Frame* frame,String id){
  Frame* ptr = frame;

  while(ptr){
    Value* possible = ptr->table->Get(id);
    if(possible){
      return possible;
    }
    ptr = ptr->previousFrame;
  }
  return nullptr;
}

static void SetValue(Frame* frame,String id,Value val){
  Value* possible = ValueExists(frame,id);
  if(possible){
    *possible = val;
  } else {
    frame->table->Insert(id,val);
  }
}

static Frame* CreateFrame(Frame* previous,Arena* out){
  ArenaMark mark = MarkArena(out);

  Frame* frame = PushStruct<Frame>(out);
  frame->table = PushTrieMap<String,Value>(out); // Testing a fixed hashmap for now.
  frame->mark = mark;
  frame->previousFrame = previous;
  return frame;
}

static Frame* globalFrame;
static Frame* currentFrame;
static Arena TE_ArenaInst;
static Arena* TE_Arena = &TE_ArenaInst;

void TE_Init(){
  TE_ArenaInst = InitArena(Megabyte(1));
  
  globalFrame = CreateFrame(nullptr,TE_Arena);
  currentFrame = globalFrame;
}

void TE_ProcessTemplate(StringBuilder* b,String tmpl){
  TEMP_REGION(temp,nullptr);

  int size = tmpl.size;
  for(int i = 0; i < size; i++){
    if(i + 2 < size && tmpl[i] == '@' && tmpl[i+1] == '{'){
      i = i+2;

      int start = i;
      for(; i < size; i++){
        if(tmpl[i] == '}'){
          break;
        }
      }
      String subName = String{&tmpl[start],i - start};

      Opt<Value> optVal = GetValue(currentFrame,subName);

      if(!optVal.has_value()){
        printf("[Error] Template did not find the member: '%.*s'\n",UN(subName));
        Assert(false);
      }

      Value val = optVal.value();

      if(val.type == ValueType_STRING){
        b->PushString(val.str);
      } else if(val.type == ValueType_NUMBER) {
        b->PushString("%ld",val.number);
      } else if(val.type == ValueType_BOOLEAN) {
        if(val.boolean){
          b->PushString("true");
        } else {
          b->PushString("false");
        }
      } else {
        Assert(false);
      }
    } else {
      b->PushChar(tmpl[i]);
    }
  }

  TE_Clear();
}

String TE_ProcessTemplate(Arena* out,String tmpl){
  TEMP_REGION(temp,out);
  auto b = StartString(temp);
  TE_ProcessTemplate(b,tmpl);

  String content = EndString(out,b);
  
  return content;
}

void TE_ProcessTemplate(FILE* outputFile,String tmpl){
  TEMP_REGION(temp,nullptr);
  
  String content = TE_ProcessTemplate(temp,tmpl);
  
  fprintf(outputFile,"%.*s",UN(content));
  fflush(outputFile);
}

void TE_SimpleSubstitute(StringBuilder* b,String tmpl,Hashmap<String,String>* subs){
  int size = tmpl.size;
  for(int i = 0; i < size; i++){
    if(i + 2 < size && tmpl[i] == '@' && tmpl[i+1] == '{'){
      i = i+2;

      int start = i;
      for(; i < size; i++){
        if(tmpl[i] == '}'){
          break;
        }
      }
      String subName = String{&tmpl[start],i - start};

      String toSubstituteWith = subs->GetOrFail(subName);
      b->PushString(toSubstituteWith);
    } else {
      b->PushChar(tmpl[i]);
    }
  }
}

String TE_Substitute(String tmpl,String* valuesToReplace,Arena* out){
  TEMP_REGION(temp,out);
  auto b = StartString(temp);
  
  int size = tmpl.size;
  for(int i = 0; i < size; i++){
    if(i + 2 < size && tmpl[i] == '@' && tmpl[i+1] == '{'){
      i = i+2;

      int start = i;
      for(; i < size; i++){
        if(tmpl[i] == '}'){
          break;
        }
      }
      String subIndex = String{&tmpl[start],i - start};
      int index = ParseInt(subIndex);
      
      b->PushString(valuesToReplace[index]);
    } else {
      b->PushChar(tmpl[i]);
    }
  }

  return EndString(out,b);
}

void TE_PushScope(){
  currentFrame = CreateFrame(currentFrame,TE_Arena);
}

void TE_PopScope(){
  Frame* previousFrame = currentFrame->previousFrame;
  
  if(!previousFrame){
    TE_Clear();
    return;
  }
  
  PopMark(currentFrame->mark);
  currentFrame = previousFrame;
}

void TE_Clear(){
  currentFrame->table->Clear();
}

Value MakeValue(){
  Value val = {};
  val.type = ValueType_NIL;
  return val;
};

Value MakeValue(int number){
  Value val = {};
  val.type = ValueType_NUMBER;
  val.number = number;
  return val;
};

Value MakeValue(i64 number){
  Value val = {};
  val.type = ValueType_NUMBER;
  val.number = number;
  return val;
}

Value MakeValue(String str){
  Value val = {};
  val.type = ValueType_STRING;
  val.str = str;
  return val;
}

Value MakeValue(bool b){
  Value val = {};
  val.type = ValueType_BOOLEAN;
  val.boolean = b;
  return val;
}

void TE_SetNumber(String id,int number){
  SetValue(currentFrame,id,MakeValue(number));
}

void TE_SetString(String id,String str){
  Value val = {};
  val.type = ValueType_STRING;
  val.str = TrimWhitespaces(str);

  SetValue(currentFrame,id,val);
}

void TE_SetHex(String id,int number){
  // TODO: Need to indicate that this is a hexadecimal number
  Value val = {};
  val.type = ValueType_NUMBER;
  val.number = number;
  
  SetValue(currentFrame,id,val);
}

void TE_SetBool(String id,bool boolean){
  Value val = {};
  val.type = ValueType_BOOLEAN;
  val.boolean = boolean;
  
  SetValue(currentFrame,id,val);
}
