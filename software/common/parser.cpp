#include "parser.hpp"

#include <cstring>
#include <cctype>

#include "utils.hpp"

void Tokenizer::ConsumeWhitespace(){
  if(keepWhitespaces){
    return;
  }

  while(1){
    if(ptr >= end)
      return;

    if(ptr[0] == '\n'){
      ptr += 1;
      lines += 1;
      continue;
    }

    if(std::isspace(ptr[0])){
      ptr += 1;
      continue;
    }

    if(!keepComments){
      if(ptr[0] == '/' && ptr[1] == '/'){
        while(ptr < end && *ptr != '\n') ptr += 1;
        ptr += 1;
        continue;
      }

      if(ptr[0] == '/' && ptr[1] == '*'){
        while(ptr + 1 < end &&
              !(ptr[0] == '*' && ptr[1] == '/')) ptr += 1;
        ptr += 2;
        continue;
      }
    }

    return;
  }
}

Tokenizer::Tokenizer(String content,const char* singleChars,std::vector<std::string> specialCharsList)
:start(content.data)
,ptr(content.data)
,end(content.data + content.size)
,lines(1)
,singleChars(singleChars)
,specialChars(specialCharsList)
{
  keepWhitespaces = false;
  keepComments = false;
}

int Tokenizer::SpecialChars(const char* ptr, size_t size){
  for(std::string special : specialChars){
    if(special.size() >= size){
      continue;
    }

    String specialStr = {special.c_str(),(int) special.size()};
    String ptrStr = {ptr,specialStr.size};

    if(CompareString(ptrStr,specialStr)){
      return specialStr.size;
    }
  }

  for(size_t i = 0; i < singleChars.size(); i++){
    if(singleChars[i] == ptr[0]){
      return 1;
    }
  }

  return 0;
}

Token Tokenizer::PeekToken(){
  Token token = {};

  if(ptr >= end){
    return token;
  }

  ConsumeWhitespace();

  // After this, only need to set size
  token.data = ptr;

  // Handle special tokens (custom)
  int size = SpecialChars(ptr,end - ptr);

  if(size != 0){
    token.size = size;

    return token;
  }

  for(int i = 1; &ptr[i] <= end; i++){
    token.size = i;
    if((SpecialChars(&ptr[i],end - &ptr[i])) || std::isspace(ptr[i])){
      return token;
    }
  }

  return token;
}

Token Tokenizer::NextToken(){
  Token res = PeekToken();
  AdvancePeek(res);
  return res;
};

String Tokenizer::GetStartOfCurrentLine(){
  // Get start of the line
  const char* lineStart = ptr;
  while(lineStart > start){
    lineStart -= 1;
    if(lineStart[0] == '\n'){
      lineStart += 1;
      break;
    }
  }

  // Get end of the line
  const char* lineEnd = ptr;
  while(lineEnd < end){
    lineEnd += 1;
    if(lineEnd[0] == '\n'){
      break;
    }
  }

  String fullLine = STRING(lineStart,lineEnd - lineStart);

  return fullLine;
}

void PrintEscapedToken(Token tok){
  // Not very good in terms of performance but should suffice
  for(int i = 0; i < tok.size; i++){
    switch(tok[i]){
    case '\a': printf("\\a");
    case '\b': printf("\\b");
    case '\r': printf("\\r");
    case '\f': printf("\\f");
    case '\t': printf("\\t");
    case '\n': printf("\\n");
    case '\0': printf("\\0");
    case '\v': printf("\\v");
    default: printf("%c",tok[i]);
    }
  }
}

Token Tokenizer::AssertNextToken(const char* format){
  Token token = NextToken();

  if(!CompareToken(token,format)){
    String fullLine = GetStartOfCurrentLine();
    int lineStart = GetTokenPositionInside(fullLine,token);

    printf("Parser Error.\n Expected to find:");
    PrintEscapedToken(STRING(format));
    printf("\n");
    printf("  Got:");
    PrintEscapedToken(token);
    printf("\n");
    printf("%6d | %.*s\n",lines,UNPACK_SS(fullLine));
    printf("      "); // 6 spaces to match the 6 digits above
    printf(" | ");

    for(int i = 0; i < lineStart; i++){
      printf(" ");
    }
    printf("^\n");

    DEBUG_BREAK();
  }

  return token;
}

Token Tokenizer::PeekFindUntil(const char* str){
  Token token = {};

  String ss = STRING(str);

  int i = 0;
  while(&ptr[i] + ss.size <= end){
    if(ptr[i] == str[0]){
      String cmp = STRING(&ptr[i],ss.size);

      if(CompareString(ss,cmp)){
        token.size = i;
        token.data = ptr;

        return token;
      }
    }

    if(!keepComments){
      if(&ptr[i+1] + ss.size < end && ptr[i] == '/' && ptr[i+1] == '/'){
        while(ptr[i] != '\n'){
          i += 1;
        }
        continue;
      }

      if(&ptr[i+1] + ss.size < end && ptr[i] == '/' && ptr[i+1] == '*'){
        while(&ptr[i] + ss.size < end && (ptr[i-1] != '*' || ptr[i] != '/')){
          i += 1;
        }
        i += 1;
        continue;
      }
    }

    i += 1;
  }

  token.size = -1; // Didn't find anything. TODO: Not a good way of reporting errors

  return token;
}

Token Tokenizer::PeekFindIncluding(const char* str){
  Token tok = PeekFindUntil(str);

  if(tok.size == -1){
    return tok;
  }

  tok.size += strlen(str);

  return tok;
}

Token Tokenizer::PeekFindIncludingLast(const char* str){
  void* mark = Mark();

  while(!Done()){
    Token peek = PeekFindIncluding(str);

    if(peek.size <= 0){
      break;
    }

    AdvancePeek(peek);
  }

  String res = Point(mark);

  Rollback(mark);

  return res;
}

FindFirstResult Tokenizer::FindFirst(std::initializer_list<const char*> strings){
  Token peekFind = {};
  peekFind.size = -1;

  const char* res = nullptr;
  bool first = true;
  for(const char* str : strings){
    Token token = PeekFindUntil(str);

    if(first){
      if(token.size != -1){
        first = false;
        peekFind = token;
        res = str;
      }
    }

    if(token.size > 0 && token.size < peekFind.size){
      peekFind = token;
      res = str;
    }
  }

  FindFirstResult result = {};
  result.peekFindNotIncluded = peekFind;

  if(res){
    result.foundFirst = STRING(res);
  } else {
    result.foundFirst.size = -1;
  }

  return result;
}

Token Tokenizer::NextFindUntil(const char* str){
  Token token = PeekFindUntil(str);
  AdvancePeek(token);

  return token;
}

bool Tokenizer::IfPeekToken(const char* str){
  Token peek = PeekToken();

  if(CompareString(peek,str)){
    return true;
  }

  return false;
}

bool Tokenizer::IfNextToken(const char* str){
  Token peek = PeekToken();

  if(CompareString(peek,str)){
    AdvancePeek(peek);
    return true;
  }

  return false;
}

bool Tokenizer::IsSingleChar(char ch){
  for(char v : singleChars){
    if(v == ch){
      return true;
    }
  }
  return false;
}

bool Tokenizer::IsSingleChar(const char* chars){
  for(int i = 0; chars[i]; i++){
    if(!IsSingleChar(chars[i])){
      return false;
    }
  }
  return true;
}

Token Tokenizer::PeekWhitespace(){
  Token token = {};

  token.data = ptr;

  for(int i = 0; &ptr[i] <= end; i++){
    if(!std::isspace(ptr[i])){
      token.size = i;
      break;
    }
  }

  return token;
}

Token Tokenizer::Finish(){
  Token token = {};

  token.data = ptr;
  token.size = end - ptr;

  ptr = end;

  return token;
}

void* Tokenizer::Mark(){
  return (void*) ptr;
}

Token Tokenizer::Point(void* mark){
  Assert(ptr >= mark);

  Token token = {};
  token.data = ((char*)mark);
  token.size = ptr - ((char*)mark);

  return token;
}

void Tokenizer::Rollback(void* mark){
  ptr = (const char*) mark;
}

void Tokenizer::SetSingleChars(const char* singleChars){
  this->singleChars = std::string(singleChars);
}

void Tokenizer::AdvancePeek(Token tok){
  for(int i = 0; i < tok.size; i++){
    if(tok[i] == '\n'){
      lines += 1;
    }
  }
  ptr += tok.size;
}

bool Tokenizer::Done(){
  void* mark = Mark();

  ConsumeWhitespace();

  bool res = ptr >= end;

  Rollback(mark);

  return res;
}

Token Tokenizer::Remaining(){
  String res = {};

  res.data = ptr;
  res.size = end - ptr;

  return res;
}

bool Tokenizer::IsSpecialOrSingle(String toTest){
  if(toTest.size > 1){
    for(std::string& str : specialChars){
      if(CompareString(str.c_str(),toTest)){
        return true;
      }
    }
  } else {
    for(char ch : singleChars){
      if(ch == toTest[0]){
        return true;
      }
    }
  }

  return false;
}

String Tokenizer::PeekUntilDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen){
  for(const char* str : open){
    Assert(IsSpecialOrSingle(STRING(str)));
  }
  for(const char* str : close){
    Assert(IsSpecialOrSingle(STRING(str)));
  }

  void* pos = Mark();

  String res = {};
  res.data = (const char*) pos;

  int seen = numberOpenSeen;
  while(!Done()){
    Token tok = PeekToken();

    for(const char* str : open){
      if(CompareString(tok,str)){
        seen += 1;
        break;
      }
    }

    for(const char* str : close){
      if(CompareString(tok,str)){
        if(seen == 0){
          goto end;
        }

        seen -= 1;

        if(seen == 0){
          res = Point(pos);
          goto end;
        }
        break;
      }
    }

    AdvancePeek(tok);
  }

 end:
  Rollback(pos); // Act as peek

  return res;
}

String Tokenizer::PeekIncludingDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen){
  Token until = PeekUntilDelimiterExpression(open,close,numberOpenSeen);

  void* pos = Mark();

  AdvancePeek(until);
  NextToken();

  String res = Point(pos);
  Rollback(pos);

  return res;  
}

bool CheckStringOnlyWhitespace(Token tok){
  for(int i = 0; i < tok.size; i++){
    char ch = tok[i];
    if(!std::isspace(ch)){
      return false;
    }
  }
  return true;
}

bool CheckFormat(const char* format,Token tok){
  int tokenIndex = 0;
  for(int formatIndex = 0; 1;){
    char formatChar = format[formatIndex];

    if(formatChar == '\0'){
      if(tok.size == tokenIndex){
        return true;
      } else {
        return false;
      }
    }

    if(tokenIndex >= tok.size){
      return false;
    }

    if(formatChar == '%'){
      char type = format[formatIndex + 1];
      formatIndex += 2;

      switch(type){
      case 'd':{
        if(!IsNum(tok[tokenIndex])){
          return false;
        }

        for(tokenIndex += 1; tokenIndex < tok.size; tokenIndex++){
          if(!IsNum(tok[tokenIndex])){
            break;
          }
        }
      }break;
      case 's':{
        char terminator = format[formatIndex];

        for(;tokenIndex < tok.size; tokenIndex++){
          if(tok[tokenIndex] == terminator){
            break;
          }
        }
      }break;
      case '\0':{
        NOT_POSSIBLE;
      }break;
      default:{
        NOT_IMPLEMENTED;
      }break;
      }
    } else if(formatChar != tok[tokenIndex]){
      return false;
    } else {
      formatIndex += 1;
      tokenIndex += 1;
    }
  }

  return true;
}

bool Contains(String str,const char* toCheck){
  Tokenizer tok(str,"",{toCheck});

  while(!tok.Done()){
    Token token = tok.NextToken();

    if(CompareString(token,toCheck)){
      return true;
    }
  }

  return false;
}

String TrimWhitespaces(String in){
  const char* start = in.data;
  const char* end = &in.data[in.size-1];

  while(std::isspace(*start) && start < end) ++start;
  while(std::isspace(*end) && end > start) --end;

  String res = {};
  res.data = start;
  res.size = end - start + 1;

  return res;
}

String GeneratePointingString(int startPos,int size,Arena* arena){
  Byte* mark = MarkArena(arena);
  for(int i = 0; i < startPos; i++){
    PushString(arena," ");
  }
  for(int i = 0; i < size; i++){
    PushString(arena,"^");
  }
  String res = PointArena(arena,mark);
  return res;
}

Token ExtendToken(Token t1,Token t2){
  if(t1.size == -1){
    return t2;
  }

  Assert(t1.data <= t2.data);
  Assert(t2.size + t2.data >= t1.size + t1.data);

  if(t1.data == t2.data){
    return t2;
  }

  t1.size = (t2.data + t2.size) - t1.data;

  return t1;
}

int GetTokenPositionInside(String text,String token){
  int res = token.data - text.data;

  if(res >= text.size || res < 0){
    return -1;
  }
  return res;
}

int CountSubstring(String str,String substr){
  if(substr.size == 0){
    return 0;
  }

  int count = 0;
  for(int i = 0; i < (str.size - substr.size + 1); i++){
    const char* view = &str[i];

    bool possible = true;
    for(int ii = 0; ii < substr.size; ii++){
      if(view[ii] != substr[ii]){
        possible = false;
        break;
      }
    }

    if(possible){
      count += 1;
    }
  }

  return count;
}

void StoreToken(Token token,char* buffer){
  memcpy(buffer,token.data,token.size);
  buffer[token.size] = '\0';
}

int CompareStringNoEnd(const char* strNoEnd,const char* strWithEnd){
  for(int i = 0; strWithEnd[i] != '\0'; i++){
    if(strNoEnd[i] != strWithEnd[i]){
      return 0;
    }
  }

  return 1;
}

// The following two functions together will error if not decimal or hexadecimal
static int CharToInt(char ch){
  if(ch >= '0' && ch <= '9'){
    return ch - '0';
  } else if(ch >= 'A' && ch <= 'F'){
    return 10 + (ch - 'A');
  } else if(ch >= 'a' && ch <= 'f'){
    return 10 + (ch - 'a');
  } else {
    NOT_IMPLEMENTED;
  }
  return 0;
}

int ParseInt(String ss){
  const char* str = ss.data;
  int size = ss.size;

  int sign = 1;
  if(size >= 1 && str[0] == '-'){
    sign = -1;
    size -= 1;
    str += 1;
  }

  if(size >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')){ // Hexadecimal
      int res = 0;

      for(int i = 2; i < size; i++){
        res *= 16;
        res += CharToInt(str[i]);
      }

      return (res * sign);
  } else {
    int res = 0;
    for(int i = 0; i < size; i++){
      Assert(str[i] >= '0' && str[i] <= '9');

      res *= 10;
      res += CharToInt(str[i]);
    }
    return (res * sign);
  }
}

double ParseDouble(String str){
  static char buffer[1024];

  Memcpy(buffer,(char*) str.data,str.size);
  buffer[str.size] = '\0';

  double d;
  sscanf(buffer,"%lf",&d);

  return d;
}

float ParseFloat(String str){
  static char buffer[1024];

  Memcpy(buffer,(char*) str.data,str.size);
  buffer[str.size] = '\0';

  float d;
  sscanf(buffer,"%f",&d);

  return d;
}

bool IsNum(char ch){
  bool res = (ch >= '0' && ch <= '9');

  return res;
}

struct OperationList{
  const char** op;
  int nOperations;
  OperationList* next;
};

Expression* ParseOperationType_(Tokenizer* tok,OperationList* operators,ParsingFunction finalFunction,Arena* tempArena){
  void* start = tok->Mark();

  if(operators == nullptr){
    Expression* expr = finalFunction(tok,tempArena);

    expr->text = tok->Point(start);
    return expr;
  }

  OperationList* nextOperators = operators->next;
  Expression* current = ParseOperationType_(tok,nextOperators,finalFunction,tempArena);

  while(1){
    Token peek = tok->PeekToken();

    bool foundOne = false;
    for(int i = 0; i < operators->nOperations; i++){
      const char* elem = operators->op[i];

      if(CompareString(peek,elem)){
        tok->AdvancePeek(peek);
        Expression* expr = PushStruct<Expression>(tempArena);
        *expr = {};
        expr->expressions = PushArray<Expression*>(tempArena,2);

        expr->type = Expression::OPERATION;
        expr->op = elem;
        expr->expressions[0] = current;
        expr->expressions[1] = ParseOperationType_(tok,nextOperators,finalFunction,tempArena);

        current = expr;
        foundOne = true;
      }
    }

    if(!foundOne){
      break;
    }
  }

  current->text = tok->Point(start);
  return current;
}

Expression* ParseOperationType(Tokenizer* tok,std::initializer_list<std::initializer_list<const char*>> operators,ParsingFunction finalFunction,Arena* tempArena){
  void* mark = tok->Mark();

  OperationList head = {};
  OperationList* ptr = nullptr;

  for(std::initializer_list<const char*> outerList : operators){
    if(ptr){
      ptr->next = PushStruct<OperationList>(tempArena);
      ptr = ptr->next;
      *ptr = {};
    } else {
      ptr = &head;
    }

    ptr->op = PushArray<const char*>(tempArena,outerList.size()).data;

    for(const char* str : outerList){
      ptr->op[ptr->nOperations++] = str;
    }
  }

  Expression* expr = ParseOperationType_(tok,&head,finalFunction,tempArena);
  expr->text = tok->Point(mark);

  return expr;
}

static void PrintSpaces(int amount){
  for(int i = 0; i < amount; i++){
    printf(" ");
  }
}

static void PrintExpression(Expression* exp,int level){
  PrintSpaces(level);
  switch(exp->type){
  case Expression::UNDEFINED:{
    printf("UNDEFINED\n");
  }break;
  case Expression::OPERATION:{
    printf("OPERATION\n");
  }break;
  case Expression::IDENTIFIER:{
    printf("IDENTIFIER\n");
  }break;
  case Expression::COMMAND:{
    printf("COMMAND\n");
  }break;
  case Expression::LITERAL:{
    Value val = exp->val;
    printf("LITERAL: %ld\n",val.number);
  }break;
  case Expression::ARRAY_ACCESS:{
    printf("ARRAY_ACCESS\n");
  }break;
  case Expression::MEMBER_ACCESS:{
    printf("MEMBER_ACCESS\n");
  }break;
  default: NOT_IMPLEMENTED;
  }

  if(exp->op){
    PrintSpaces(level);
    printf("op: %s\n",exp->op);
  }

  if(exp->id.size){
    PrintSpaces(level);
    printf("id: %.*s\n",UNPACK_SS(exp->id));
  }

  for(Expression* subExpressions : exp->expressions){
    PrintExpression(subExpressions,level + 2);
    printf("\n");
  }
}

void PrintExpression(Expression* exp){
  PrintExpression(exp,0);
}

