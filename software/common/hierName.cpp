#include "hierName.hpp"
#include "utilsCore.hpp"

static struct {
  Arena* arena;
  
  HIER_Node* hashTable[512];
} HIER_State;

static HIER_Name GetOrAllocate(HIER_Name child,String name){
  if(Empty(name)){
    return child;
  }

  u32 childHash = 0;
  if(child.node){
    childHash = child.node->hash;
  }

  u32 hash = Hash(name) + childHash;
  u32 index = hash % 512;

  HIER_Node* before = nullptr;
  HIER_Node* ptr = HIER_State.hashTable[index];
  for(; ptr != nullptr; ptr = ptr->nextChain){
    if(ptr->name == name && ptr->child == child.node){
      return ptr;
    }
    before = ptr;
  }

  HIER_Node* newNode = PushStruct<HIER_Node>(HIER_State.arena);
  newNode->name = PushString(HIER_State.arena,name);
  newNode->child = child.node;
  
  if(before){
    before->nextChain = newNode;
  } else {
    HIER_State.hashTable[index] = newNode;
  }
  
  return newNode;
}

static Array<HIER_Name> ParentToChildNodes(HIER_Name start,Arena* out){
  TEMP_REGION(temp,out);

  auto list = PushList<HIER_Name>(temp);
  for(HIER_Name ptr = start; HIER_IsValid(ptr); ptr = HIER_GetChild(ptr)){
    *list->PushElem() = ptr;
  }
  Array<HIER_Name> asArray = PushArray(out,list);
  
  return asArray;
}

static Array<HIER_Name> ChildToParentNodes(HIER_Name start,Arena* out){
  Array<HIER_Name> array = ParentToChildNodes(start,out);
  ReverseInPlace(array);
  
  return array;
}

HIER_Name::HIER_Name(String str){
  HIER_Name res = GetOrAllocate({},str);
  *this = res;
}

void HIER_Init(){
  static Arena arenaInst = InitArena(Megabyte(4));
  HIER_State.arena = &arenaInst;
}

bool HIER_IsValid(HIER_Name name){
  bool res = (name.node != nullptr);
  return res;
}

HIER_Name HIER_GetChild(HIER_Name base){
  if(base.node){
    HIER_Name parent = {base.node->child};
    return parent;
  }

  return {};
}

String HIER_GetFullName(HIER_Name name,String separator,Arena* out){
  TEMP_REGION(temp,nullptr);

  Array<HIER_Name> nodes = ParentToChildNodes(name,temp);
  int size = nodes.size;
  Array<String> asString = PushArray<String>(temp,nodes.size);
  for(int i = 0; i < size; i++){
    asString[i] = nodes[i].node->name;
  }
  
  String res = JoinStrings(asString,separator,out);
  return res;
}

HIER_Name operator+(HIER_Name parent,HIER_Name bottom){
  TEMP_REGION(temp,nullptr);

  Array<HIER_Name> nodes = ChildToParentNodes(parent,temp);
  
  HIER_Name ptr = bottom;
  for(HIER_Name name : nodes){
    ptr = GetOrAllocate(ptr,name.node->name);
  }

  return ptr;
}
