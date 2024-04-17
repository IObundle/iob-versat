#ifndef INCLUDED_PARSER
#define INCLUDED_PARSER

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

class Tokenizer{
public:
  const char* start;
  const char* ptr;
  const char* end;
  TokenizerTemplate* tmpl;
  
private:

  void ConsumeWhitespace();

public:

  // Line and column start at one. Subtract one to get zero based indexes
  int line;
  int column;

  // TODO: This should technically be part of the Tokenizer template
  bool keepWhitespaces;
  bool keepComments;

public:

  String GetFullLineForGivenToken(Token token);
  
  // TODO: Need to make a function that returns a location for a given token, so that I can return a good error message for the token not being the expected on. The function accepts a token and either returns a string or returns some structure that contains all the info needed to output such text.
  
  // TODO: Make some asserts. Special chars should not contain empty chars
  Tokenizer(String content,const char* singleChars,std::vector<std::string> specialChars); // Content must remain valid through the entire parsing process
  Tokenizer(String content,TokenizerTemplate* tmpl); // Content must remain valid. Tokenizer makes no copies
  Token PeekToken();
  Token NextToken();

  String GetRichLocationError(Token got,Arena* out);
  Token AssertNextToken(const char* str);

  String PeekCurrentLine(); // Get full line (goes backwards until start of line and peeks until newline).
  Token PeekRemainingLine(); // Does not go back. 
  
  bool IfPeekToken(const char* str);
  bool IfNextToken(const char* str);

  Optional<Token> PeekFindUntil(const char* str);
  Optional<Token> PeekFindIncluding(const char* str);
  Optional<Token> PeekFindIncludingLast(const char* str);
  Optional<Token> NextFindUntil(const char* str);

  Optional<FindFirstResult> FindFirst(std::initializer_list<const char*> strings);

  Token PeekWhitespace();

  Token Finish(); // Acts like a Next, puts Tokenizer at the end

  TokenizerMark Mark();
  Token Point(TokenizerMark mark);
  void Rollback(TokenizerMark mark);
  
  void AdvancePeek(Token tok);

  bool Done(); // If more tokens, returns false. Can return true even if it contains more text (but no more tokens)

  bool IsSpecialOrSingle(String toTest);

  TokenizerTemplate* SetTemplate(TokenizerTemplate* tmpl); // Returns old template

  // For expressions where there is a open and a closing delimiter (think '{ { } }') and need to check where the matching close delimiter is.
  Optional<Token> PeekUntilDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen);
  Optional<Token> PeekIncludingDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen);
};

bool IsOnlyWhitespace(String tok);
bool Contains(String str,const char* toCheck);

bool CheckFormat(const char* format,String tok);
Array<Value> ExtractValues(const char* format,String tok,Arena* arena);

Array<String> Split(String content,char sep,Arena* out); // For now only split over one char. 
String TrimWhitespaces(String in);

String PushPointingString(Arena* out,int startPos,int size);

int GetTokenPositionInside(String text,Token token); // Does not compare strings, just uses pointer arithmetic

int CountSubstring(String str,String substr);

// This functions should check for errors. Also these functions should return an error if they do not parse everything. Something like "3a" should flag an error for ParseInt, instead of just returning 3. Either they consume everything or it's an error
int ParseInt(String str);
double ParseDouble(String str);
float ParseFloat(String str);
bool IsNum(char ch);

typedef Expression* (*ParsingFunction)(Tokenizer* tok,Arena* out);
Expression* ParseOperationType(Tokenizer* tok,std::initializer_list<std::initializer_list<const char*>> operators,ParsingFunction finalFunction,Arena* out);

TokenizerTemplate* CreateTokenizerTemplate(Arena* out,const char* singleChars,std::vector<const char*> specialChars);

#endif // INCLUDED_PARSER



