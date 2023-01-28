#ifndef INCLUDED_PARSER
#define INCLUDED_PARSER

#include <initializer_list>
#include <vector>
#include <string>

#include "utils.hpp"
#include "type.hpp"

struct Command;

struct Expression{
   const char* op;
   String id;
   Array<Expression*> expressions;
   Command* command;
   Value val;
   String text;

   enum {UNDEFINED,OPERATION,IDENTIFIER,COMMAND,LITERAL,ARRAY_ACCESS,MEMBER_ACCESS} type;
};

typedef int (*CharFunction) (const char* ptr,int size);
typedef String Token;

class Tokenizer{
   const char* start;
   const char* ptr;
   const char* end;
   int lines;
   std::string singleChars; // TODO: Why use string instead of sized string?
   std::vector<std::string> specialChars; // TODO: Why use string instead of sized string?

public:

   bool keepWhitespaces;
   bool keepComments;

private:

   int SpecialChars(const char* ptr,size_t size);
   void ConsumeWhitespace();

public:

   Tokenizer(String content,const char* singleChars,std::vector<std::string> specialChars); // Content must remain valid through the entire parsing process

   Token PeekToken();
   Token NextToken();

   Token AssertNextToken(const char* str);

   Token PeekFindUntil(const char* str);
   Token PeekFindIncluding(const char* str);
   Token NextFindUntil(const char* str);

   String GetStartOfCurrentLine();

   bool IfPeekToken(const char* str);
   bool IfNextToken(const char* str);

   bool IsSingleChar(char ch);
   bool IsSingleChar(const char* chars);

   Token FindFirst(std::initializer_list<const char*> strings);

   // For expressions where there is a open and a closing delimiter (think '{...{...}...}') and need to check where an associated close delimiter is
   String PeekUntilDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen); //

   Token PeekWhitespace();

   Token Finish();

   void* Mark();
   Token Point(void* mark);
   void Rollback(void* mark);

   void AdvancePeek(Token tok);

   void SetSingleChars(const char* singleChars);
   bool Done();
   int Lines(){return lines;};

   bool IsSpecialOrSingle(String toTest);
};

bool CheckStringOnlyWhitespace(Token tok);
bool CheckFormat(const char* format,Token tok);
bool Contains(String str,const char* toCheck);

String TrimWhitespaces(String in);

String GeneratePointingString(int startPos,int size,Arena* arena);

Token ExtendToken(Token t1,Token t2);
int GetTokenPositionInside(String text,String token); // Does not compare strings, just uses pointer arithmetic

int CountSubstring(String str,String substr);

void StoreToken(Token token,char* buffer);
#define CompareToken CompareString

int ParseInt(String str);
double ParseDouble(String str);
float ParseFloat(String str);
bool IsNum(char ch);

typedef Expression* (*ParsingFunction)(Tokenizer* tok,Arena* arena);
Expression* ParseOperationType(Tokenizer* tok,std::initializer_list<std::initializer_list<const char*>> operators,ParsingFunction finalFunction,Arena* tempArena);

#endif // INCLUDED_PARSER
