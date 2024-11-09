#pragma once

#include <initializer_list>
#include <vector>
#include <string>

#include "utils.hpp"
#include "type.hpp"

struct Command;

// TODO: The entire code base reuses "Expression" in a lot of different situations while in reality every single Expression should be unique for the given file/module. 
struct Expression{
  const char* op;
  String id;
  Array<Expression*> expressions;
  Command* command;
  Value val;
  String text;
  int approximateLine;
  
  enum {UNDEFINED,OPERATION,IDENTIFIER,COMMAND,LITERAL,ARRAY_ACCESS,MEMBER_ACCESS} type;
};

void PrintExpression(Expression* exp);

typedef int (*CharFunction) (const char* ptr,int size);

struct Cursor{
  int line;
  int column;
};

struct Token : public String{
  Range<Cursor> loc;

  Token& operator=(String str){
    this->data = str.data;
    this->size = str.size;
    return *this;
  }
};

template<> class std::hash<Token>{
public:
   std::size_t operator()(Token const& s) const noexcept{
     return std::hash<String>()(s);
   }
};

struct FindFirstResult{
  String foundFirst;
  Token peekFindNotIncluded;
};

struct Trie{
  u16 array[128];
};

struct TokenizerTemplate{
  Array<Trie> subTries;
};

struct TokenizerMark{
  const char* ptr;
  Cursor pos;
};

#define MAX_STORED_TOKENS 4

struct Tokenizer{
  const char* start;
  const char* ptr;
  const char* end;
  TokenizerTemplate* tmpl;
  Arena leaky;
  
  Token storedTokens[MAX_STORED_TOKENS];

  // Line and column start at one. Subtract one to get zero based indexes
  int line;
  int column;
  char amountStoredTokens;
  
  // TODO: This should technically be part of the Tokenizer template
  bool keepWhitespaces;
  bool keepComments;
  
private:

  void ConsumeWhitespace();
  Token ParseToken();
  void AdvanceOneToken();
  Token PopOneToken();
  const char* GetCurrentPtr();
  
public:
  
  // TODO: Need to make a function that returns a location for a given token, so that I can return a good error message for the token not being the expected on. The function accepts a token and either returns a string or returns some structure that contains all the info needed to output such text.
  
  // TODO: Make some asserts. Special chars should not contain empty chars
  Tokenizer(String content,const char* singleChars,BracketList<const char*> specialChars); // Content must remain valid through the entire parsing process
  Tokenizer(String content,TokenizerTemplate* tmpl); // Content must remain valid. Tokenizer makes no copies
  ~Tokenizer();

  Token PeekToken(int index = 0);
  Token NextToken();

  Token AssertNextToken(const char* str);

  String PeekCurrentLine(); // Get full line (goes backwards until start of line and peeks until newline).
  Token PeekRemainingLine(); // Does not go back. 
  
  bool IfPeekToken(const char* str);
  bool IfNextToken(const char* str); // Only does "next" if token matches str

  // All these calls are not very good. No point having a tokenizer if we just skip the tokenization process
  Opt<Token> PeekFindUntil(const char* str);
  Opt<Token> PeekFindIncluding(const char* str);
  Opt<Token> PeekFindIncludingLast(const char* str);
  Opt<Token> NextFindUntil(const char* str);
  
  Opt<FindFirstResult> FindFirst(BracketList<const char*> strings);

  Token PeekWhitespace();

  Token Finish(); // Acts like a Next, puts Tokenizer at the end

  TokenizerMark Mark();
  Token Point(TokenizerMark mark);
  void Rollback(TokenizerMark mark);
  String GetContent();
  
  void AdvancePeek(int amount = 1);
  void AdvancePeekBad(Token token);

  void AdvanceRemainingLine();
  
  bool Done(); // If more tokens, returns false. Can return true even if it contains more text (but no more tokens)

  bool IsSpecialOrSingle(String toTest);

  TokenizerTemplate* SetTemplate(TokenizerTemplate* tmpl); // Returns old template

  // For expressions where there is a open and a closing delimiter (think '{ { } }') and need to check where the matching close delimiter is.
  Opt<Token> PeekUntilDelimiterExpression(BracketList<const char*> open,BracketList<const char*> close, int numberOpenSeen);

  // For expressions where there is a open and a closing delimiter (think '{ { } }') and need to advance such expressions (mostly to skip some checkions while parsing is still not fully complete)
  bool AdvanceDelimiterExpression(BracketList<const char*> open,BracketList<const char*> close, int numberOpenSeen);
};

bool IsOnlyWhitespace(String tok);
bool Contains(String str,const char* toCheck);

bool CheckFormat(const char* format,String tok);
Array<Value> ExtractValues(const char* format,String tok,Arena* arena);

Array<String> Split(String content,char sep,Arena* out); // For now only split over one char. 

String TrimLeftWhitespaces(String in);
String TrimRightWhitespaces(String in);
String TrimWhitespaces(String in);

String PushPointingString(Arena* out,int startPos,int size);

int GetTokenPositionInside(String text,Token token); // Does not compare strings, just uses pointer arithmetic

int CountSubstring(String str,String substr);

String GetFullLineForGivenToken(String content,Token token);
String GetRichLocationError(String content,Token got,Arena* out);

// This functions should check for errors. Also these functions should return an error if they do not parse everything. Something like "3a" should flag an error for ParseInt, instead of just returning 3. Either they consume everything or it's an error
int ParseInt(String str);
double ParseDouble(String str);
float ParseFloat(String str);
bool IsNum(char ch);

TokenizerTemplate* CreateTokenizerTemplate(Arena* out,const char* singleChars,BracketList<const char*> specialChars);

/* Generic expression parser. The ExpressionType struct needs to have the following members with the following types:
     Array<ExpressionType> expressions;
     const char* op;
     enum {OPERATION} type;
     Token text;
*/

template<typename Exp>
using ParsingFunction = Exp* (*)(Tokenizer* tok,Arena* out);

struct OperationList{
  const char** op;
  int nOperations;
  OperationList* next;
};

template<typename ExpressionType>
ExpressionType* ParseOperationType_(Tokenizer* tok,OperationList* operators,ParsingFunction<ExpressionType> finalFunction,Arena* out){
  auto start = tok->Mark();

  if(operators == nullptr){
    ExpressionType* expr = finalFunction(tok,out);

    expr->text = tok->Point(start);
    return expr;
  }

  OperationList* nextOperators = operators->next;
  ExpressionType* current = ParseOperationType_(tok,nextOperators,finalFunction,out);

  while(1){
    Token peek = tok->PeekToken();

    bool foundOne = false;
    for(int i = 0; i < operators->nOperations; i++){
      const char* elem = operators->op[i];

      if(CompareString(peek,elem)){
        tok->AdvancePeek();
        ExpressionType* expr = PushStruct<ExpressionType>(out);
        *expr = {};
        expr->expressions = PushArray<ExpressionType*>(out,2);

        expr->type = ExpressionType::OPERATION;
        expr->op = elem;
        expr->expressions[0] = current;
        expr->expressions[1] = ParseOperationType_(tok,nextOperators,finalFunction,out);

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

template<typename ExpressionType>
ExpressionType* ParseOperationType(Tokenizer* tok,BracketList<BracketList<const char*>> operators,ParsingFunction<ExpressionType> finalFunction,Arena* out){
  auto mark = tok->Mark();

  OperationList head = {};
  OperationList* ptr = nullptr;

  for(BracketList<const char*> outerList : operators){
    if(ptr){
      ptr->next = PushStruct<OperationList>(out);
      ptr = ptr->next;
      *ptr = {};
    } else {
      ptr = &head;
    }

    ptr->op = PushArray<const char*>(out,outerList.size()).data;

    for(const char* str : outerList){
      ptr->op[ptr->nOperations++] = str;
    }
  }

  ExpressionType* expr = ParseOperationType_<ExpressionType>(tok,&head,finalFunction,out);
  expr->text = tok->Point(mark);

  return expr;
}

