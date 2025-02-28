#include <cstdio>

#include "filesystem.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "utilsCore.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#define INCLUDE_GUARD "INCLUDED_GENERATED_%s\n"

bool HasExtension(const char* filepath){
  fs::path relative = filepath;
  fs::path absolute = fs::weakly_canonical(fs::absolute(relative));

  Tokenizer tok(STRING(absolute.c_str()),".",{});

  Opt<Token> str = tok.PeekFindIncluding(".");
  if(str.has_value()){
    return true;
  }
  return false;
}

// 0 - exe name
// 1 - output file with no extension
// 2+ - pair of <variable name> '=' <filename>
int main(int argc,const char* argv[]){
  if(argc < 3){
    printf("Need at least 3 arguments: <exe> <output filepath> [<variable>'='<filename>]\n");
    return -1;
  }

  const char* outputPath = argv[1];

  if(HasExtension(outputPath)){
    printf("Something is wrong with output filepath. We expected no extension, since we produce a header file and a cpp file\n");
    return -1;
  }

  String filename = ExtractFilenameOnly(STRING(argv[1]));
  const char* outName = filename.data;

  Arena inst1 = InitArena(Megabyte(64));
  contextArenas[0] = &inst1;
  Arena inst2 = InitArena(Megabyte(64));
  contextArenas[1] = &inst2;
  
  TEMP_REGION(perm,nullptr);
  TEMP_REGION(temp,perm);

  String headerPath = PushString(temp,"%s.hpp",outputPath);
  String sourcePath = PushString(temp,"%s.cpp",outputPath);
  
  FILE* headerFile = OpenFileAndCreateDirectories(headerPath,"w",FilePurpose_MISC);
  FILE* sourceFile = OpenFileAndCreateDirectories(sourcePath,"w",FilePurpose_MISC);
  DEFER_CLOSE_FILE(headerFile);
  DEFER_CLOSE_FILE(sourceFile);
  
  if(!headerFile || !sourceFile){
    printf("Failed to open output file!\n");
    return -1;
  }
  
  fprintf(headerFile,"#ifndef " INCLUDE_GUARD,outName);
  fprintf(headerFile,"#define " INCLUDE_GUARD,outName);
  fprintf(headerFile,"#include \"utils.hpp\"\n");
  fprintf(headerFile,"#include \"memory.hpp\"\n\n");

  fprintf(sourceFile,"#include \"%s.hpp\"\n\n",outName);

  Array<String> templateFiles = PushArray<String>(perm,256);
  int templatesFound = 0;
  for(int i = 2; i < argc; i++){
    String filename = STRING(argv[i]);

    Tokenizer tok(filename,"./",{});

#if 0
    while(!tok.Done()){
      Token token = tok.NextToken();
      printf("%.*s\n",UNPACK_SS(token));
    }
#endif

    Token path = tok.PeekFindIncludingLast("/").value();
    tok.AdvancePeekBad(path);

    String variable = tok.NextToken();

    tok.AssertNextToken(".");
    tok.AssertNextToken("tpl");

    templateFiles[templatesFound++] = PushString(perm,"%.*s",UNPACK_SS(variable));

    BLOCK_REGION(temp);
    String content = PushFile(temp,StaticFormat("%.*s",UNPACK_SS(filename))); // Static format for null terminated string

    fprintf(headerFile,"extern String %.*s_template;\n",UNPACK_SS(variable));
    fprintf(sourceFile,"String %.*s_template = STRING(",UNPACK_SS(variable));

    bool startOfLine = true;
    bool lastWasNewline = true;
    for(int pos = 0; pos < content.size; pos++){
      if(startOfLine){
        fprintf(sourceFile,"\"");
        startOfLine = false;
      }

      lastWasNewline = false;

      int ch = content[pos];
      switch(ch){
      case '\\':{
        fprintf(sourceFile,"\\\\");
      } break;
      case '"':{
        fprintf(sourceFile,"\\\"");
      }break;
      case '\n':{
        fprintf(sourceFile,"\\n\"\n");
        startOfLine = true;
        lastWasNewline = true;
      }break;
      case EOF:{
        printf("Unexpected EOF\n");
        exit(0);
        fprintf(sourceFile,"\\n\"\n");
        break;
      }break;
      default:{
        fprintf(sourceFile,"%c",ch);
      }break;
      }
    }
    if(!lastWasNewline){
      fprintf(sourceFile,"\\n\"\n");
    }

    fprintf(sourceFile,");\n\n");
  }

  // TODO: This program could be more generic. Right now we are hardcoding the program to do templates
  fprintf(sourceFile,"static Pair<String,String> templateNameToContentData[] = {\n");

  for(int i = 0; i < templatesFound; i++){
    fprintf(sourceFile,"{STRING(\"%.*s.tpl\"),%.*s_template}",UNPACK_SS(templateFiles[i]),UNPACK_SS(templateFiles[i]));

    if(i + 1 < templatesFound){
      fprintf(sourceFile,",\n");
    }
  }
  fprintf(sourceFile,"};\n\n");

  fprintf(sourceFile,"Array<Pair<String,String>> generated_templateNameToContent = {templateNameToContentData,%d};\n\n",templatesFound);

  fprintf(headerFile,"extern Array<Pair<String,String>> generated_templateNameToContent;\n\n");

  fprintf(headerFile,"#endif //" INCLUDE_GUARD,outName);

  return 0;
}



