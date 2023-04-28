#include <cstdio>

#include "parser.hpp"
#include "utils.hpp"

#ifdef STANDALONE

Arena tempInst;
Arena* temp = &tempInst;
Arena permInst;
Arena* perm = &permInst;

// 0 - exe name
// 1 - output file
// 2+ - pair of <variable name> '=' <filename>
int main(int argc,const char* argv[]){
   if(argc < 3){
      printf("Need at least 3 arguments: <exe> <output filepath> [<variable>'='<filename>]\n");
      return 0;
   }

   for(int i = 0; i < argc; i++){
      printf("%s\n",argv[i]);
   }

   tempInst = InitArena(Megabyte(256));
   permInst = InitArena(Megabyte(64));

   FILE* output = OpenFileAndCreateDirectories(argv[1],"w");

   if(!output){
      printf("Failed to open output file!\n");
      return -1;
   }

   Array<String> templateFiles = PushArray<String>(perm,256);
   int templatesFound = 0;
   for(int i = 2; i < argc; i++){
      String filename = STRING(argv[i]);

      Tokenizer tok(filename,".",{});

      String path = tok.PeekFindIncludingLast("/");
      tok.AdvancePeek(path);

      String variable = tok.NextToken();
      tok.AssertNextToken(".");
      tok.AssertNextToken("tpl");

      templateFiles[templatesFound++] = PushString(perm,"%.*s",UNPACK_SS(variable));

      BLOCK_REGION(temp);
      String content = PushFile(temp,StaticFormat("%.*s",UNPACK_SS(filename))); // Static format for null terminated string

      fprintf(output,"static String %.*s = STRING(",UNPACK_SS(variable));

      bool startOfLine = true;
      bool lastWasNewline = true;
      for(int pos = 0; pos < content.size; pos++){
         if(startOfLine){
            fprintf(output,"\"");
            startOfLine = false;
         }

         lastWasNewline = false;

         int ch = content[pos];
         switch(ch){
            case '\\':{
               fprintf(output,"\\\\");
            } break;
            case '"':{
               fprintf(output,"\\\"");
            }break;
            case '\n':{
               fprintf(output,"\\n\"\n");
               startOfLine = true;
               lastWasNewline = true;
            }break;
            case EOF:{
               printf("UUAHIDHAIHD\n");
               exit(0);
               fprintf(output,"\\n\"\n");
               break;
            }break;
            default:{
            fprintf(output,"%c",ch);

            }break;
         }
      }
      if(!lastWasNewline){
         fprintf(output,"\\n\"\n");
      }

      fprintf(output,");\n\n");
   }

   fprintf(output,"Pair<String,String> templateNameToContentData[] = {\n");

   for(int i = 0; i < templatesFound; i++){
      fprintf(output,"{STRING(\"%.*s.tpl\"),%.*s}",UNPACK_SS(templateFiles[i]),UNPACK_SS(templateFiles[i]));

      if(i + 1 < templatesFound){
         fprintf(output,",\n");
      }
   }
   fprintf(output,"};\n\n");

   fprintf(output,"Array<Pair<String,String>> templateNameToContent = {templateNameToContentData,%d};\n\n",templatesFound);

   fclose(output);

   return 0;
}

#endif // STANDALONE













