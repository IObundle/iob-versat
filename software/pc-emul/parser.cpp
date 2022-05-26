#include "parser.hpp"

#include <cstring>

#include "utils.hpp"

static int IsWhitespace(char ch){
   if(ch == '\n' || ch == ' ' || ch == '\t'){
      return 1;
   }

   return 0;
}

void Tokenizer::ConsumeWhitespace(){
   while(1){
      if(ptr >= end)
         return;

      if(ptr[0] == '\n'){
         ptr += 1;
         lines += 1;
         continue;
      }

      if(IsWhitespace(ptr[0])){
         ptr += 1;
         continue;
      }

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

      return;
   }
}

Tokenizer::Tokenizer(const char* buffer,int bufferSize,const char* singleChars,std::initializer_list<std::string> specialCharsList)
:ptr(buffer)
,end(buffer + bufferSize)
,lines(1)
,singleChars(singleChars)
{
   specialChars.reserve(specialCharsList.size());

   for(std::string str : specialCharsList){
      specialChars.push_back(str);
   }
}

int Tokenizer::SpecialChars(const char* ptr, int size){
   for(std::string special : specialChars){
      if(special.size() >= size){
         continue;
      }

      SizedString specialStr = {special.c_str(),special.size()};
      SizedString ptrStr = {ptr,specialStr.size};

      if(CompareString(ptrStr,specialStr)){
         return specialStr.size;
      }
   }

   for(int i = 0; i < singleChars.size(); i++){
      if(singleChars[i] == ptr[0]){
         return 1;
      }
   }

   return 0;
}

Token Tokenizer::PeekToken(){
   Token token = {};

   ConsumeWhitespace();

   // After this, only need to set size
   token.str = ptr;

   // Handle special tokens (custom)
   int size = SpecialChars(ptr,end - ptr);

   if(size != 0){
      token.size = size;

      return token;
   }

   for(int i = 1; &ptr[i] <= end; i++){
      token.size = i;
      if((SpecialChars(&ptr[i],end - &ptr[i])) || IsWhitespace(ptr[i])){
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

Token Tokenizer::AssertNextToken(const char* str){
   Token token = NextToken();

   Assert(CompareToken(token,str));

   return token;
}

Token Tokenizer::PeekFindUntil(const char* str){
   Token token = {};

   SizedString ss = MakeSizedString(str);

   for(int i = 0; &ptr[i] + ss.size < end; i++){
      if(ptr[i] == str[0]){
         SizedString cmp = MakeSizedString(&ptr[i],ss.size);

         if(CompareString(ss,cmp)){
            token.size = i;
            token.str = ptr;

            return token;
         }
      }
   }

   token.size = -1; // Didn't find anything

   return token;
}

Token Tokenizer::Finish(){
   Token token = {};

   token.str = ptr;
   token.size = end - ptr;

   ptr = end;

   return token;
}

void* Tokenizer::Mark(){
   return (void*) ptr;
}

Token Tokenizer::Point(void* mark){
   Assert(ptr > mark);

   Token token = {};
   token.str = ((char*)mark);
   token.size = ptr - ((char*)mark);

   return token;
}

void Tokenizer::AdvancePeek(Token tok){
   for(int i = 0; i < tok.size; i++){
      if(tok.str[i] == '\n'){
         lines += 1;
      }
   }
   ptr += tok.size;
}

bool Tokenizer::Done(){
   ConsumeWhitespace();

   bool res = ptr >= end;
   return res;
}

Token ExtendToken(Token t1,Token t2){
   if(t1.str > t2.str){
      ExtendToken(t2,t1);
   }

   t1.size = (t2.str - t1.str + t2.size);

   return t1;
}

int CountSubstring(SizedString str,SizedString substr){
   if(substr.size == 0){
      return 0;
   }

   int count = 0;
   for(int i = 0; i < (str.size - substr.size + 1); i++){
      const char* view = &str.str[i];

      bool possible = true;
      for(int ii = 0; ii < substr.size; ii++){
         if(view[ii] != substr.str[ii]){
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
   memcpy(buffer,token.str,token.size);
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
      Assert(false); // Do not care for other bases, for now
   }
}

int ParseInt(SizedString ss){
   const char* str = ss.str;
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

