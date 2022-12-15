#include "memory.hpp"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "logger.hpp"

int GetPageSize(){
   static int pageSize = 0;

   if(pageSize == 0){
      pageSize = getpagesize();
   }

   return pageSize;
}

static int pagesAllocated = 0;
static int pagesDeallocated = 0;

void* AllocatePages(int pages){
   static int fd = 0;

   if(fd == 0){
      fd = open("/dev/zero", O_RDWR);

      Assert(fd != -1);
   }

   pagesAllocated += pages;
   void* res = mmap(0, pages * GetPageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);
   Assert(res != MAP_FAILED);

   return res;
}

void DeallocatePages(void* ptr,int pages){
   pagesDeallocated += pages;
   munmap(ptr,pages * GetPageSize());
}

void CheckMemoryStats(){
   if(pagesAllocated != pagesDeallocated){
      Log(LogModule::MEMORY,LogLevel::WARN,"Number of pages freed/allocated: %d/%d",pagesDeallocated,pagesAllocated);
   }
}

ArenaMarker::ArenaMarker(Arena* arena){
   this->arena = arena;
   this->mark = MarkArena(arena);
}

ArenaMarker::~ArenaMarker(){
   PopMark(this->arena,this->mark);
}

void InitArena(Arena* arena,size_t size){
   arena->used = 0;
   arena->totalAllocated = size;
   arena->mem = (Byte*) calloc(size,sizeof(Byte));
}

Arena SubArena(Arena* arena,size_t size){
   Byte* mem = PushBytes(arena,size);

   Arena res = {};
   res.mem = mem;
   res.totalAllocated = size;

   return res;
}

void Free(Arena* arena){
   free(arena->mem);
   arena->totalAllocated = 0;
   arena->used = 0;
}

Byte* MarkArena(Arena* arena){
   return &arena->mem[arena->used];
}

void PopMark(Arena* arena,Byte* mark){
   arena->used = mark - arena->mem;
}

Byte* PushBytes(Arena* arena, int size){
   Byte* ptr = &arena->mem[arena->used];

   Assert(arena->used + size < arena->totalAllocated);

   memset(ptr,0,size);
   arena->used += size;

   return ptr;
}

SizedString PointArena(Arena* arena,Byte* mark){
   SizedString res = {};
   res.data = mark;
   res.size = &arena->mem[arena->used] - mark;
   return res;
}

SizedString PushFile(Arena* arena,const char* filepath){
   SizedString res = {};
   FILE* file = fopen(filepath,"r");

   if(!file){
      res.size = -1;
      return res;
   }

   long int size = GetFileSize(file);

   Byte* mem = PushBytes(arena,size + 1);
   fread(mem,sizeof(Byte),size,file);
   mem[size] = '\0';

   fclose(file);

   res.size = size;
   res.data = (const char*) mem;

   return res;
}

SizedString PushString(Arena* arena,SizedString ss){
   Byte* mem = PushBytes(arena,ss.size);

   memcpy(mem,ss.data,ss.size);

   SizedString res = {};
   res.data = (const char*) mem;
   res.size = ss.size;

   return res;
}

SizedString vPushString(Arena* arena,const char* format,va_list args){
   char* buffer = &arena->mem[arena->used];
   int size = vsprintf(buffer,format,args);

   arena->used += (size_t) (size);

   Assert(arena->used < arena->totalAllocated);

   SizedString res = MakeSizedString(buffer,size);

   return res;
}

SizedString PushString(Arena* arena,const char* format,...){
   va_list args;
   va_start(args,format);

   SizedString res = vPushString(arena,format,args);

   va_end(args);

   return res;
}

void PushNullByte(Arena* arena){
   Byte* res = PushBytes(arena,1);
   *res = '\0';
}

void BitArray::Init(Byte* memory,int bitSize){
   this->memory = memory;
   this->bitSize = bitSize;
}

void BitArray::Init(Arena* arena,int bitSize){
   this->bitSize = bitSize;
   this->memory = MarkArena(arena);
   PushBytes(arena,BitToByteSize(bitSize));
}

void BitArray::Fill(bool value){
   int fillValue = (value ? 0xff : 0x00);

   int byteSize = BitToByteSize(this->bitSize);
   for(int i = 0; i < byteSize; i++){
      this->memory[i] = fillValue;
   }
}

void BitArray::Copy(BitArray array){
   Assert(this->bitSize >= array.bitSize);

   int byteSize = BitToByteSize(this->bitSize);
   Memcpy(this->memory,array.memory,byteSize);
}

int BitArray::Get(int index){
   Assert(index < this->bitSize);

   int byteIndex = index / 8;
   int bitIndex = 7 - (index % 8);

   Byte byte = memory[byteIndex];
   int result = (byte & (1 << bitIndex) ? 1 : 0);

   return result;
}

void BitArray::Set(int index,bool value){
   Assert(index < this->bitSize);

   int byteIndex = index / 8;
   int bitIndex = 7 - (index % 8);

   if(value){
      memory[byteIndex] |= (1 << bitIndex);
   } else {
      memory[byteIndex] &= ~(1 << bitIndex);
   }
}

SizedString BitArray::PrintRepresentation(Arena* output){
   Byte* mark = MarkArena(output);
   for(int i = this->bitSize - 1; i >= 0; --i){
      int val = Get(i);

      if(val){
         PushString(output,MakeSizedString("1"));
      } else {
         PushString(output,MakeSizedString("0"));
      }
   }
   SizedString res = PointArena(output,mark);
   return res;
}

BitArray& BitArray::operator&=(const BitArray& other){
   Assert(this->bitSize == other.bitSize);

   int byteSize = BitToByteSize(this->bitSize);
   for(int i = 0; i < byteSize; i++){
      this->memory[i] &= other.memory[i];
   }

   return *this;
}

PoolInfo CalculatePoolInfo(int elemSize){
   PoolInfo info = {};

   info.unitsPerFullPage = (GetPageSize() - sizeof(PoolHeader)) / elemSize;
   info.bitmapSize = RoundUpDiv(info.unitsPerFullPage,8);
   info.unitsPerPage = (GetPageSize() - sizeof(PoolHeader) - info.bitmapSize) / elemSize;
   info.pageGranuality = 1;

   return info;
}

PageInfo GetPageInfo(PoolInfo poolInfo,Byte* page){
   PageInfo info = {};

   info.header = (PoolHeader*) (page + poolInfo.pageGranuality * GetPageSize() - sizeof(PoolHeader));
   info.bitmap = (Byte*) info.header - poolInfo.bitmapSize;

   return info;
}

GenericPoolIterator::GenericPoolIterator(Byte* page,int numberElements,int elemSize)
:fullIndex(0)
,bit(7)
,index(0)
,numberElements(numberElements)
,elemSize(elemSize)
,page(page)
{
   poolInfo = CalculatePoolInfo(elemSize);
   pageInfo = GetPageInfo(poolInfo,page);

   if(page && !(pageInfo.bitmap[index] & (1 << bit))){
      ++(*this);
   }
}

bool GenericPoolIterator::HasNext(){
   if(!page){
      return false;
   }

   if(this->fullIndex < this->numberElements){ // Kinda of a hack, for now
      return true;
   }

   return false;
}

GenericPoolIterator& GenericPoolIterator::operator++(){
   while(fullIndex < numberElements){
      fullIndex += 1;
      bit -= 1;
      if(bit < 0){
         index += 1;
         bit = 7;
      }

      if(index * 8 + (7 - bit) >= poolInfo.unitsPerPage){
         index = 0;
         bit = 7;
         page = pageInfo.header->nextPage;
         if(page == nullptr){
            break;
         }
         pageInfo = GetPageInfo(poolInfo,page);
      }

      if(pageInfo.bitmap[index] & (1 << bit)){
         break;
      }
   }

   return *this;
}

Byte* GenericPoolIterator::operator*(){
   Assert(page != nullptr);

   Byte* view = (Byte*) page;
   Byte* val = &view[(index * 8 + (7 - bit)) * elemSize];

   Assert(pageInfo.bitmap[index] & (1 << bit));

   return val;
}


