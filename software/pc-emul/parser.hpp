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
   SizedString id;
   Array<Expression*> expressions;
   Command* command;
   Value val;
   SizedString text;

   enum {UNDEFINED,OPERATION,IDENTIFIER,COMMAND,LITERAL,ARRAY_ACCESS,MEMBER_ACCESS} type;
};

typedef int (*CharFunction) (const char* ptr,int size);
typedef SizedString Token;

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

   Tokenizer(SizedString content,const char* singleChars,std::vector<std::string> specialChars); // Content must remain valid through the entire parsing process

   Token PeekToken();
   Token NextToken();

   Token AssertNextToken(const char* str);

   Token PeekFindUntil(const char* str);
   Token PeekFindIncluding(const char* str);
   Token NextFindUntil(const char* str);

   bool IfPeekToken(const char* str);
   bool IfNextToken(const char* str);

   Token FindFirst(std::initializer_list<const char*> strings);

   // For expressions where there is a open and a closing delimiter (think '{...{...}...}') and need to check where an associated close delimiter is
   SizedString PeekUntilDelimiterExpression(std::initializer_list<const char*> open,std::initializer_list<const char*> close, int numberOpenSeen); //

   Token PeekWhitespace();

   Token Finish();

   void* Mark();
   Token Point(void* mark);
   void Rollback(void* mark);

   void AdvancePeek(Token tok);

   void SetSingleChars(const char* singleChars);
   bool Done();
   int Lines(){return lines;};

   bool IsSpecialOrSingle(SizedString toTest);
};

bool CheckStringOnlyWhitespace(Token tok);
bool CheckFormat(const char* format,Token tok);
bool Contains(SizedString str,const char* toCheck);

Token ExtendToken(Token t1,Token t2);

int CountSubstring(SizedString str,SizedString substr);

void StoreToken(Token token,char* buffer);
#define CompareToken CompareString

int ParseInt(SizedString str);
double ParseDouble(SizedString str);
bool IsNum(char ch);

typedef Expression* (*ParsingFunction)(Tokenizer* tok);
Expression* ParseOperationType(Tokenizer* tok,std::initializer_list<std::initializer_list<const char*>> operators,ParsingFunction finalFunction,Arena* tempArena);

#endif // INCLUDED_PARSER
