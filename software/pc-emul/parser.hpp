#ifndef INCLUDED_PARSER
#define INCLUDED_PARSER

#include <initializer_list>
#include <vector>
#include <string>

#include "utils.hpp"

typedef int (*CharFunction) (const char* ptr,int size);
typedef SizedString Token;

class Tokenizer{
   const char* ptr;
   const char* end;
   int lines;
   std::string singleChars;
   std::vector<std::string> specialChars;

private:

   int SpecialChars(const char* ptr, int size);
   void ConsumeWhitespace();

public:

   Tokenizer(const char* buffer,int bufferSize,const char* singleChars,std::initializer_list<std::string> specialChars); // Buffer must remain valid through the entire parsing process

   Token PeekToken();
   Token NextToken();

   Token AssertNextToken(const char* str);

   Token PeekFindUntil(const char* str);

   Token Finish();

   void* Mark();
   Token Point(void* mark);

   void AdvancePeek(Token tok);

   bool Done();
   int Lines(){return lines;};

};

Token ExtendToken(Token t1,Token t2);

void StoreToken(Token token,char* buffer);
#define CompareToken CompareString

int ParseInt(SizedString str);

#endif // INCLUDED_PARSER
