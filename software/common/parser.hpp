#pragma once

#include "utils.hpp"
#include "utilsCore.hpp"

enum TokenType : u16{
  TokenType_INVALID = 0,
  TokenType_EOF,
  TokenType_NEWLINE,
  TokenType_WHITESPACE,
  TokenType_COMMENT,
  TokenType_UNTERMINATED_MULTILINE_COMMENT,

  // Single characters are equal to their ASCII value.
  // { Start characters
  TokenType_CHAR_GROUP_0_START = '!',
  TokenType_CHAR_GROUP_0_LAST = '/',
  TokenType_CHAR_GROUP_1_START = ':',
  TokenType_CHAR_GROUP_1_LAST = '@',
  TokenType_CHAR_GROUP_2_START = '[',
  TokenType_CHAR_GROUP_2_LAST = '`',
  TokenType_CHAR_GROUP_3_START = '{',
  TokenType_CHAR_GROUP_3_LAST = '~',
  // } End characters

  // Normal types commonly used
  TokenType_IDENTIFIER = 128,
  TokenType_NUMBER,
  TokenType_FILEPATH,
  
  // Double digit symbols
  TokenType_DOUBLE_DOT,     // ..
  TokenType_DOUBLE_HASHTAG, // ##
  TokenType_ARROW,          // ->
  TokenType_SHIFT_RIGHT,    // >>
  TokenType_SHIFT_LEFT,     // <<
  TokenType_XOR_EQUAL,      // ^=
  
  // Triple digit symbols
  TokenType_ROTATE_RIGHT,   // >><
  TokenType_ROTATE_LEFT,   // ><<

  TokenType_VERILOG_ATTRIBUTE_START, // (*
  TokenType_VERILOG_ATTRIBUTE_END,   // *)
  
  // Keywords
  // { Start keywords
  TokenType_KEYWORD_MODULE,
  TokenType_KEYWORD_MERGE,
  TokenType_KEYWORD_SHARE,
  TokenType_KEYWORD_STATIC,
  TokenType_KEYWORD_DEBUG,
  TokenType_KEYWORD_CONFIG,
  TokenType_KEYWORD_STATE,
  TokenType_KEYWORD_MEM,
  TokenType_KEYWORD_FOR,
  // } End keywords

  // Verilog preprocessing directives 
  // { Start VERILOG_PREPROCESS
  TokenType_VERILOG_DEFINE,
  TokenType_VERILOG_UNDEF,
  TokenType_VERILOG_TIMESCALE,
  TokenType_VERILOG_INCLUDE,
  // nocheckin: Missing (timescale, resetall, undefineall)
  TokenType_VERILOG_IFDEF,
  TokenType_VERILOG_IFNDEF,
  TokenType_VERILOG_ELSE,
  TokenType_VERILOG_ELSIF,
  TokenType_VERILOG_ENDIF,
  // Any token that starts with an ` but is not a define
  TokenType_VERILOG_PREPROCESS, 
  // } End VERILOG_PREPROCESS

  // { Start Verilog Keywords
  TokenType_VERILOG_KEYWORD_MODULE,
  TokenType_VERILOG_KEYWORD_ENDMODULE,
  TokenType_VERILOG_KEYWORD_PARAMETER,
  TokenType_VERILOG_KEYWORD_SIGNED,
  TokenType_VERILOG_KEYWORD_INPUT,
  TokenType_VERILOG_KEYWORD_OUTPUT,
  TokenType_VERILOG_KEYWORD_INOUT,
  TokenType_VERILOG_KEYWORD_REG,
  TokenType_VERILOG_KEYWORD_WIRE,
  // } End Verilog Keywords

  // TODO: While this is something that is kinda cool to have, it
  //       might also be fundamentally wrong.  Because of arrays and
  //       stuff like that, it is possible to have certain units have
  //       the name of keywords and still cause no problems.  A
  //       const[N] array will create units whose name is const_0,
  //       const_1 and so on, which do not cause problems from c or
  //       verilog POV.  We probably want to move this check to
  //       someplace else instead of pushing this responsibility to
  //       the parser itself. Just because we parse a C keyword does
  //       not mean that we produce a C keyword.
  // NOTE: We probably just want to check if any FU instance has a C
  //       keyword name just before we finish registering the module.
  //       It seems to be the better place to put this check.

  // We do not really care which keyword it is. We just want to make
  // sure that the generated C code is syntatically
  // correct since the user might use a C keyword in place of
  // a name and cause problems later on (Ex: 'const' is a valid name
  // from the POV of Versat but its a keyword in C which causes
  // problems when generating the C structs and so on).
  TokenType_C_KEYWORD,

  TokenType_C_STRING,

  // We solve this by adding a number at the end of every instance
  // so for now this is mostly unused
  TokenType_VERILOG_KEYWORD
};

#define TokenType_START_OF_KEYWORDS   (TokenType_KEYWORD_MODULE)
#define TokenType_END_OF_KEYWORDS     (TokenType_KEYWORD_FOR + 1)

#define TokenType_START_OF_VERILOG_PREPROCESS   (TokenType_VERILOG_DEFINE)
#define TokenType_END_OF_VERILOG_PREPROCESS     (TokenType_VERILOG_PREPROCESS + 1)

#define TokenType_START_OF_VERILOG_KEYWORDS   (TokenType_VERILOG_KEYWORD_MODULE)
#define TokenType_END_OF_VERILOG_KEYWORDS   (TokenType_VERILOG_KEYWORD_WIRE + 1)

#define TOK_TYPE(IN) ((TokenType) IN)

struct TokenLocation{
  //FileContent content;
  int bytePos;
  int line;
  int column;
};

struct Token{
  TokenType type;

  String originalData;
  FileContent originalFile;

  union{
    String identifier;
    String whitespace;
    String comment;
    String cString;
    String filepath;
    i64 number;
  };
};

String PARSE_PushDebugRepr(Arena* out,Token token);

struct TokenizeResult{
  Token token;
  u32 bytesParsed;
};

inline TokenizeResult& operator|=(TokenizeResult& lhs,TokenizeResult rhs){
  if(lhs.token.type == TokenType_INVALID){
    lhs = rhs;
  }

  return lhs;
}

struct DefaultTokenizerState{
  const char* start;
  const char* ptr;
  const char* end;
  FileContent content;
};

typedef Token (*TokenizeFunction)(void* tokenizerState);

#define MAX_STORED_TOKENS 4

enum ParsingOptions{
  ParsingOptions_NONE = 0,

  ParsingOptions_SKIP_WHITESPACE = (1 << 0),
  ParsingOptions_SKIP_COMMENTS   = (1 << 1),

  ParsingOptions_ERROR_ON_C_KEYWORDS = (1 << 2),
  ParsingOptions_ERROR_ON_VERILOG_KEYWORDS = (1 << 3),

  ParsingOptions_ERROR_ON_C_VERILOG_KEYWORDS = (ParsingOptions_ERROR_ON_C_KEYWORDS | ParsingOptions_ERROR_ON_VERILOG_KEYWORDS),

  ParsingOptions_DEFAULT = (ParsingOptions_SKIP_WHITESPACE | ParsingOptions_SKIP_COMMENTS)
};

inline ParsingOptions operator|(ParsingOptions lhs,ParsingOptions rhs){
  ParsingOptions res = (ParsingOptions) ((int) lhs | (int) rhs);
  return res;
}

struct Parser{
  void* tokenizerState;
  Arena* arena;

  u8 amountStored;
  Token storedTokens[MAX_STORED_TOKENS];

  TokenizeFunction tokenizer;

  ArenaList<String>* errors;

  ParsingOptions options;
  const char* currentFile; // Optional, gives better error messages

  // Helpers
  void EnsureTokens(int amount);
  void ReportError(String error);
  
  void ReportUnexpectedToken(Token token,BracketList<TokenType> expectedList);

  // NOTE: Options does not currently reset the stored tokens. This means that if we peek a bunch of tokens ahead
  //       and then change options it might be possible that we ignore or return more tokens than we expected.
  //       Regardless, the parsing process never peeks ahead more than it needs so it might be fine.
  //       If calling Next and such then we can change options easily. If we are peeking then need to be careful.
  ParsingOptions SetOptions(ParsingOptions options);

  Token NextToken();
  Token PeekToken(int lookahead = 0);

  bool IfNextToken(TokenType type);
  bool IfNextToken(char singleChar);

  bool IfPeekToken(TokenType type);
  bool IfPeekToken(char singleChar);
  
  Token ExpectNext(TokenType type);
  Token ExpectNext(char singleChar);

  void ExpectIdentifier(String expectedContent);

  void Synch(BracketList<TokenType> possibleTypes);

  bool Done();
};

Parser* StartParsing(TokenizeFunction tokenizer,String content,Arena* freeArena,ParsingOptions = ParsingOptions_DEFAULT);
Parser* StartParsing(TokenizeFunction tokenizer,void* tokenizerState,Arena* freeArena,ParsingOptions = ParsingOptions_DEFAULT);

// ============================================================================
// Tokenizer function helpers

enum ParseWhitespaceOptions{
  ParseWhitespaceOptions_NONE = 0,
  ParseWhitespaceOptions_INCLUDE_NEWLINES = (1 << 1),

  ParseWhitespaceOptions_DEFAULT = ParseWhitespaceOptions_INCLUDE_NEWLINES
};

TokenizeResult ParseWhitespace(const char* start,const char* end,ParseWhitespaceOptions options = ParseWhitespaceOptions_DEFAULT);
TokenizeResult ParseNewline(const char* start,const char* end);
TokenizeResult ParseComments(const char* start,const char* end);
TokenizeResult ParseSymbols(const char* start,const char* end);
TokenizeResult ParseNumber(const char* start,const char* end);
TokenizeResult ParseIdentifier(const char* start,const char* end);
TokenizeResult ParseMultiSymbol(const char* start,const char* end,String format,TokenType result);

TokenizeResult ParseVerilogPreprocess(const char* start,const char* end);

TokenizeResult ParseCString(const char* start,const char* end);

// Since this is only for helper code, we define that all filepaths must begin with an '.'
// This separates them from other normal identifiers 
TokenizeResult ParseFilepath(const char* start,const char* end);

//TODO: Create a parse remaining so that any other symbol does not cause problems further down the line.

// ======================================
// Check if identifier is a keyword in another language.
// The intent was to prevent Versat from generating an invalid software or hardware file 
// by using a keyword in a place that is not allowed, but so far it might be better to put this logic
// after the parsing is done since there is no guarantee that we generate a keyword from the input.

bool PARSE_IsCKeyword(String identifier);
bool PARSE_IsVerilogKeyword(String identifier);

// ======================================
// Location

struct LocInfo{
  int line;
  int column;

  Array<String> allLines;

  Array<String> linesBefore;
  String lineContent;
  Array<String> linesAfter;
};

LocInfo PARSE_GetLinesAroundLocation(const char* pos,String content, int linesBefore, int linesAfter,Arena* out);
