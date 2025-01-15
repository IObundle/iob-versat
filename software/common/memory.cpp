#include "memory.hpp"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <errno.h>

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "logger.hpp"
#include "intrinsics.hpp"
#include "utilsCore.hpp"

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
  void* res = mmap(0, pages * GetPageSize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  Assert(res != MAP_FAILED);
 
  pagesAllocated += pages;
  return res;
}

void DeallocatePages(void* ptr,int pages){
  pagesDeallocated += pages;
  munmap(ptr,pages * GetPageSize());
}

long PagesAvailable(){
  long res = get_avphys_pages(); // No idea how good this is, appears to "kinda" work after large allocations (and writing to the pages).
  return res;
}

void CheckMemoryStats(){
  if(pagesAllocated != pagesDeallocated){
    LogWarn(LogModule::MEMORY,"Number of pages freed/allocated: %d/%d",pagesDeallocated,pagesAllocated);
  }
}

void AlignArena(Arena* arena,int alignment){
  int offset = arena->used % alignment;

  if(offset == 0){
    return;
  } else {
    PushBytes(arena,alignment - offset);
  }
}

Arena InitArena(size_t size){
  Arena arena = {};

  arena.used = 0;
  arena.totalAllocated = size;
  arena.mem = (Byte*) calloc(size,sizeof(Byte));
  
  if(arena.mem == nullptr){
    fprintf(stderr,"Error allocating memory. Make sure enough memory is available\n");
    exit(1);
  }

  Assert(IS_ALIGNED_64(arena.mem));

  return arena;
}

Arena InitLargeArena(){
  Arena arena = {};

  arena.used = 0;
  arena.totalAllocated = Gigabyte(1);
  arena.mem = (Byte*) AllocatePages(arena.totalAllocated / GetPageSize());
  Assert(arena.mem);

  return arena;
}

Arena SubArena(Arena* arena,size_t size){
  AlignArena(arena,alignof(void*));
  Byte* mem = PushBytes(arena,size);

  Arena res = {};
  res.mem = mem;
  res.totalAllocated = size;

  return res;
}

void PopToSubArena(Arena* arena,Arena subArena){
  Byte* subArenaMemPos = &subArena.mem[subArena.used];

  size_t old = arena->used;
  arena->used = subArenaMemPos - arena->mem;
  Assert(old >= arena->used);
}

void Reset(Arena* arena){
  arena->used = 0;
}

void Free(Arena* arena){
  free(arena->mem);
  arena->totalAllocated = 0;
  arena->used = 0;
}

ArenaMark MarkArena(Arena* arena){
  ArenaMark mark = {};
  mark.arena = arena;
  mark.mark = &arena->mem[arena->used];
  return mark;
}

void PopMark(ArenaMark mark){
  Arena* arena = mark.arena;
  arena->used = mark.mark - arena->mem;
}

Byte* PushBytes(Arena* arena, size_t size){
  Byte* ptr = &arena->mem[arena->used];

  if(arena->used + size > arena->totalAllocated){
    printf("[%s] Used: %zd, Size: %zd, Total: %zd\n",__PRETTY_FUNCTION__,arena->used,size,arena->totalAllocated);
    DEBUG_BREAK();
    exit(0);
  }
  
  arena->used += size;
  arena->maximum = std::max(arena->maximum,arena->used);
  
  return ptr;
}

size_t SpaceAvailable(Arena* arena){
  size_t remaining = arena->totalAllocated - arena->used;
  return remaining;
}

DynamicString StartString(Arena* arena){
  DynamicString res = {};
  res.arena = arena;
  res.mark = &arena->mem[arena->used];
  return res;
}

String EndString(DynamicString mark){
  Arena* arena = mark.arena;
  String res = {};
  res.size = &arena->mem[arena->used] - mark.mark;
  if(Empty(res)){ // Put data as NULL if size 0, probably less error prone. Not sure.
    res.data = nullptr;
  } else {
    res.data = (char*) mark.mark;
  }

  return res;
}

String PushFile(Arena* arena,FILE* file){
  String res;
  long int size = GetFileSize(file);

  AlignArena(arena,alignof(void*));

  Byte* mem = PushBytes(arena,size);
  int amountRead = fread(mem,sizeof(Byte),size,file);

  if(amountRead != size){
    fprintf(stderr,"Memory PushFile failed to read entire file\n");
    exit(-1);
  }

  res.size = size;
  res.data = (const char*) mem;

  return res;
}

String PushFile(Arena* arena,String filepath){
  return PushFile(arena,StaticFormat("%.*s",UNPACK_SS(filepath)));
}

//TODO: Replace return with Optional. Handle errors
String PushFile(Arena* arena,const char* filepath){
  FILE* file = fopen(filepath,"r");
  DEFER_CLOSE_FILE(file);
  
  if(!file){
    String res = {};
    printf("Failed to open file: %s\n",filepath);
    DEBUG_BREAK();
    res.size = -1;
    return res;
  }

  String res = PushFile(arena,file);

  return res;
}

String PushChar(Arena* arena,const char ch){
  Byte* mem = PushBytes(arena,1);

  *mem = ch;
  String res = {};
  res.data = (const char*) mem;
  res.size = 1;

  return res;
}

String PushString(Arena* arena,int size){
  Byte* mem = PushBytes(arena,size);

  String res = {};
  res.data = (const char*) mem;
  res.size = size;

  return res;
}

String PushString(Arena* arena,String ss){
  String res = PushString(arena,ss.size);
  memcpy((void*) res.data,ss.data,ss.size);
  return res;
}

String vPushString(Arena* arena,const char* format,va_list args){
  char* buffer = (char*) &arena->mem[arena->used];
  size_t maximum = arena->totalAllocated - arena->used;
  int size = vsnprintf(buffer,maximum,format,args);

  Assert(size >= 0);
  Assert(((size_t) size) < maximum);

  arena->used += (size_t) (size);

  String res = STRING(buffer,size);

  return res;
}

String PushString(Arena* arena,const char* format,...){
  va_list args;
  va_start(args,format);

  String res = vPushString(arena,format,args);

  va_end(args);

  return res;
}

void PushNullByte(Arena* arena){
  Byte* res = PushBytes(arena,1);
  *res = '\0';
}

static inline DynamicArena* GetDynamicArenaHeader(void* memoryStart,int numberPages){
  int pageSize = GetPageSize() * numberPages;

  Byte* start = (Byte*) memoryStart;
  DynamicArena* view = (DynamicArena*) (start + pageSize - sizeof(DynamicArena));

  return view;
}

void AlignArena(DynamicArena* arena,int alignment){
  int offset = arena->used % alignment;

  if(offset == 0){
    return;
  } else {
    PushBytes(arena,alignment - offset);
  }
}

DynamicArena* CreateDynamicArena(int numberPages){
  void* mem = AllocatePages(numberPages);

  DynamicArena* arena = GetDynamicArenaHeader(mem,numberPages);
  *arena = {};
  arena->mem = (Byte*) mem;
  arena->pagesAllocated = numberPages;

  return arena;
}

Arena SubArena(DynamicArena* arena,size_t size){
  Arena res = {};

  AlignArena(arena,alignof(void*));
  res.totalAllocated = size;
  res.mem = PushBytes(arena,size);

  return res;
}

Byte* PushBytes(DynamicArena* arena,size_t size){
  DynamicArena* ptr = arena;

  DynamicArena* previous = ptr;
  while(ptr->mem + ptr->used + size > (Byte*) ptr){
    if(ptr->next){
      ptr = ptr->next;
    } else {
      ptr = CreateDynamicArena((size / GetPageSize()) + 1);
      previous->next = ptr;
    }
  }

  Byte* res = ((Byte*) ptr->mem) + ptr->used;
  ptr->used += size;

  Assert(ptr->used <= (GetPageSize() * ptr->pagesAllocated - sizeof(DynamicArena)));

  return res;
}

void Clear(DynamicArena* arena){
  FOREACH_LIST(DynamicArena*,ptr,arena){
    ptr->used = 0;
  }
}

String PushString(DynamicArena* arena,String str){
  Byte* bytes = PushBytes(arena,str.size);
  memcpy((void*) bytes,str.data,str.size);
  String res = {.data = (char*) bytes,.size = str.size};
  return res;
}

bool BitIterator::operator!=(BitIterator& iter){ // Returns false if passed over iter
   bool passedOver = currentByte > iter.currentByte || (currentByte == iter.currentByte && currentBit >= iter.currentBit);

   return !passedOver;
}

void BitIterator::operator++(){
  int byteSize = (array->bitSize + 7) / 8;

  // Increment once
  currentBit += 1;
  if(currentBit >= 8){
    currentBit = 0;
    currentByte += 1;
  }

  // Stop at first valid found after
  // TODO: Maybe use the FirstBitSetIndex function, instead of this
  //       Need to change that function to maybe return bitSize if not
  while(currentByte < byteSize){
    unsigned char ch = array->memory[currentByte];

    if(!ch){
      currentByte += 1;
      currentBit = 0;
      continue;
    }

    // Fallthrough switch
    switch(currentBit){
    case 0: if(ch & 0x01) goto end; else ++currentBit;
    case 1: if(ch & 0x02) goto end; else ++currentBit;
    case 2: if(ch & 0x04) goto end; else ++currentBit;
    case 3: if(ch & 0x08) goto end; else ++currentBit;
    case 4: if(ch & 0x10) goto end; else ++currentBit;
    case 5: if(ch & 0x20) goto end; else ++currentBit;
    case 6: if(ch & 0x40) goto end; else ++currentBit;
    case 7: if(ch & 0x80) goto end; else {
      currentBit = 0;
      currentByte += 1;
    }
    }
  }

 end:

  //Assert(array->Get(*(*this)));
  return;
}

int BitIterator::operator*(){
  int index = currentByte * 8 + currentBit;
  return index;
}

void BitArray::Init(Byte* memory,int bitSize){
  this->memory = memory;
  this->bitSize = bitSize;
  this->byteSize = BitSizeToByteSize(bitSize);
  Assert(IS_ALIGNED_32(this->byteSize));
}

void BitArray::Init(Arena* arena,int bitSize){
  this->memory = MarkArena(arena).mark;
  this->bitSize = bitSize;
  this->byteSize = ALIGN_UP_32(BitSizeToByteSize(bitSize));
  PushBytes(arena,this->byteSize); // Makes it easier to use popcount
}

void BitArray::Fill(bool value){
  int fillValue = (value ? 0xff : 0x00);

  for(int i = 0; i < this->byteSize; i++){
    this->memory[i] = fillValue;
  }
}

void BitArray::Copy(BitArray array){
  Assert(this->bitSize >= array.bitSize);

  Memcpy(this->memory,array.memory,byteSize);
}

int BitArray::Get(int index){
  Assert(index < this->bitSize);

  int byteIndex = index / 8;
  int bitIndex = index % 8;
  Byte byte = memory[byteIndex];
  int result = (byte & (1 << bitIndex) ? 1 : 0);

  return result;
}

void BitArray::Set(int index,bool value){
  Assert(index < this->bitSize);

  int byteIndex = index / 8;
  int bitIndex = index % 8;

  if(value){
    memory[byteIndex] |= (1 << bitIndex);
  } else {
    memory[byteIndex] &= ~(1 << bitIndex);
  }
}

int BitArray::GetNumberBitsSet(){
  int count = 0;

#if 1
  i32* ptr = (i32*) this->memory;
  int i = 0;
  for(;i < this->bitSize; i += 32){
    count += PopCount(*ptr);
    ptr += 1;
  }
#else
  // Deal with data not aligned to 4
  int32* ptr = (int32*) this->memory;
  int i = 0;
  for(;(i + 32) < this->bitSize; i += 32){
    count += PopCount(*ptr);
    ptr += 1;
  }

  if(i < this->bitSize){
    int bytes = BitSizeToByteSize(this->bitSize - i);

    int32 test = {};
    int8* testView = &test;
    int8* ptrView = (int8*) ptr;

    switch(bytes){
    case 1: testView[0] = ptrView[0];
    case 2: testView[1] = ptrView[1];
    case 3: testView[2] = ptrView[2];
    case 4: testView[3] = ptrView[3]; // Shouldn't happen, as it means we are aligned
    }

    count += PopCount(test);
  }
#endif

  return count;
}

int BitArray::FirstBitSetIndex(){
#ifdef VERSAT_DEBUG
  int correct = 0;
  for(int i = 0; i < this->bitSize; i++){
    if(Get(i)){
      correct = i;
      break;
    }
  }
#endif
   
  u32* ptr = (u32*) this->memory;
  int i = 0;
  for(;i < this->bitSize; i += 32){
    if(*ptr != 0){
      int index = i + TrailingZerosCount(*ptr);
#ifdef VERSAT_DEBUG
      Assert(index == correct);
#endif
      return index;
    }
    ptr += 1;
  }
  Assert(false); // Not correct but good for the current needs
  return -1;
}

int BitArray::FirstBitSetIndex(int start){
  // TODO: For now I want correctness, find the bug to trailing zero.
  //#ifdef VERSAT_DEBUG
  int correct = 0;
  for(int i = start; i < this->bitSize; i++){
    if(Get(i)){
      correct = i;
      break;
    }
  }
  return correct;
//#endif

  Assert(start < this->bitSize);
  int i = ALIGN_UP_32(start - 31);
  u32 startDWord = BitSizeToDWordSize(i);
  u32* ptr = (u32*) this->memory;
  ptr = &ptr[startDWord];

  u32 val = *ptr;

  while(val != 0){
    int bitIndex = TrailingZerosCount(val);
    int index = i + bitIndex;
    if(index >= this->bitSize){
      Assert(false); // Not correct but good for the current needs
      return -1;
    }
    if(index >= start){
#ifdef VERSAT_DEBUG
      Assert(index == correct);
#endif
      return index;
    }

    val = CLEAR_BIT(val,bitIndex);
  }

  i += 32;
  ptr += 1;
  for(;i < this->bitSize; i += 32){
    if(*ptr != 0){
      int index = i + TrailingZerosCount(*ptr);
#ifdef VERSAT_DEBUG
      Assert(index == correct);
#endif
      return index;
    }
    ptr += 1;
  }
  Assert(false); // Not correct but good for the current needs
  return -1;
}

String BitArray::PrintRepresentation(Arena* output){
  auto mark = StartString(output);
  for(int i =  0; i < this->bitSize; i++){
    int val = Get(i);

    if(val){
      PushString(output,STRING("1"));
    } else {
      PushString(output,STRING("0"));
    }
  }
  String res = EndString(mark);
  return res;
}

void BitArray::operator&=(BitArray& other){
  Assert(this->bitSize == other.bitSize);

  for(int i = 0; i < byteSize; i++){
    this->memory[i] &= other.memory[i];
  }
}

BitIterator BitArray::begin(){
  BitIterator iter = {};
  iter.array = this;

  if(bitSize == 0){
    return iter;
  }

  if(!Get(0)){
    ++iter;
  }

  return iter;
}

BitIterator BitArray::end(){
  BitIterator iter = {};
  iter.array = this;
  iter.currentByte = bitSize/ 8;
  iter.currentBit = bitSize % 8;
  return iter;
}

GenericPoolIterator IteratePool(void* pool,int sizeOfType,int alignmentOfType){
  GenericPoolIterator iter = {};
  if(pool == nullptr){
    return iter;
  }
  
  Byte* mem = *(Byte**) pool;
  iter.Init(mem,sizeOfType);

  return iter;
}

bool HasNext(GenericPoolIterator iter){
  return iter.HasNext();
}

void* Next(GenericPoolIterator& iter){
  void* res = *iter;
  ++iter;
  return res;
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

void GenericPoolIterator::Init(Byte* page,int elemSize){
  fullIndex = 0;
  bit = 7;
  index = 0;
  this->elemSize = elemSize;
  this->page = page;

  poolInfo = CalculatePoolInfo(elemSize);
  pageInfo = GetPageInfo(poolInfo,page);

  if(page && !(pageInfo.bitmap[index] & (1 << bit))){
    ++(*this);
  }
}

bool GenericPoolIterator::HasNext(){
  if(page){
    return true;
  }

  return false;
}

void GenericPoolIterator::operator++(){
  while(1){
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
}

Byte* GenericPoolIterator::operator*(){
  Assert(page != nullptr);

  Byte* view = (Byte*) page;
  Byte* val = &view[(index * 8 + (7 - bit)) * elemSize];

  Assert(pageInfo.bitmap[index] & (1 << bit));

  return val;
}

// Dynamic String

void DynamicString::PushChar(const char ch){
  Byte* mem = PushBytes(arena,1);

  *mem = ch;
  String res = {};
  res.data = (const char*) mem;
  res.size = 1;
}

void DynamicString::PushString(String ss){
  String res = ::PushString(this->arena,ss.size);
  memcpy((void*) res.data,ss.data,ss.size);
}

void DynamicString::vPushString(const char* format,va_list args){
  char* buffer = (char*) &arena->mem[arena->used];
  size_t maximum = arena->totalAllocated - arena->used;
  int size = vsnprintf(buffer,maximum,format,args);

  Assert(size >= 0);
  Assert(((size_t) size) < maximum);

  arena->used += (size_t) (size);

  String res = STRING(buffer,size);
}

void DynamicString::PushString(const char* format,...){
  va_list args;
  va_start(args,format);

  vPushString(format,args);

  va_end(args);
}

void DynamicString::PushNullByte(){
  Byte* res = PushBytes(arena,1);
  *res = '\0';
}
