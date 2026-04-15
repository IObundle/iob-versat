#pragma once

#include "utils.hpp"

void HIER_Init();

struct HIER_Node{
  String name;
  u32 hash;

  HIER_Node* child;
  HIER_Node* nextChain;
};

struct HIER_Name{
  HIER_Node* node;

  HIER_Name() = default;
  HIER_Name(HIER_Node* node):node(node){};
  HIER_Name(String str);
};

void HIER_Init();

bool HIER_IsValid(HIER_Name name);

String HIER_GetFullName(HIER_Name name,String separator,Arena* out);
HIER_Name HIER_GetChild(HIER_Name base);

HIER_Name operator+(HIER_Name parent,HIER_Name bottom);
