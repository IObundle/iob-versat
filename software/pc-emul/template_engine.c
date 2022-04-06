#if MARK

#include "template_engine.h"

#include "stdlib.h"
#include "stdio.h"

#include "utils.h"

#define MAX_CUSTOM_TYPES 256

static Type customTypes[MAX_CUSTOM_TYPES];
static int customTypesIndex;

int AddType(char* name,Type type){
   int res = VAL_CUSTOM + customTypesIndex;

   Assert(customTypesIndex < MAX_CUSTOM_TYPES);

   customTypes[customTypesIndex++] = type;

   return res;
}

static char content[1024 * 1024];

static char buffer[1024 * 1024];
static int charBufferSize;

static void CheckIdentifierNotFound(Value* val,char* id){
   if(val == NULL){
      printf("Identifier not found: %s\n",id);
      Assert(0);
   }
}

static char* PushString(char* str,int size){
   char* res = &buffer[charBufferSize];

   for(int i = 0; i < size & *str != '\0'; i++){
      buffer[charBufferSize++] = *(str++);
   }

   buffer[charBufferSize++] = '\0';

   return res;
}

static int IsWhitespace(char ch){
   if(ch == '\n' || ch == ' ' || ch == '\t'){
      return 1;
   }

   return 0;
}

static void ConsumeWhitespace(Tokenizer* tok){
   while(1){
      if(tok->index >= tok->size)
         return;

      char ch = tok->content[tok->index];

      if(ch == '\n'){
         tok->index += 1;
         tok->lines += 1;
         continue;
      }

      if(IsWhitespace(ch)){
         tok->index += 1;
         continue;
      }

      return;
   }
   return;
}

static int SingleChar(char ch){
   char options[] = "[]+()";

   for(int i = 0; options[i] != '\0'; i++){
      if(options[i] == ch){
         return 1;
      }
   }

   return 0;
}

static Token PeekToken(Tokenizer* tok){
   Token token = {};

   ConsumeWhitespace(tok);

   if(tok->index >= tok->size){
      return token;
   }

   char ch = tok->content[tok->index];

   token.str[0] = ch;
   token.size = 1;

   if(SingleChar(ch)){
      return token;
   }

   if(ch == '\"'){ // Special case for strings
      for(int i = 1; tok->index + i < tok->size; i++){
         ch = tok->content[tok->index + i];

         token.str[i] = ch;
         token.size = (i + 1);

         if(ch == '\"'){
            token.str[token.size] = '\0';
            return token;
         }
      }
   } else {
      for(int i = 1; tok->index + i < tok->size; i++){
         ch = tok->content[tok->index + i];

         if(SingleChar(ch) || IsWhitespace(ch)){
            token.str[token.size] = '\0';
            return token;
         }

         token.str[i] = ch;
         token.size = (i + 1);
      }
   }

   token.str[token.size] = '\0';

   return token;
}

static Token NextToken(Tokenizer* tok){
   Token res = PeekToken(tok);

   tok->index += res.size;

   #if 0
   printf("%s\n",res.str);
   #endif

   return res;
};

#define SYMBOL_TABLE_SIZE 4096
static SymbolEntry symbolHashTable[SYMBOL_TABLE_SIZE];

static int StringCmp(const char* str1,const char* str2){
   if(str1 == NULL || str2 == NULL){
      if(str1 == NULL && str2 == NULL){
         return 1;
      } else {
         return 0;
      }
   }

   while(*str1 != '\0' && *str2 != '\0'){
      if(*str1 != *str2){
         return 0;
      }

      str1++;
      str2++;
   }

   return (*str1 == *str2);
}

static int StringHash(const char* str){
   int num = 0;

   while(*str++ != '\0'){
      num += (num << 6) + (int) *str;
   }

   return num;
}

static Value* AddEntry(const char* symbol,Value val){
   int hash = StringHash(symbol);

   for(int i = 0; i < SYMBOL_TABLE_SIZE; i++){
      SymbolEntry* entry = &symbolHashTable[(hash + i) % SYMBOL_TABLE_SIZE];

      if(entry->symbol == NULL){
         entry->symbol = symbol;
         entry->val = val;
         return &entry->val;
      }
   }

   printf("Error 4, run out of space for hash table\n");
   exit(0);
}

static Value* GetEntry(const char* symbol){
   int hash = StringHash(symbol);

   for(int i = 0; i < SYMBOL_TABLE_SIZE; i++){
      SymbolEntry* entry = &symbolHashTable[(hash + i) % SYMBOL_TABLE_SIZE];

      if(StringCmp(entry->symbol,symbol)){
         return &entry->val;
      }
   }

   return NULL;
}

static Value* SetEntry(const char* symbol,Value val){
   Value* entry = GetEntry(symbol);

   if(entry == NULL){
      entry = AddEntry(symbol,val);
   }

   *entry = val;

   return entry;
}

static int CompareToken(Token tok,const char* str){
   for(int i = 0; i < tok.size; i++){
      if(tok.str[i] != str[i]){
         return 0;
      }
   }

   return (str[tok.size] == '\0');
}

static Expression expressions[1024];
static int expressionSize;

static Expression* PushExpression(Expression expr){
   Expression* res = &expressions[expressionSize];

   expressions[expressionSize++] = expr;

   return res;
}

static Value Eval(Expression* expr);

static char templateBuffer[1024*1024];
static int bufferSize;
static int numberBuffered;

static void ResetTemplateBuffer(){
   numberBuffered = 0;
   bufferSize = 0;
   templateBuffer[0] = '\0';
}

static void Format(const char* format){
   char entryBuffer[64];

   for(int i = 0; format[i] != '\0'; i++){
      if(format[i] == '$'){
         i += 2; // Because of (, add 2
         int j = 0;
         while(format[i] != ')'){
            entryBuffer[j++] = format[i++];
         }

         entryBuffer[j] = '\0';
         Value* val = GetEntry(entryBuffer);

         CheckIdentifierNotFound(val,entryBuffer);

         if(val->type == VAL_NUMBER){
            bufferSize += sprintf(&templateBuffer[bufferSize],"%d",val->number);
         } else {
            Assert(0);
         }
      } else {
         templateBuffer[bufferSize++] = format[i];
      }
   }
   templateBuffer[bufferSize++] = '\0';
   templateBuffer[bufferSize] = '\0';

   numberBuffered += 1;
}

#define AssertIdentifier(EXPR) Assert(EXPR->type == TYPE_IDENTIFIER)
#define AssertNumber(VAL)  Assert(VAL.type == VAL_NUMBER)
#define AssertStruct(VAL)  Assert(VAL.type == VAL_STRUCT)
#define AssertString(VAL)  Assert(VAL.type == VAL_STRING)

static FILE* output;

void PrintValue(Value val){
   switch(val.type){
   case VAL_NUMBER:{
      printf("%d\n",val.number);
   }break;
   case VAL_STRING:{
      printf("\"%s\"\n",val.str);
   }break;
   case VAL_ARRAY:{
      printf("<%d>\n",(int) val.array);
   }break;
   }
}

static void Print(Expression* expr,int level){
   for(int i = 0; i < level; i++){
      printf("  ");
   }
   switch(expr->type){
      case TYPE_EXPRESSION:{
         printf("Expr: %s\n",expr->id);
         for(int i = 0; i < expr->size; i++){
            Print(expr->expressions[i],level + 1);
         }
      } break;
      case TYPE_VAL:{
         printf("Val: ");
         PrintValue(expr->val);
      } break;
      case TYPE_ARRAY:{
         printf("Array: %s\n",expr->id);
         printf("[\n");
         Print(expr->expressions[0],level + 1);
         printf("]\n");
      } break;
      case TYPE_IDENTIFIER:{
         printf("Id: %s\n",expr->id);
      } break;
   }
}

static Expression* Parse(Tokenizer* tok){
   Token token = NextToken(tok);
   Expression* expr = PushExpression((Expression){});

   expr->type = TYPE_EXPRESSION;
   // Specific tokens give info about how to parse the rest
   if(CompareToken(token,"print_buffer")){
      expr->size = 0;
      expr->id = PushString(token.str,token.size);
      return expr;
   }
   if(CompareToken(token,"list_buffer")){
      expr->size = 1;
   }
   if(CompareToken(token,"for")){
      expr->size = 4;
   }
   if(CompareToken(token,"format_buffer")){
      expr->size = 1;
   }
   if(CompareToken(token,"if")){
      expr->size = 2;
   }
   if(CompareToken(token,"+")){
      expr->size = 2;
   }
   if(CompareToken(token,"-")){
      expr->size = 2;
   }
   if(CompareToken(token,"set")){
      expr->size = 2;
   }
   if(CompareToken(token,"debug")){
      expr->size = 1;
   }
   if(CompareToken(token,"(")){
      expr->id = "(";

      for(int i = 0; PeekToken(tok).str[0] != ')'; i++){
         expr->expressions[expr->size++] = Parse(tok);
         Assert(i < MAX_EXPRESSION_SIZE);
      }
      NextToken(tok);

      return expr;
   }
   for(int i = 0; i < expr->size; i++){
      expr->expressions[i] = Parse(tok);
   }
   if(expr->size != 0){
      expr->id = PushString(token.str,token.size);
      return expr;
   }

   expr->type = TYPE_VAL;
   if(token.str[0] >= '0' && token.str[0] <= '9'){
      int num = 0;

      for(int i = 0; i < token.size; i++){
         num *= 10;
         num += token.str[i] - '0';
      }

      expr->val.type = VAL_NUMBER;
      expr->val.number = num;
   } else if(token.str[0] == '\"'){
      expr->val.str = PushString(&token.str[1],token.size - 2);
      expr->val.type = VAL_STRING;
   } else {
      expr->type = TYPE_IDENTIFIER;
      expr->id = PushString(token.str,token.size);

      Token peek = PeekToken(tok);

      if(peek.str[0] == '['){
         expr->type = TYPE_ARRAY;
         NextToken(tok);
         expr->expressions[0] = Parse(tok);
         expr->size = 1;
         Assert(NextToken(tok).str[0] == ']');
      }
   }

   return expr;
};

static void PrintEnvironment(){
   for(int i = 0; i < SYMBOL_TABLE_SIZE; i++){
      SymbolEntry entry = symbolHashTable[i];

      if(entry.symbol != NULL){
         printf("%s:",entry.symbol);
         PrintValue(entry.val);
      }
   }
}

static Value ExpressionEval(Expression* expr){
   Value val = {};

   val.type = VAL_NUMBER; // By default val is set to number

   if(StringCmp(expr->id,"print_buffer")){
      char* ptr = templateBuffer;
      for(int i = 0; i < numberBuffered; i++){
         fprintf(output,"%s",ptr);
         while(*ptr != '\0'){
            ptr += 1;
         }
         ptr += 1;
      }

      ResetTemplateBuffer();
      val.type = VAL_NULL;
   }
   if(StringCmp(expr->id,"list_buffer")){
      Value sep = Eval(expr->expressions[0]);

      char* ptr = templateBuffer;
      for(int i = 0; i < numberBuffered; i++){
         fprintf(output,"%s",ptr);
         while(*ptr != '\0'){
            ptr += 1;
         }
         ptr += 1;

         if(i != numberBuffered - 1){
            fprintf(output,"%s",sep.str);
         }
      }

      ResetTemplateBuffer();
      val.type = VAL_NULL;
   }
   if(StringCmp(expr->id,"for")){
      AssertIdentifier(expr->expressions[0]);

      Value initial = Eval(expr->expressions[1]);
      Value end = Eval(expr->expressions[2]);

      SetEntry(expr->expressions[0]->id,initial);

      for(int index = initial.number; index < end.number;){
         val = Eval(expr->expressions[3]);

         index += 1;
         GetEntry(expr->expressions[0]->id)->number = index;
      }
   }
   if(StringCmp(expr->id,"format_buffer")){
      Format(Eval(expr->expressions[0]).str);
   }
   if(StringCmp(expr->id,"if")){
      Value decision = Eval(expr->expressions[0]);

      if(decision.number){
         val = Eval(expr->expressions[1]);
      }
   }
   if(StringCmp(expr->id,"+")){
      Value v1 = Eval(expr->expressions[0]);
      Value v2 = Eval(expr->expressions[1]);

      val.number = v1.number + v2.number;
   }
   if(StringCmp(expr->id,"-")){
      Value v1 = Eval(expr->expressions[0]);
      Value v2 = Eval(expr->expressions[1]);

      val.number = v1.number - v2.number;
   }
   if(StringCmp(expr->id,"set")){
      Value val = Eval(expr->expressions[1]);

      SetEntry(expr->expressions[0]->id,val);
   }
   if(StringCmp(expr->id,"(")){
      for(int i = 0; i < expr->size; i++){
         val = Eval(expr->expressions[i]);
      }
   }
   if(StringCmp(expr->id,"debug")){
      Value val = Eval(expr->expressions[0]);

      if(val.number){
         PrintEnvironment();
      }
   }

   return val;
}

static Value Eval(Expression* expr){
   switch(expr->type){
      case TYPE_EXPRESSION:{
         return ExpressionEval(expr);
      };
      case TYPE_VAL:{
         return expr->val;
      } break;
      case TYPE_IDENTIFIER:{
         Value* entry = GetEntry(expr->id);

         CheckIdentifierNotFound(entry,expr->id);

         return *entry;
      } break;
      case TYPE_ARRAY:{
         Value* entry = GetEntry(expr->id);
         CheckIdentifierNotFound(entry,expr->id);

         Value index = Eval(expr->expressions[0]);

         if(index.number >= entry->size){
            printf("Error 7, index bigger than array size: %d/%d\n",index.number,entry->size);
            exit(0);
         }

         Value res = {};

         res.type = VAL_NUMBER;
         res.number = entry->array[index.number];

         return res;
      } break;
   }

   return (Value){.type = VAL_NULL};
}

typedef struct TypeTest_t{
   int a,b,c;
} TypeTest;

typedef struct TypeTest2_t{
   TypeTest a,b;
} TypeTest2;

TypeInfoReturn ParseMain(FILE* file);

void ProcessTemplate(char* templateFilepath,char* outputFilepath){
   Tokenizer tokenizer = {};

   FILE* file = fopen(templateFilepath,"r");

   if(!file){
      printf("Error opening template: %s\n",templateFilepath);
      return;
   }

   #if 0
   FILE* test = fopen("/home/zettasticks/IOBundle/iob-soc-sha/submodules/VERSAT/software/versat.h","r");

   if(!test){
      printf("error\n");
      exit(0);
   }

   ParseMain(test);
   return;
   #endif

   int size = fread(content,sizeof(char),1024*1024,file);
   fclose(file);
   content[size] = '\0';

   tokenizer.content = content;
   tokenizer.lines = 1;
   tokenizer.size = size;

   output = fopen(outputFilepath,"w");

   #if 0
      output = stdout;
   #endif

   int startIndex = 0;
   while(1){
      tokenizer.index = startIndex;
      int endIndex;
      for(endIndex = startIndex; endIndex < size && !(content[endIndex] == '$' && content[endIndex+1] == '{'); endIndex += 1){
         if(content[endIndex] == '\n')
            tokenizer.lines += 1;
      }

      if(content[endIndex] != '$' && endIndex == size){
         fwrite(&content[tokenizer.index],sizeof(char),endIndex - tokenizer.index,output);
         break;
      }

      fwrite(&content[tokenizer.index],sizeof(char),endIndex - tokenizer.index,output);
      int parsingBlockStartIndex = endIndex + 2;

      int parsingBlockEndIndex = parsingBlockStartIndex;
      for(; parsingBlockEndIndex < size && !(content[parsingBlockEndIndex] == '$' && content[parsingBlockEndIndex + 1] == '}'); parsingBlockEndIndex += 1){
         if(content[parsingBlockEndIndex] == '\n')
            tokenizer.lines += 1;
      }

      startIndex = parsingBlockEndIndex + 2;

      tokenizer.index = parsingBlockStartIndex;
      tokenizer.size = parsingBlockEndIndex;

      Value val = {};
      while(PeekToken(&tokenizer).size != 0){
         Expression* expr = Parse(&tokenizer);

         #if 0
         Print(expr,0);
         #endif

         val = Eval(expr);
      }

      if(val.type != VAL_NULL){
         ResetTemplateBuffer();

         SetEntry("res",val);
         Format("$(res)");
         fprintf(output,"%s",templateBuffer);
         ResetTemplateBuffer();
      }
   }

   printf("\n");
}

void TemplateSetNumber(char* id,int number){
   Value val = {};

   val.type = VAL_NUMBER;
   val.number = number;

   AddEntry(id,val);
}

void TemplateSetString(char* id,char* str){
   Value val = {};

   val.type = VAL_STRING;
   val.str = str;

   AddEntry(id,val);
}

void TemplateSetArray(char* id,int* array,int size){
   Value val = {};

   val.type = VAL_ARRAY;
   val.array = array;
   val.size = size;

   AddEntry(id,val);
}

#endif
