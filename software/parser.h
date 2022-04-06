#ifndef INCLUDED_PARSER
#define INCLUDED_PARSER

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

typedef struct Tokenizer_t{
   char* content;
   int size;
   int index;
   int lines;
} Tokenizer;

typedef struct Token_t{
   char str[256];
   int size;
} Token;

struct TypeInfo_t;

typedef struct Type_t{
   struct TypeInfo_t* info;
   int ptr;
} Type;

typedef struct Member_t{
   Type type;
   char name[512];
   int extraArrayElements; // 0 for single elements
} Member;

typedef struct TypeInfo_t {
   union{
      struct {
         Member* members;
         int nMembers;
      };
      Type alias;
   };
   int size;
   enum {TYPE_INFO_UNKNOWN,
         TYPE_INFO_SIMPLES,
         TYPE_INFO_ALIAS,
         TYPE_INFO_STRUCT,
         TYPE_INFO_STRUCT_DECL,
         TYPE_INFO_UNION,
         TYPE_INFO_UNION_DECL,
         TYPE_INFO_COUNT} type;
   char name[512];
} TypeInfo;

typedef struct TypeInfoReturn_t {
   TypeInfo* info;
   int nInfo;
} TypeInfoReturn;

#endif // INCLUDED_PARSER
