#include "memory.hpp"

#include <unistd.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#include <cstdarg>

#include "intrinsics.hpp"

#if defined(__SANITIZE_ADDRESS__)
#pragma message "Using address sanitizer"
const char* __asan_default_options() { return "detect_leaks=0"; }
#endif

// Arena Usage recording and reporting. Mostly some statistics to figure out how much memory our program is used and if some function is doing something odd

//static Arena debugArena; 

struct ArenaInfo{
  Arena* arena;
  size_t used;
};
  
struct FunctionPos{
  const char* file;
  const char* functionName;
  int line;
};

struct FunctionAllocationInfo{
  FunctionPos pos;
  size_t memoryUsed;
};

static Arena debugMemoryArena;
static Array<ArenaInfo> debugArenaStack;
static int debugArenaIndex;


//static ArenaList<FunctionAllocationInfo>* debugInfo;

static void InitMemoryDebug(){
#if 0
  static bool init = false;
  if(init){
    return;
  }
  init = true;

  debugMemoryArena = InitArena(Megabyte(16));
  debugArenaStack = PushArray<ArenaInfo>(&debugMemoryArena,100);
  debugArenaIndex = 0;

  debugInfo = PushList<FunctionAllocationInfo>(&debugMemoryArena);
#endif
}

void ReportArenaUsage(){
#if 0
  Array<FunctionAllocationInfo> asArray = PushArray(&debugMemoryArena,debugInfo);

  auto CompareFunction = [](const void* f1,const void* f2) -> int{
    FunctionAllocationInfo* v1 = (FunctionAllocationInfo*) f1;
    FunctionAllocationInfo* v2 = (FunctionAllocationInfo*) f2;

    if(v2->memoryUsed > v1->memoryUsed){
      return 1;
    } else if(v2->memoryUsed < v1->memoryUsed){
      return -1;
    } else {
      return 0;
    }
  };
  
  qsort(asArray.data,asArray.size,sizeof(FunctionAllocationInfo),CompareFunction);

  asArray.size = MIN(asArray.size,10);
  
  for(FunctionAllocationInfo info : asArray){
    String prettyMemoryStr = ReprMemorySize(info.memoryUsed,&debugMemoryArena); // NOTE: Leaking
    printf("%s:%s:%d : %.*s\n",info.pos.file,info.pos.functionName,info.pos.line,UN(prettyMemoryStr));
  }
#endif
}

void DebugInitRegion(Arena* arena,const char* file,const char* function,int line){
#if 0
#ifndef VERSAT_DEBUG
  return;
#endif

  InitMemoryDebug();
  
  debugArenaStack[debugArenaIndex].arena = arena;
  debugArenaStack[debugArenaIndex].used = arena->used;

  debugArenaIndex += 1;
#endif
}

void DebugEndRegion(Arena* arena,const char* file,const char* function,int line){
#if 0
#ifndef VERSAT_DEBUG
  return;
#endif

  Assert(debugArenaIndex > 0);
  debugArenaIndex -= 1;

  ArenaInfo startInfo = debugArenaStack[debugArenaIndex];

  size_t memoryUsed = arena->used - startInfo.used;

  // NOTE: We currently do not store any small usages, otherwise we could easily overflow our debug arena
  if(memoryUsed > 1024){
    FunctionAllocationInfo* info = debugInfo->PushElem();
    info->pos.file = file;
    info->pos.functionName = function;
    info->pos.line = line;
    info->memoryUsed = memoryUsed;
  }
#endif
}

Arena* contextArenas[2];
Arena singleUseCasesArenas[8];

u8 singleUseArenaBeingUsed = 0;

Arena* GetArena(Arena* diff){
  if(contextArenas[0] != diff){
    return contextArenas[0];
  }

  Assert(contextArenas[1] != diff);
  return contextArenas[1];
}

Arena* GetArena2(Arena* diff,Arena* diff2){
  if(diff == diff2){
    return GetArena(diff);
  }
  if(diff == nullptr){
    return GetArena(diff2);
  }
  if(diff2 == nullptr){
    return GetArena(diff);
  }

  // At this point diff and diff2 must be different and non null
  if(contextArenas[0] != diff && contextArenas[0] != diff2){
    return contextArenas[0];
  }

  if(contextArenas[1] != diff && contextArenas[1] != diff2){
    return contextArenas[1];
  }

  printf("Ran out of temporary arenas\n");
  DEBUG_BREAK();
  return nullptr;
}

Arena* GetSingleUseArena(){
  for(int i = 0; i < 8; i += 1){
    int index = (1 << i);
    if(singleUseArenaBeingUsed & index){
      continue;
    }
    
    singleUseArenaBeingUsed |= index;
    return &singleUseCasesArenas[i];
  }

  printf("Ran out of single use arenas\n");
  DEBUG_BREAK();
  return nullptr;
}

void FreeSingleUseArena(Arena* arena){
  arena->used = 0;

  int index = (arena - singleUseCasesArenas);

  Assert(index >= 0 && index <= 8);
  singleUseArenaBeingUsed &= ~(1 << index);
}

int GetPageSize(){
  static int pageSize = 0;

  if(pageSize == 0){
    pageSize = getpagesize();
  }

  return pageSize;
}

int AlignToNextPage(int amount){
  int pages = ((amount - 1) / GetPageSize()) + 1;
  int aligned = pages * GetPageSize();
  return aligned;
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

void AlignArena(Arena* arena,int alignment){
  int offset = arena->used % alignment;

  if(offset == 0){
    return;
  } else {
    PushBytes(arena,alignment - offset);
  }
}

Arena InitArena_(size_t size,const char* file,int line){
  Arena arena = {};

  arena.used = 0;
  arena.totalAllocated = size;
  arena.mem = (Byte*) calloc(size,sizeof(Byte));
  arena.fileCreationPlace = file;
  arena.lineCreationPlace = line;

  if(arena.mem == nullptr){
    fprintf(stderr,"Error allocating memory. Make sure enough memory is available\n");
    exit(1);
  }

  Assert(IS_ALIGNED_64(arena.mem));

  ASAN_POISON_MEMORY_REGION(arena.mem,arena.totalAllocated);
  
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

void Reset(Arena* arena){
  arena->used = 0;

  ASAN_POISON_MEMORY_REGION(arena->mem,arena->totalAllocated);
}

void Free(Arena* arena){
  ASAN_UNPOISON_MEMORY_REGION(arena->mem,arena->totalAllocated);

  free(arena->mem);
  arena->totalAllocated = 0;
  arena->used = 0;
}

size_t MemoryUsage(Arena* arena){
  return arena->used;
}

ArenaMark MarkArena(Arena* arena){
  ArenaMark mark = {};
  mark.arena = arena;
  mark.mark = &arena->mem[arena->used];
  return mark;
}

void PopMark(ArenaMark mark){
  Arena* arena = mark.arena;

  Assert(!(arena->flags & (u8) ArenaFlags_NO_POP));

  int biggerUsed = arena->used;
  arena->used = mark.mark - arena->mem;
  
  ASAN_POISON_MEMORY_REGION(&arena->mem[arena->used],biggerUsed - arena->used);
}

Byte* PushBytes(Arena* arena, size_t size){
  Byte* ptr = &arena->mem[arena->used];

  if(arena->used + size > arena->totalAllocated){
    printf("[%s] Used: %zd, Size: %zd, Total: %zd\n",__PRETTY_FUNCTION__,arena->used,size,arena->totalAllocated);
    NOT_IMPLEMENTED("Need to change arena to linked list approach");
    exit(0);
  }
  
  arena->used += size;
  arena->maximum = MAX(arena->maximum,arena->used);

  ASAN_UNPOISON_MEMORY_REGION(ptr,size);
  
  return ptr;
}

size_t SpaceAvailable(Arena* arena){
  size_t remaining = arena->totalAllocated - arena->used;
  return remaining;
}

StringBuilder* StartString(Arena* out){
  StringBuilder* builder = PushStruct<StringBuilder>(out);
  builder->arena = out;

  // Simpler to preallocate one node, very rare for a StringBuilder to be started and not used once
  builder->head = PushStruct<StringNode>(out);
  builder->tail = builder->head;
  
  return builder;
}

void StringBuilder::PushString(String string){
  StringNode* ptr = this->tail;

  if(debugPrint){
    printf("%.*s",UN(string));
  }
  
  String str = string;
  while(1){
    i16 toCopyAmount = MIN((long unsigned) str.size,(STRING_NODE_SIZE - ptr->used));
    memcpy(&ptr->buffer[ptr->used],str.data,toCopyAmount);

    str.data += toCopyAmount;
    str.size -= toCopyAmount;
    ptr->used += toCopyAmount;
    
    if(ptr->used == STRING_NODE_SIZE){
      ptr->next = PushStruct<StringNode>(arena);
      ptr = ptr->next;
    }

    if(str.size == 0){
      break;
    }
  }

  this->tail = ptr;
}

// TODO: Performance will drop if this gets called repeatedly and in contexts where we are constantly pushing chars
//       Need to start tracking that sort of stuff in debug mode.
void StringBuilder::PushChar(char ch){
  String str = {};
  str.data = &ch;
  str.size = 1;
  PushString(str);
}

void StringBuilder::vPushString(const char* format,va_list args){
  TEMP_REGION(temp,arena);
  String toPush = ::vPushString(temp,format,args);
  PushString(toPush);
}

void StringBuilder::PushSpaces(int amount){
  for(int i = 0; i < amount; i++){
    this->PushChar(' ');
  }
}

void StringBuilder::PushString(const char* format,...){
  va_list args;
  va_start(args,format);

  vPushString(format,args);

  va_end(args);
}

String EndString(Arena* out,StringBuilder* builder){
  int totalSize = 0;
  for(StringNode* ptr = builder->head; ptr != nullptr; ptr = ptr->next){
    totalSize += ptr->used;
  }

  Byte* data = PushBytes(out,totalSize + 1); // Null byte appended

  String res = {};
  res.data = (const char*) data;
  res.size = totalSize;
  
  for(StringNode* ptr = builder->head; ptr != nullptr; ptr = ptr->next){
    memcpy(data,ptr->buffer,ptr->used);
    data += ptr->used;
  }

  *data = '\0';
  
  return res;
}

String PushFile(Arena* out,FILE* file){
  String res;
  long int size = GetFileSize(file);

  AlignArena(out,alignof(void*));

  Byte* mem = PushBytes(out,size);
  int amountRead = fread(mem,sizeof(Byte),size,file);

  if(amountRead != size){
    fprintf(stderr,"Memory PushFile failed to read entire file\n");
    exit(-1);
  }

  res.size = size;
  res.data = (const char*) mem;

  return res;
}

String PushFile(Arena* out,String filepath){
  return PushFile(out,StaticFormat("%.*s",UN(filepath)));
}

//TODO: Replace return with Optional. Handle errors
String PushFile(Arena* out,const char* filepath){
  FILE* file = OpenFile(filepath,"r",FilePurpose_READ_CONTENT);
  DEFER_CLOSE_FILE(file);
  
  if(!file){
    String res = {};
    printf("Failed to open file: %s\n",filepath);
    NOT_IMPLEMENTED("Need to return opt and let code handle this instead. TODO");
    res.size = -1;
    return res;
  }

  String res = PushFile(out,file);

  return res;
}

String PushString(Arena* arena,String ss){
  int size = ss.size;
  
  Byte* mem = PushBytes(arena,size + 1);

  String res = {};
  res.data = (const char*) mem;
  res.size = size;

  memcpy((void*) res.data,ss.data,ss.size);
  mem[size] = '\0';

  return res;
}

String vPushString(Arena* arena,const char* format,va_list args){
  int extraBuffer; // Just to make sure that vsnprintf with 0 size does not cause problems with a nullptr we pass it just a bit of memory.
  
  va_list copy;
  va_copy(copy,args);

  int stringSizeWithTerm = vsnprintf((char*) &extraBuffer,0,format,copy);
  char* buffer = (char*) PushBytes(arena,stringSizeWithTerm + 1);

  int size = vsnprintf(buffer,stringSizeWithTerm + 1,format,args);

  Assert(size == stringSizeWithTerm);

  String res = String(buffer,size);

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
  String res = String((char*) bytes,str.size);
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

  int minAmount = 8; // Should it be variable? Probably need more time using Pool to check if we do need to do anything extra here.

  // Because the bitmap depends on the amount of units that exist, we first calculate a minimum size then calculate the amount of units that we can actually fit into that size.
  auto CalculateBlockSize = [elemSize](int amountOfUnits){
    int size = AlignToNextPage(amountOfUnits * elemSize + sizeof(PoolHeader) + RoundUpDiv(amountOfUnits,8));
    return size;
  };

  int minBlockSize = CalculateBlockSize(minAmount);
  int minPagesPerBlock = minBlockSize / GetPageSize();

  // TODO: Stupid way of calculating true size for the given minimum.
  int amount = minAmount + 1;
  for(; true ; amount += 1){
    int blockSize = CalculateBlockSize(amount);
    int pagesPerBlock = blockSize / GetPageSize();

    if(pagesPerBlock != minPagesPerBlock){
      amount -= 1;
      break;
    }
  }

  info.unitsPerPageBlock = amount;
  info.bitmapSize = RoundUpDiv(amount,8);
  info.pagesPerBlock = CalculateBlockSize(amount) / GetPageSize();

  return info;
}

PageInfo GetPageInfo(PoolInfo poolInfo,Byte* page){
  PageInfo info = {};

  info.header = (PoolHeader*) (page + poolInfo.pagesPerBlock * GetPageSize() - sizeof(PoolHeader));
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

    if(index * 8 + (7 - bit) >= poolInfo.unitsPerPageBlock){
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

GenericArrayIterator IterateArray(void* array,int sizeOfType,int alignmentOfType){
  GenericArrayIterator res = {};
  res.array = array;
  res.sizeOfType = sizeOfType;
  res.alignmentOfType = alignmentOfType;

  return res;
}

bool HasNext(GenericArrayIterator iter){
  void** arrayData = (void**) iter.array;
  int* size = (int*) (arrayData + 1);

  return iter.index < *size;
}

void* Next(GenericArrayIterator& iter){
  void** arrayData = (void**) iter.array;
  void* arrayStart = (void*) arrayData[0]; 

  char* view = (char*) arrayStart;
  void* data = (void*) &view[iter.index * iter.sizeOfType];
  iter.index += 1;
  
  return data;
}

GenericTrieMapIterator IterateTrieMap(void* trieMap,int sizeOfType,int alignmentOfType){
  GenericTrieMapIterator res = {};
  res.trieMap = trieMap;
  res.sizeOfType = sizeOfType;
  res.alignmentOfType = alignmentOfType;

  TrieMap<int,int>* view = ((TrieMap<int,int>*) trieMap);

  res.ptr = view->head;
  
  return res;
}

bool HasNext(GenericTrieMapIterator iter){
  TrieMap<int,int>* view = ((TrieMap<int,int>*) iter.trieMap);

  return iter.index < view->inserted;
}

void* Next(GenericTrieMapIterator& iter){
  TrieMapNode<int,int>* ptr = (TrieMapNode<int,int>*) iter.ptr;

  void* data = ((char*)ptr) + sizeof(void*) * 4;
  TrieMapNode<int,int>* next = *(TrieMapNode<int,int>**) (((char*)data) + iter.sizeOfType);
  iter.ptr = next;
  
  iter.index += 1;

  return data;
}

GenericHashmapIterator IterateHashmap(void* hashmap,int sizeOfType,int alignmentOfType){
  GenericHashmapIterator res = {};
  res.hashmap = hashmap;
  res.sizeOfType = sizeOfType;
  res.alignmentOfType = alignmentOfType;

  return res;
}

bool HasNext(GenericHashmapIterator iter){
  Hashmap<int,int>* view = ((Hashmap<int,int>*) iter.hashmap);

  return iter.index < view->nodesUsed;
}

void* Next(GenericHashmapIterator& iter){
  Hashmap<int,int>* view = ((Hashmap<int,int>*) iter.hashmap);

  char* arrayView = (char*) view->data;
  void* data = (void*) &arrayView[iter.index * iter.sizeOfType];
  iter.index += 1;
  return data;
}
