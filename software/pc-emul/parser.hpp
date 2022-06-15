#ifndef INCLUDED_PARSER
#define INCLUDED_PARSER

#include <initializer_list>
#include <vector>
#include <string>

#include "utils.hpp"
#include "type.hpp"

struct Expression{
   const char* op;
   SizedString id;
   Expression** expressions;
   int size;
   Value val;

   enum {UNDEFINED,OPERATION,IDENTIFIER,LITERAL,ARRAY_ACCESS,MEMBER_ACCESS} type;
};

typedef int (*CharFunction) (const char* ptr,int size);
typedef SizedString Token;

class Tokenizer{
   const char* ptr;
   const char* end;
   int lines;
   std::string singleChars;
   std::vector<std::string> specialChars;

public:

   bool keepWhitespaces;
   bool keepComments;

private:

   int SpecialChars(const char* ptr, int size);
   void ConsumeWhitespace();

public:

   Tokenizer(SizedString content,const char* singleChars,std::vector<std::string> specialChars); // Content must remain valid through the entire parsing process

   Token PeekToken();
   Token NextToken();

   Token AssertNextToken(const char* str);

   Token PeekFindUntil(const char* str);
   Token PeekFindIncluding(const char* str);
   Token NextFindUntil(const char* str);

   Token FindFirst(std::initializer_list<const char*> strings);

   Token PeekWhitespace();

   Token Finish();

   void* Mark();
   Token Point(void* mark);
   void Rollback(void* mark);

   void AdvancePeek(Token tok);

   bool Done();
   int Lines(){return lines;};

};

bool CheckFormat(const char* format,Token tok);

Token ExtendToken(Token t1,Token t2);

int CountSubstring(SizedString str,SizedString substr);

void StoreToken(Token token,char* buffer);
#define CompareToken CompareString

int ParseInt(SizedString str);
bool IsNum(char ch);

typedef Expression* (*ParsingFunction)(Tokenizer* tok);
Expression* ParseOperationType(Tokenizer* tok,std::vector<std::vector<const char*>> operators,ParsingFunction finalFunction,Arena* tempArena);

template<typename T>
struct SimpleHash{
   std::size_t operator()(T const& val) const noexcept{
      char* view = (char*) &val;

      size_t res = 0;
      std::hash<char> hasher;
      for(int i = 0; i < sizeof(T); i++){
         res += hasher(view[i]);
      }
      return res;
   }
};

template<typename T>
struct SimpleEqual{
   bool operator()(const T &left, const T &right) const {
      bool res = (memcmp(&left,&right,sizeof(T)) == 0);

      return res;
   }
};

template<>
struct SimpleHash<SizedString>{
   std::size_t operator()(SizedString const& val) const noexcept{
      size_t res = 0;
      std::hash<char> hasher;
      for(int i = 0; i < val.size; i++){
         res += hasher(val.str[i]);
      }
      return res;
   }
};

template<>
struct SimpleEqual<SizedString>{
   bool operator()(const SizedString &left, const SizedString &right) const {
      bool res = CompareString(left,right);

      return res;
   }
};


#endif // INCLUDED_PARSER
