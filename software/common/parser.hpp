#pragma once

#include "utils.hpp"
#include "utilsCore.hpp"

struct Tokenizer;

enum ValueType{
  ValueType_NIL,
  ValueType_NUMBER,
  ValueType_STRING,
  ValueType_BOOLEAN
};

struct Value{
  ValueType type;

  union{
    bool boolean;
    char ch;
    i64 number;
    String str;
  };
};

// TODO: Expression is now mostly a Verilog type thing. We can remove it from here and make it fully Verilog specific.
struct Expression{
  const char* op;
  String id;
  Array<Expression*> expressions;
  Value val;
  String text;
  int approximateLine;
  
  enum {UNDEFINED,OPERATION,IDENTIFIER,FUNCTION,LITERAL} type;
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

/*
  TODO: The parser code is breaking in multiple places and currently only works because some hacks are being made.
        The biggest problem is the way templates are "defined". Having to tell the tokenizer all the combinations of symbols that we care about is kinda stupid, because if the parser looks at a symbol and does not recognize it then we probably want to return from wathever we are trying to parse and let the upper code try to make sense of it. Instead, the only thing that the template stuff does is tell us whether we want to treat something as a symbol (and return it immediatly) or if we want to treat it as an identifier (and consume as many non whitespace characters as possible).
        Ex: It is stupid that the parser looks at "3{" and treats this as a token, unless we tell it that '{' is a symbol so that the parser actually outputs a "3" and a "{" seperatly. It is also stupid how we are approaching this anyway. We know full well that the languages we are trying to parse follow basically the same set of rules. The only major difference is which set of multicharacter symbols the language supports ( "<<" might mean something in one language while in the other we want to parse each "<" individually).
        The second stupid thing that we are trying to do is not handling comments properly. Because Verilog does not support attributes in the places that we want it to, we have no choice than to use comments to pass Versat specific info. There is no other choice, comments are the only thing that allow us to put wathever information we want in wathever place we want. Because the tokenizer was not built with this in mind, there are a lot of places where the code is kinda uglier than it needs to be.
*/

struct Tokenizer{
  const char* start;
  const char* ptr;
  const char* end;
  TokenizerTemplate* tmpl;
  Arena leaky;
  
  // Currently a FIFO implemented by copying from i+1 to i to remove it.
  Token storedTokens[MAX_STORED_TOKENS];

  // Line and column start at one. Subtract one to get zero based indexes
  int line;
  int column;
  char amountStoredTokens;
  
  // TODO: This should technically be part of the Tokenizer template
  bool keepWhitespaces;
  bool keepComments;

  // Mostly implementation stuff, generally use the functions below unless you know what you are doing.
  void ConsumeWhitespace();
  Token ParseToken();
  void AdvanceOneToken();
  Token PopOneToken();
  const char* GetCurrentPtr();
  
  // TODO: Need to make a function that returns a location for a given token, so that I can return a good error message for the token not being the expected on. The function accepts a token and either returns a string or returns some structure that contains all the info needed to output such text.
  
  // TODO: Make some asserts. Special chars should not contain empty chars
  Tokenizer(String content,String singleChars,BracketList<String> specialChars); // Content must remain valid through the entire parsing process
  Tokenizer(String content,TokenizerTemplate* tmpl); // Content must remain valid. Tokenizer makes no copies
  ~Tokenizer();

  Token PeekToken(int index = 0);
  Token NextToken();

  Token AssertNextToken(String str);

  String PeekCurrentLine(); // Get full line (goes backwards until start of line and peeks until newline).
  Token PeekRemainingLine(); // Does not go back. 

  void AdvanceComments();

  void FlushStoredTokens();
  
  bool IfPeekToken(String str);
  bool IfNextToken(String str); // Only does "next" if token matches str

  // All these calls are not very good. No point having a tokenizer if we just skip the tokenization process
  Opt<Token> PeekFindUntil(String str);
  Opt<Token> PeekFindIncluding(String str);
  Opt<Token> PeekFindIncludingLast(String str);
  Opt<Token> NextFindUntil(String str);
  
  Opt<FindFirstResult> FindFirst(BracketList<String> strings);

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
};

void InitParser(Arena* perm);

bool IsIdentifier(String str); // Default identifier rules (starts with alpha or '_' and can contain numbers after)
bool IsOnlyWhitespace(String tok);
bool Contains(String str,String toCheck);
bool StartsWith(String toSearch,String starter);

Token ParseComment(Tokenizer* tok,Arena* out);

bool CheckFormat(const char* format,String tok);
Array<Value> ExtractValues(const char* format,String tok,Arena* arena);

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

// TODO: This is BROKEN. The main problem is that we do not want to have to indicate symbols that we do not care about. The default way of handling stuff is that every single single symbol should be parsed individually EXPECT unless specialChars contains a multi line symbol at which point the tokenizer attemps to parse bigger. (Basically, we want identifier, numbers and single symbol rules by default and the only difference between templates is which symbols are grouped into a bigger symbol [basically wether stuff like == gets parsed into two symbols or just one]).

TokenizerTemplate* CreateTokenizerTemplate(Arena* out,String singleChars,BracketList<String> specialChars);

struct TemplateMarker{
  TokenizerTemplate* previousTemplate;
  Tokenizer* tok;

  TemplateMarker(Tokenizer* tok,TokenizerTemplate* newTemplate){this->tok = tok; previousTemplate = tok->SetTemplate(newTemplate);};
  ~TemplateMarker(){tok->SetTemplate(previousTemplate);};
}; 

#define TOKENIZER_REGION(TOK,TMPL) TemplateMarker _marker(__LINE__)(TOK,TMPL)



// TODO: We want to remove this. Trying to make a generic parser for this kind of stuff is more trouble than it is worth. It is easier to copy code and make changes than it is trying to force everything into a single interface. It also makes it easier to make changes later on which we probably need to do since different languages have different ways of handling this kind of stuff.

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

