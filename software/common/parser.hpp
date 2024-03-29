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
typedef String Token;

struct FindFirstResult{
  Token foundFirst;
  Token peekFindNotIncluded;
  bool foundNone;
};

// TODO: Vast majority of extra functions could be removed if we allowed a quick change in
//       the values for single char, special chars and other configurations (like parse words or something like that)
//       Changing the tokenizer to allow this greater amount of freedom would simplify future changes. Things like
//       FindFirst and stuff are just a product of not being able to change things on the fly. 
class Tokenizer{
  const char* start;
  const char* ptr;
  const char* end;

public:
  std::string singleChars; // TODO: Why use string instead of sized string?
  std::vector<std::string> specialChars; // TODO: Why use string instead of sized string?

private:

  int SpecialChars(const char* ptr,size_t size);
  void ConsumeWhitespace();

public:

  int lines;
  // Can change mid way to alter manner in which the tokenizer handles special cases
  bool keepWhitespaces;
  bool keepComments;

public:

  // TODO: Make some asserts. Special chars should not contain empty chars
  Tokenizer(String content,const char* singleChars,std::vector<std::string> specialChars); // Content must remain valid through the entire parsing process

  Token PeekToken();
  Token NextToken();

  Token AssertNextToken(const char* str);

  Token PeekFindUntil(const char* str);
  Token PeekFindIncluding(const char* str);
  Token PeekFindIncludingLast(const char* str);
  Token NextFindUntil(const char* str);
  
  String PeekCurrentLine(); // Get full line. Not fully tested, GetCurrentLine
  
  bool IfPeekToken(const char* str);
  bool IfNextToken(const char* str);

  bool IsSingleChar(char ch);
  bool IsSingleChar(const char* chars);

  FindFirstResult FindFirst(std::initializer_list<const char*> strings);

  Token PeekWhitespace();

  Token Finish();

  Byte* Mark();
  Token Point(void* mark);
  void Rollback(void* mark);

  void AdvancePeek(Token tok);

  void SetSingleChars(const char* singleChars);
  bool Done(); // If more tokens, returns false. Can return true even if it contains more text (but no more tokens)
  Token Remaining();
  int Lines(){return lines;};

  bool IsSpecialOrSingle(String toTest);

  // For expressions where there is a open and a closing delimiter (think '{ { } }') and need to check where the matching close delimiter is.
  String PeekUntilDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen);
  String PeekIncludingDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen);
};

bool IsOnlyWhitespace(Token tok);
bool Contains(String str,const char* toCheck);

bool CheckFormat(const char* format,Token tok);
Array<Value> ExtractValues(const char* format,Token tok,Arena* arena);

Array<String> Split(String content,char sep,Arena* out); // For now only split over one char. 

String TrimWhitespaces(String in);

String GeneratePointingString(int startPos,int size,Arena* arena);

Token ExtendToken(Token t1,Token t2);
int GetTokenPositionInside(String text,String token); // Does not compare strings, just uses pointer arithmetic

int CountSubstring(String str,String substr);

void StoreToken(Token token,char* buffer);
#define CompareToken CompareString

//void PrintEscapedToken(Token tok);

// This functions should check for errors. Also these functions should return an error if they do not parse everything. Something like STRING("3 a") should flag an error for ParseInt, for example, instead of just returning 3. Either they consume everything or it's an error
int ParseInt(String str);
double ParseDouble(String str);
float ParseFloat(String str);
bool IsNum(char ch);

typedef Expression* (*ParsingFunction)(Tokenizer* tok,Arena* out);
Expression* ParseOperationType(Tokenizer* tok,std::initializer_list<std::initializer_list<const char*>> operators,ParsingFunction finalFunction,Arena* out);

/*

struct Trie{
  u16 index;
};

struct TokenizerTemplate{
  u16 array[255];
  Trie* trieBuffer;
};

TokenizerTemplate* CreateTokenizerTemplate(Arena* out,const char* singleChars,std::initializer_list<const char*> specialChars);

*/

#endif // INCLUDED_PARSER



