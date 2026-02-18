#include "parser.hpp"

#include <cstring>
#include <cctype>

#include "memory.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

#define EOT_INDEX 0
#define TOKEN_GOOD (u16) 0xffff
#define TOKEN_NONE (u16) 0x0

// ======================================
// Parser global templates. 

static TokenizerTemplate* multiLineCommentsTmpl;

void InitParser(Arena* perm){
  multiLineCommentsTmpl = CreateTokenizerTemplate(perm,"",{"/*","*/"});
}

void Tokenizer::ConsumeWhitespace(){
  if(keepWhitespaces){
    return;
  }

  while(1){
    if(ptr >= end)
      return;

    if(ptr[0] == '\n'){
      ptr += 1;
      line += 1;
      column = 1;
      continue;
    }

    if(std::isspace(ptr[0])){
      ptr += 1;
      column += 1;
      continue;
    }

    // Only one character left, cannot be a comment
    if(ptr + 1 >= end){
      return;
    }

    if(!keepComments){
      if(ptr[0] == '/' && ptr[1] == '/'){
        while(ptr < end && *ptr != '\n') ptr += 1;
        ptr += 1;
        line += 1;
        column = 1;
        continue;
      }

      if(ptr[0] == '/' && ptr[1] == '*'){
        while(ptr + 1 < end && !(ptr[0] == '*' && ptr[1] == '/')) {
          if(*ptr == '\n'){
            line += 1;
            column = 1;
          }
          ptr += 1;
        }
        ptr += 2;
        column += 2;
        continue;
      }
    }

    return;
  }
}

Tokenizer::Tokenizer(String content,String singleChars,BracketList<String> specialCharsList)
:start(content.data)
,ptr(content.data)
,end(content.data + content.size)
,line(1)
,column(1)
,amountStoredTokens(0)
{
  keepWhitespaces = false;
  keepComments = false;
  leaky = InitArena(Kilobyte(16));

  this->tmpl = CreateTokenizerTemplate(&leaky,singleChars,specialCharsList);
}

Tokenizer::Tokenizer(String content,TokenizerTemplate* tmpl)
:start(content.data)
,ptr(content.data)
,end(content.data + content.size)
,line(1)
,column(1)
,amountStoredTokens(0)
{
  keepWhitespaces = false;
  keepComments = false;
  leaky = {};
  this->tmpl = tmpl;
}

Tokenizer::~Tokenizer(){
  if(leaky.mem){
    Free(&leaky);
  }
}

Token Tokenizer::ParseToken(){
  ConsumeWhitespace();

  // Only returns strings that are fully specified in the tokenizer template.
  // NOTE: Now that I think about it. This seems overkill. Need to test some examples to see if this is needed at all.
  auto GetSpecial = [this](const char* ptr){
    u16 val = tmpl->subTries[0].array[*ptr];
    if(val == TOKEN_GOOD){
      String str{ptr,1};
      return str;
    } else if(val == TOKEN_NONE){
      return String{};
    } else {
      Trie* currentTrie = &tmpl->subTries[val];

      const char* start = ptr;
      const char* bestFoundEnd = nullptr;
      ptr++;
      while(ptr < end){
        u16 val = currentTrie->array[*ptr];
        u16 possibleEnd = currentTrie->array[0];

        if(possibleEnd){
          bestFoundEnd = ptr;
        }

        if(val == TOKEN_GOOD){
          String str = {start,(int) (ptr - start)};
          ptr++;
          return str;
          break;
        } else if(val == TOKEN_NONE){
          if(bestFoundEnd){
            String str = {start,(int) (bestFoundEnd - start)};
            ptr =  bestFoundEnd;
            return str;
          } else {
            return String{};
          }
          break;
        } else {
          currentTrie = &tmpl->subTries[val];
          ptr++;
        }
      }

      // Last possible change to find special.
      u16 possibleEnd = currentTrie->array[0];
      if(possibleEnd){
        String str = {start,(int) (ptr - start)};
        return str;
      }
    }

    return String{};
  };

  Token tok = {};
  tok.loc.start = {.line = this->line,.column = this->column};
  
  const char* peek = ptr;
  while(peek < end){
    if(*peek == '\0'){
      NOT_POSSIBLE();
      break;
    }

    u16 val = tmpl->subTries[0].array[*peek];
    if(val == TOKEN_GOOD){
      tok = String(peek,1); 
      tok.loc.end = {.line = this->line,.column = this->column + 1};
      return tok;
    } else if(val == TOKEN_NONE){
      const char* start = peek;
      peek++;

      while(peek < end){
        u16 val = tmpl->subTries[0].array[*peek];
        if(val == TOKEN_GOOD){
          break;
        }

        if(val != TOKEN_NONE){
          String special = GetSpecial(peek);
          if(special.size){
            break;
          }
        }
        peek++;
      }

      tok = String(start,(int) (peek - start)); 
      tok.loc.end = {.line = this->line,.column = this->column + tok.size};
      return tok;
    } else {
      Trie* currentTrie = &tmpl->subTries[val];

      const char* start = peek;
      const char* bestFoundEnd = nullptr;
      peek++;
      while(peek < end){
        u16 val = currentTrie->array[*peek];
        u16 possibleEnd = currentTrie->array[0];

        if(possibleEnd){
          bestFoundEnd = peek;
        }

        if(val == TOKEN_GOOD){
          tok = String(start,(int) (peek - start)); 
          tok.loc.end = {.line = this->line,.column = this->column + tok.size};

          return tok;
          break;
        } else if(val == TOKEN_NONE){
          if(bestFoundEnd){
            tok = String(start,(int) (bestFoundEnd - start)); 
            tok.loc.end = {.line = this->line,.column = this->column + tok.size};

            return tok;
          } else {
            tok = String(start,(int) (peek - start)); 
            tok.loc.end = {.line = this->line,.column = this->column + tok.size};
            return tok;
          }
          break;
        } else {
          currentTrie = &tmpl->subTries[val];
          peek++;
        }
      }

      // Either a special or a normal token.
      tok = String(start,(int) (peek - start)); 
      tok.loc.end = {.line = this->line,.column = this->column + tok.size};
      return tok;
    }
  }

  tok = String(ptr,(int) (peek - ptr)); 
  tok.loc.end = {.line = this->line,.column = this->column + tok.size};
  return tok;
}

void Tokenizer::AdvanceOneToken(){
  Assert(amountStoredTokens < MAX_STORED_TOKENS);

  Token tok = ParseToken();
  if(tok.size == 0){
    return;
  }

  ptr = tok.data + tok.size;
  storedTokens[amountStoredTokens++] = tok;
}

Token Tokenizer::PeekToken(int index){
  if(Done()){ // TODO: SLOW
    return {};
  }

  while(index >= amountStoredTokens){
    AdvanceOneToken();
  }

  return storedTokens[index];
}

Token Tokenizer::PopOneToken(){
  Assert(amountStoredTokens > 0);

  Token res = storedTokens[amountStoredTokens - 1];

  // TODO: Implement a proper FIFO using a read and write index instead of all this copying
  for(int i = 0; i < amountStoredTokens - 1; i++){
    storedTokens[i] = storedTokens[i+1];
  }

  amountStoredTokens -= 1;
  
  return res;
}

Token Tokenizer::NextToken(){
  if(amountStoredTokens == 0){
    AdvanceOneToken();
  }

  Token tok = PeekToken();
  AdvancePeek();
  Assert(tok.size > 0);
  return tok;
};

String Tokenizer::PeekCurrentLine(){
  const char* ptr = GetCurrentPtr();

  // Get start of the line
  const char* lineStart = ptr;
  if(lineStart[0] == '\n'){
    String res = {ptr,1};
    return res;
  }

  while(lineStart > start){
    lineStart -= 1;
    if(*lineStart == '\n'){
      lineStart += 1;
      break;
    }
  }

  // Get end of the line
  const char* lineEnd = ptr;
  while(lineEnd < end){
    if(*lineEnd == '\n'){
      break;
    }
    lineEnd += 1;
  }

  String fullLine = String(lineStart,lineEnd - lineStart + 1);

  return fullLine;
}

void Tokenizer::AdvanceRemainingLine(){
  const char* ptr = GetCurrentPtr();
  
  const char* lineEnd = ptr;
  while(lineEnd < this->end){
    if(*lineEnd == '\n'){
      break;
    }
    lineEnd += 1;
  }

  this->ptr = lineEnd + 1;
  this->amountStoredTokens = 0;
}

Token Tokenizer::PeekRemainingLine(){
  const char* ptr = GetCurrentPtr();

  const char* lineStart = ptr;
  if(*lineStart == '\n'){
    String res = {ptr,1};
    Token tok = {};
    tok = res;
    tok.loc.start.line = this->line;
    tok.loc.start.column = this->column;
    tok.loc.end.line = this->line + 1;
    tok.loc.end.column = 1;
    return tok;
  }

  // Get end of the line
  const char* lineEnd = ptr;
  while(lineEnd < this->end){
    if(*lineEnd == '\n'){
      break;
    }
    lineEnd += 1;
  }

  String fullLine = String(lineStart,lineEnd - lineStart + 1);
  Token tok = {};
  tok = fullLine;

  tok.loc.start.line = this->line;
  tok.loc.start.column = this->column;

  tok.loc.end.line = this->line + 1;
  tok.loc.end.column = 1;
  //tok.loc.end.line = this->line;
  //tok.loc.end.column = this->column + fullLine.size;
  
  return tok;
}

void Tokenizer::AdvanceComments(){
  bool prev = this->keepComments;
  this->keepComments = false;

  this->ConsumeWhitespace();

  this->keepComments = prev;
}

void Tokenizer::FlushStoredTokens(){
  if(amountStoredTokens == 0){
    return;
  }
  
  Token tok = storedTokens[0];

  this->ptr = tok.data;
  this->line = tok.loc.start.line;
  this->column = tok.loc.start.column;
  this->amountStoredTokens = 0;
}

Token Tokenizer::AssertNextToken(String format){
  Token token = NextToken();
  
  if(!CompareString(token,format)){
    String fullLine = PeekCurrentLine();
    int lineStart = GetTokenPositionInside(fullLine,token);

    printf("Parser Error.\n Expected to find:");
    PrintEscapedString(format,' ');
    printf("\n");
    printf("  Got:");
    PrintEscapedString(token,' ');
    printf("\n");
    printf("%6d | %.*s\n",line,UN(fullLine));
    printf("      "); // 6 spaces to match the 6 digits above
    printf(" | ");

    for(int i = 0; i < lineStart; i++){
      printf(" ");
    }
    printf("^\n");

    DEBUG_BREAK_OR_EXIT();
  }

  return token;
}

Opt<Token> Tokenizer::PeekFindUntil(String toFind){
  auto mark = Mark();

  while(1){
    if(Done()){
      Rollback(mark);
      return {};
    }
    
    Token peek = PeekToken();
    if(CompareString(peek,toFind)){
      break;
    }
    AdvancePeek();
  }

  Token result = Point(mark);
  Rollback(mark);

  return result;
}

Opt<Token> Tokenizer::PeekFindIncluding(String toFind){
  auto mark = Mark();
  
  while(1){
    if(Done()){
      Rollback(mark);
      return {};
    }
    
    Token token = NextToken();
    if(CompareString(token,toFind)){
      break;
    }
  }

  Token result = Point(mark);
  Rollback(mark);

  return result;
}

Opt<Token> Tokenizer::PeekFindIncludingLast(String str){
  auto mark = Mark();

  bool foundAtLeastOne = false;
  while(!Done()){
    Opt<Token> peek = PeekFindIncluding(str);

    if(!peek.has_value()){
      break;
    }

    foundAtLeastOne = true;
    AdvancePeekBad(peek.value());
  }

  if(!foundAtLeastOne){
    return {};
  }
  
  Token result = Point(mark);
  Rollback(mark);

  return result;
}

Opt<Token> Tokenizer::NextFindUntil(String str){
  Opt<Token> token = PeekFindUntil(str);
  PROPAGATE(token);
  
  AdvancePeekBad(token.value());

  return token;
}

Opt<FindFirstResult> Tokenizer::FindFirst(BracketList<String> strings){
  Token peekFind = {};
  peekFind.size = -1;

  String res = {};
  bool foundOne = false;
  for(String str : strings){
    Opt<Token> token = PeekFindIncluding(str);
    Opt<Token> peekFindToken = PeekFindUntil(str);
    
    if(CompareString(str,"\n")){
      token = PeekRemainingLine();
      peekFindToken = token;
    }
    
    if(!foundOne){
      if(token.has_value()){
        foundOne = true;
        peekFind = peekFindToken.value();
        res = str;
      }
    } else if(token.has_value() && peekFindToken.value().size < peekFind.size){
      peekFind = peekFindToken.value();
      res = str;
    }
  }

  if(!foundOne){
    return {};
  }
  
  FindFirstResult result = {};
  result.peekFindNotIncluded = peekFind;
  result.foundFirst = res;

  return result;
}

bool Tokenizer::IfPeekToken(String str){
  Token peek = PeekToken();

  if(CompareString(peek,str)){
    return true;
  }

  return false;
}

bool Tokenizer::IfNextToken(String str){
  Token peek = PeekToken();

  if(CompareString(peek,str)){
    AdvancePeek();
    return true;
  }

  return false;
}

Token Tokenizer::PeekWhitespace(){
  Token token = {};

  int line = this->line;
  int column = this->column;
  
  for(int i = 0; &ptr[i] <= end; i++){
    if(ptr[i] == '\n'){
      line += 1;
      column = 1;
    }
    if(!std::isspace(ptr[i])){
      token.size = i;
      break;
    }
    column += 1;
  }

  token.data = ptr;
  token.loc.start.line = this->line;
  token.loc.start.column = this->column;
  token.loc.end.line = line;
  token.loc.end.column = column;

  return token;
}

const char* Tokenizer::GetCurrentPtr(){
  if(amountStoredTokens > 0){
    return storedTokens[0].data;
  } else {
    return ptr;
  }
}

Token Tokenizer::Finish(){
  const char* ptr = GetCurrentPtr();
  
  Token token = {};
  token.data = ptr;
  token.size = end - ptr;
  token.loc.start.line = this->line;
  token.loc.start.column = this->column;

  for(int i = 0; &ptr[i] <= end; i++){
    if(ptr[i] == '\n'){
      this->line += 1;
      this->column = 1;
    }
    this->column += 1;
  }

  token.loc.end.line = this->line;
  token.loc.end.column = this->column;

  this->ptr = end;
  this->amountStoredTokens = 0;
  
  return token;
}

TokenizerMark Tokenizer::Mark(){
  const char* ptr = GetCurrentPtr();

  TokenizerMark mark = {};
  mark.ptr = ptr;
  mark.pos.line = line;
  mark.pos.column = column;
  return mark;
}

Token Tokenizer::Point(TokenizerMark mark){
  const char* ptr = GetCurrentPtr();

  Assert(ptr >= mark.ptr);

  Token token = {};
  token.loc.start = mark.pos;
  token.loc.end.line = this->line;
  token.loc.end.column = this->column;
  token.data = mark.ptr;
  token.size = ptr - mark.ptr;

  return token;
}

void Tokenizer::Rollback(TokenizerMark mark){
  this->line = mark.pos.line;
  this->column = mark.pos.column;
  this->ptr = mark.ptr;
  this->amountStoredTokens = 0;
}

String Tokenizer::GetContent(){
  return String(this->start,(this->end - this->start));
}

void Tokenizer::AdvancePeek(int amount){
  Assert(amount > 0);
  Token tok = {};
  for(int i = 0; i < amount; i++){
    if(amountStoredTokens == 0){
      AdvanceOneToken();
    }
    tok = PopOneToken();
  }
  
  // Some sanity checks, especially because we still have not locked in wether zero tokens should be returned or if we should return optionals and so on.
  if(tok.loc.end.line == 0){
    Assert(tok.loc.end.column == 0);
  }
  if(tok.loc.end.column == 0){
    Assert(tok.loc.end.line == 0);
  }
  if(tok.data == nullptr){
    Assert(tok.size == 0);
  }
  
  if(tok.loc.end.line > 0) this->line = tok.loc.end.line;
  if(tok.loc.end.column > 0) this->column = tok.loc.end.column;
  //if(tok.size > 0 && tok.data != nullptr) ptr = &tok.data[tok.size];
}

void Tokenizer::AdvancePeekBad(Token tok){
  // Some sanity checks, especially because we still have not locked in wether zero tokens should be returned or if we should return optionals and so on.
  if(tok.loc.end.line == 0){
    Assert(tok.loc.end.column == 0);
  }
  if(tok.loc.end.column == 0){
    Assert(tok.loc.end.line == 0);
  }
  if(tok.data == nullptr){
    Assert(tok.size == 0);
  }
  
  if(tok.loc.end.line > 0) this->line = tok.loc.end.line;
  if(tok.loc.end.column > 0) this->column = tok.loc.end.column;
  if(tok.size > 0 && tok.data != nullptr) ptr = &tok.data[tok.size];

  amountStoredTokens = 0;
}

bool Tokenizer::Done(){
  if(amountStoredTokens){
    return false;
  }

  auto mark = Mark();

  ConsumeWhitespace();

  bool res = (ptr >= end);

  Rollback(mark);

  return res;
}

bool Tokenizer::IsSpecialOrSingle(String toTest){
  const char* str = toTest.data;
  int count = 0;
  Trie* currentTrie = &tmpl->subTries[0];

  while(count < toTest.size){
    u16 val = currentTrie->array[*str];

    switch(val){
    case TOKEN_GOOD:{
      if(count + 1 == toTest.size){
        return true;
      }
    } break;
    case TOKEN_NONE:{
      return false;
    } break;
    default: {
      currentTrie = &tmpl->subTries[val];
    }
    }

    str += 1;
    count += 1;
  }

  if(currentTrie->array[0] == TOKEN_GOOD){
    return true;
  }
  
  return false;
}

TokenizerTemplate* Tokenizer::SetTemplate(TokenizerTemplate* tmpl){
  // TODO(major): Need to flush any parsed token.

  TokenizerTemplate* old = this->tmpl;
  this->tmpl = tmpl;
  return old;
}

bool IsIdentifier(String str){
  auto CheckSingle = [](char ch){
    return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_');
  };

  if(str.size <= 0){
    return false;
  }

  if(!CheckSingle(str.data[0])){
    return false;
  }

  for(int i = 1; i < str.size; i++){
    char ch = str.data[i];
    if(!CheckSingle(ch) && !(ch >= '0' && ch <= '9')){
      return false;
    }
  }

  return true;
}

bool IsOnlyWhitespace(String tok){
  for(int i = 0; i < tok.size; i++){
    char ch = tok[i];
    if(!std::isspace(ch)){
      return false;
    }
  }
  return true;
}

// There is a almost copy in type.cpp. Should find a way of reusing same code
bool CheckFormat(const char* format,String tok){
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
        NOT_POSSIBLE("Format char not finished"); // TODO: Probably should be a error that reports
      }break;
      default:{
        NOT_IMPLEMENTED("Implement as needed");
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

Array<String> Split(String content,char sep,Arena* out){
  TEMP_REGION(temp,out);
  int index = 0;
  int size = content.size;

  auto list = PushList<String>(temp);
  
  while(1){
    int start = index;
    while(index < size && content[index] != sep){
      index += 1;
    }
    int end = index; // content[end] is either sep or last character.
    
    String line = {};
    if(start >= size){
      break;
    } else if(index >= size){
      line = {&content[start],end - start};
      *list->PushElem() = line;
      break;
    } else if(content[index] == sep){
      line = {&content[start],end - start};
    } else {
      line = {&content[start],end - start + 1};
      //Assert(false);
    }

    *list->PushElem() = line;

    index += 1;
  }
  
  Array<String> res = PushArray(out,list);
  return res;
}

bool StartsWith(String toSearch,String starter){
  if(starter.size > toSearch.size){
    return false;
  }
  for(int i = 0; i < starter.size; i++){
    if(toSearch[i] != starter[i]){
      return false;
    }
  }
  return true;
}

Token ParseComment(Tokenizer* tok,Arena* out){
  bool prev = tok->keepComments;
  tok->keepComments = true;

  auto mark = tok->Mark();
  
  TOKENIZER_REGION(tok,multiLineCommentsTmpl);
  
  Token res = {};
  if(tok->IfNextToken("/*")){
    auto mark = tok->Mark();
    while(!tok->Done()){
      if(tok->IfPeekToken("*/")){
        break;
      }
      tok->NextToken();
    }

    res = tok->Point(mark);

    tok->IfNextToken("*/");
  } else {
    tok->Rollback(mark);
  }
  
  tok->keepComments = prev;
  return res;
}

String PushPointingString(Arena* out,int startPos,int size){
  TEMP_REGION(temp,out);
  StringBuilder* builder = StartString(temp);

  for(int i = 0; i < startPos; i++){
    builder->PushString(" ");
  }
  for(int i = 0; i < size; i++){
    builder->PushString("^");
  }

  return EndString(out,builder);
}

int GetTokenPositionInside(String text,Token token){
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
    NOT_POSSIBLE("Assuming that function is only called in places where we know char must be hex");
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
  case Expression::FUNCTION:{
    printf("FUNCTION\n");
  }break;
  case Expression::LITERAL:{
    Value val = exp->val;
    printf("LITERAL: %ld\n",val.number);
  }break;
  }

  if(exp->op){
    PrintSpaces(level);
    printf("op: %s\n",exp->op);
  }

  if(exp->id.size){
    PrintSpaces(level);
    printf("id: %.*s\n",UN(exp->id));
  }

  for(Expression* subExpressions : exp->expressions){
    PrintExpression(subExpressions,level + 2);
    printf("\n");
  }
}

void PrintExpression(Expression* exp){
  PrintExpression(exp,0);
}

// Basically turns whitespace characters into Good so that we terminate when 
// finding them while looking at special chars.
void DefaultTrieGood(Trie* t){
  for(u8 i = 0; i <= 32; i++){
    t->array[i]  = TOKEN_GOOD;
  }
}

TokenizerTemplate* CreateTokenizerTemplate(Arena* out,String singleChars,BracketList<String> specialChars){
  TokenizerTemplate* tmpl = PushStruct<TokenizerTemplate>(out);
  *tmpl = {};

  auto arr = StartArray<Trie>(out);

  Trie* topTrie = arr.PushElem();
  *topTrie = {};
  
  for(int i = 0; i < singleChars.size; i++){
    i8 index = (i8) singleChars[i];
    topTrie->array[index] = TOKEN_GOOD;
  }
  DefaultTrieGood(topTrie); // TODO: Should the top trie have these defaults?
  
  int triesIndex = 1; // Zero is reserved for the top trie
  for(String str : specialChars){
    int size = str.size;
    
    Trie* ptr = topTrie;
    for(int i = 0; i < size; i++){
      bool last = (i == size - 1);
      u8 ch = str[i];
      Assert(ch < 128); // Only 7-bit ascii for now 
      Assert(ch != EOT_INDEX);
      u16 val = ptr->array[ch];
      
      if(val == TOKEN_NONE){
        ptr->array[ch] += triesIndex++;
        Trie* newTrie = arr.PushElem();
        *newTrie = {};

        if(last){
          DefaultTrieGood(newTrie);
        }
        ptr = newTrie;
      } else if(val == TOKEN_GOOD){
        if(!last){
          ptr->array[ch] = triesIndex++;
          Trie* newTrie = arr.PushElem();
          *newTrie = {};
          DefaultTrieGood(newTrie);
          ptr = newTrie;
        }
      } else {
        ptr = &topTrie[val];
        if(last){
          DefaultTrieGood(ptr);
        }
      }
    }
  }

  tmpl->subTries = EndArray(arr);
  
  return tmpl;
}

String GetFullLineForGivenToken(String content,Token token){
  // TODO: Use location info to simplify this function.
  const char* insideLine = token.data;

  const char* contentStart = content.data;
  const char* contentEnd = content.data + content.size;
  
  const char* start = insideLine;
  while(start >= contentStart){
    if(start == contentStart){
      break;
    }

    if(*start == '\n'){
      start += 1;
      break;
    }

    start -= 1;
  }

  const char* end = insideLine;
  while(end <= contentEnd){
    if(end == contentEnd){
      end -= 1;
      break;
    }

    if(*end == '\n'){
      end -= 1;
      break;
    }

    end += 1;
  }

  String res = {};
  res.data = start;
  res.size = end - start + 1;
  return res;
}

String GetRichLocationError(String content,Token got,Arena* out){
  TEMP_REGION(temp,out);
  StringBuilder* builder = StartString(temp);

  String fullLine = GetFullLineForGivenToken(content,got);
  int line = got.loc.start.line;
  int columnIndex = got.loc.start.column - 1;

  builder->PushString("%6d | %.*s\n",line,UN(fullLine));
  builder->PushString("      "); // 6 spaces to match the 6 digits above
  builder->PushString(" | ");

  builder->PushString(PushPointingString(temp,columnIndex,got.size));

  return EndString(out,builder);
}
