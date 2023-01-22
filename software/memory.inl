#ifndef INCLUDED_MEMORY_INCLUDE
#define INCLUDED_MEMORY_INCLUDE

// Impl
template<typename T>
bool ZeroOutAlloc(Allocation<T>* alloc,int newSize){
   if(newSize <= 0){
      return false;
   }

   bool didRealloc = false;
   if(newSize > alloc->reserved){
      T* tmp = (T*) realloc(alloc->ptr,newSize * sizeof(T));

      Assert(tmp);

      alloc->ptr = tmp;
      alloc->reserved = newSize;
      didRealloc = true;
   }

   alloc->size = newSize;
   memset(alloc->ptr,0,alloc->reserved * sizeof(T));
   return didRealloc;
}

template<typename T>
bool ZeroOutRealloc(Allocation<T>* alloc,int newSize){
   if(newSize <= 0){
      return false;
   }

   bool didRealloc = false;
   if(newSize > alloc->reserved){
      T* tmp = (T*) realloc(alloc->ptr,newSize * sizeof(T));

      Assert(tmp);

      alloc->ptr = tmp;
      alloc->reserved = newSize;
      didRealloc = true;
   }

   // Clear out free (allocated now or before) space
   if(didRealloc){
      char* view = (char*) alloc->ptr;
      memset(&view[alloc->size],0,(alloc->reserved - alloc->size) * sizeof(T));
   }

   alloc->size = newSize;
   return didRealloc;
}

template<typename T>
void Reserve(Allocation<T>* alloc,int reservedSize){
   alloc->ptr = (T*) calloc(reservedSize,sizeof(T));
   alloc->reserved = reservedSize;
}

template<typename T>
void Alloc(Allocation<T>* alloc,int newSize){
   if(alloc->ptr != nullptr){
      LogOnce(LogModule::MEMORY,LogLevel::WARN,"Allocating memory without previously freeing");
   }

   alloc->ptr = (T*) malloc(sizeof(T) * newSize);
   alloc->reserved = newSize;
   alloc->size = newSize;
}

template<typename T>
void RemoveChunkAndCompress(Allocation<T>* alloc,T* ptr,int size){
   Assert(Inside(alloc,ptr));

   T* copyStart = ptr + size;
   int copySize = alloc->size - (copyStart - alloc->ptr);

   Memcpy(ptr,copyStart,copySize);
   alloc->size -= size;
}

template<typename T>
bool Inside(Allocation<T>* alloc,T* ptr){
   bool res = (ptr >= alloc->ptr && ptr < (alloc->ptr + alloc->size));
   return res;
}

template<typename T>
void Free(Allocation<T>* alloc){
   free(alloc->ptr);
   alloc->ptr = nullptr;
   alloc->reserved = 0;
   alloc->size = 0;
}

template<typename T>
int MemoryUsage(Allocation<T> alloc){
   int memoryUsed = alloc.reserved * sizeof(T);
   return memoryUsed;
}

template<typename Key,typename Data>
bool HashmapIterator<Key,Data>::operator!=(HashmapIterator& iter){
   bool res = (iter.bitIter != this->bitIter);
   return res;
}

template<typename Key,typename Data>
void HashmapIterator<Key,Data>::operator++(){
   ++this->bitIter;
}

template<typename Key,typename Data>
Pair<Key,Data>& HashmapIterator<Key,Data>::operator*(){
   int index = *this->bitIter;

   auto& pair = map->memory[index];

   return pair;
}

template<typename Key,typename Data>
struct Node{
   Pair<Key,Data> data;
   Node* next;
};

template<typename Key,typename Data>
struct HashmapHeader{
   int nodesAllocated;
   int nodesUsed;
   Array<Node<Key,Data>*> buckets;
   Array<Node<Key,Data>> data;
};

template<typename Key,typename Data>
void Hashmap<Key,Data>::Init(Arena* arena,int maxAmountOfElements){
   if(maxAmountOfElements > 0){
      int size = AlignNextPower2(maxAmountOfElements) * 2;
      Assert(IsPowerOf2(size) && size > 1);

      memory = PushArray<Pair<Key,Data>>(arena,size);

      Memset(memory,Pair<Key,Data>{});
      valid.Init(arena,size);
      valid.Fill(0);
      initialized = true;
   }

   int size = AlignNextPower2(maxAmountOfElements) * 2;

   this->mem = (Byte*) PushStruct<HashmapHeader<Key,Data>>(arena);
   HashmapHeader<Key,Data>* header = (HashmapHeader<Key,Data>*) this->mem;

   header->nodesAllocated = size;
   header->nodesUsed = 0;
   header->buckets = PushArray<Node<Key,Data>*>(arena,size);
   Memset<Node<Key,Data>*>(header->buckets,nullptr);
   header->data = PushArray<Node<Key,Data>>(arena,size);
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::Insert(Key key,Data data){
   Assert(initialized);
   int mask = memory.size - 1;
   int index = Hash<Key>(key) & mask; // Size is power of 2

   HashmapHeader<Key,Data>* header = (HashmapHeader<Key,Data>*) this->mem;
   Node<Key,Data>* ptr = header->buckets[index];

   // Do not even need to look
   if(ptr == nullptr){
      Assert(header->nodesUsed < header->nodesAllocated);
      Node<Key,Data>* node = &header->data[header->nodesUsed++];

      node->data.key = key;
      node->data.data = data;
      node->next = nullptr;

      header->buckets[index] = node;

      return &node->data.data;
   } else {
      for(; ptr; ptr = ptr->next){
         if(ptr->data.key == key){ // Same key
            ptr->data.data = data; // No duplicated keys, overwrite data
            return &ptr->data.data;
         }
      }

      Assert(header->nodesUsed < header->nodesAllocated);
      Node<Key,Data>* newNode = &header->data[header->nodesUsed++];
      newNode->data.key = key;
      newNode->data.data = data;
      newNode->next = header->buckets[index];

      header->buckets[index] = newNode;

      return &newNode->data.data;
   }

   return nullptr;
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::InsertIfNotExist(Key key,Data data){
   Assert(initialized);
   Data* ptr = Get(key);

   if(ptr == nullptr){
      return Insert(key,data);
   }

   return nullptr;
}

template<typename Key,typename Data>
bool Hashmap<Key,Data>::Exists(Key key){
   if(!initialized){
      return false;
   }

   Data* ptr = Get(key);

   if(ptr == nullptr){
      return false;
   }
   return true;
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::Get(Key key){
   if(!initialized){
      return nullptr;
   }

   int mask = memory.size - 1;
   int index = Hash<Key>(key) & mask; // Size is power of 2

   HashmapHeader<Key,Data>* header = (HashmapHeader<Key,Data>*) this->mem;
   Node<Key,Data>* ptr = header->buckets[index];
   for(; ptr; ptr = ptr->next){
      if(ptr->data.key == key){ // Same key
         return &ptr->data.data;
      }
   }

   return nullptr;
}

template<typename Key,typename Data>
Data Hashmap<Key,Data>::GetOrFail(Key key){
   Assert(initialized);
   Data* ptr = Get(key);
   Assert(ptr);
   return *ptr;
}

template<typename Key,typename Data>
HashmapIterator<Key,Data> Hashmap<Key,Data>::begin(){
   HashmapIterator<Key,Data> iter;

   iter.map = this;
   iter.bitIter = this->valid.begin();

   return iter;
}

template<typename Key,typename Data>
HashmapIterator<Key,Data> Hashmap<Key,Data>::end(){
   HashmapIterator<Key,Data> iter;

   iter.map = this;
   iter.bitIter = this->valid.end();

   return iter;
}

template<typename T>
void PoolIterator<T>::Init(Pool<T>* pool,Byte* page){
   *this = {};

   this->pool = pool;
   this->page = page;
   this->bit = 7;

   if(page){
      pageInfo = GetPageInfo(pool->info,page);

      if(!IsValid()){
         ++(*this);
      }
   }
}

template<typename T>
bool PoolIterator<T>::operator!=(PoolIterator<T>& iter){
   bool res = this->page != iter.page; // We only care about for ranges, so no need to be specific

   return res;
}

template<typename T>
void PoolIterator<T>::Advance(){
   PoolInfo& info = pool->info;

   fullIndex += 1;
   bit -= 1;
   if(bit < 0){
      index += 1;
      bit = 7;
   }

   if(index * 8 + (7 - bit) >= info.unitsPerPage){
      index = 0;
      bit = 7;
      page = pageInfo.header->nextPage;
      if(page != nullptr){
         pageInfo = GetPageInfo(info,page);
      }
   }
}

template<typename T>
bool PoolIterator<T>::IsValid(){
   bool res = pageInfo.bitmap[index] & (1 << bit);

   return res;
}

template<typename T>
void PoolIterator<T>::operator++(){
   while(page){
      Advance();

      if(IsValid()){
         break;
      }
   }
}

template<typename T>
T* PoolIterator<T>::operator*(){
   Assert(page != nullptr);

   T* view = (T*) page;
   T* val = &view[index * 8 + (7 - bit)];

   return val;
}

#if 0
template<typename T>
Pool<T>::Pool()
:mem(nullptr)
,allocated(0)
,endSize(0)
{

}

template<typename T>
Pool<T>::~Pool(){
   Byte* ptr = mem;
   while(ptr){
      PageInfo page = GetPageInfo(info,ptr);

      Byte* nextPage = page.header->nextPage;

      DeallocatePages(ptr,1);

      ptr = nextPage;
   }
}
#endif

template<typename T>
T* Pool<T>::Alloc(){
   static int bitmask[] = {0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

   if(!mem){
      mem = (Byte*) AllocatePages(1);
      info = CalculatePoolInfo(sizeof(T));
   }

   int fullIndex = 0;
   Byte* ptr = mem;
   PageInfo page = GetPageInfo(info,ptr);
   while(page.header->allocatedUnits == info.unitsPerPage){
      if(!page.header->nextPage){
         page.header->nextPage = (Byte*) AllocatePages(1);
      }

      ptr = page.header->nextPage;
      page = GetPageInfo(info,ptr);
      fullIndex += info.unitsPerPage;
   }

   T* view = (T*) ptr;
   for(int index = 0; index * 8 < info.unitsPerPage; index += 1){
      char val = page.bitmap[index];

      if(info.unitsPerPage - index * 8 < 8){
         if(val == bitmask[(info.unitsPerPage - 1) % 8]){
            continue;
         }
      } else {
         if(val == 0xff){
            continue;
         }
      }

      for(int i = 7; i >= 0; i--){
         if(!(val & (1 << i))){ // empty location
            page.bitmap[index] |= (1 << i);

            page.header->allocatedUnits += 1;

            fullIndex += index * 8 + (7 - i);

            T* inst = new (&view[index * 8 + (7 - i)]) T(); // Needed for anything stl based

            return inst;
         }
      }
   }

   Assert(0);
   return nullptr;
}

template<typename T>
T* Pool<T>::Alloc(int index){
   if(!mem){
      mem = (Byte*) AllocatePages(1);
      info = CalculatePoolInfo(sizeof(T));
   }

   Byte* ptr = mem;
   PageInfo page = GetPageInfo(info,ptr);
   while(index >= info.unitsPerPage){
      if(!page.header->nextPage){
         page.header->nextPage = (Byte*) AllocatePages(1);
      }

      ptr = page.header->nextPage;
      page = GetPageInfo(info,ptr);
      index -= info.unitsPerPage;
   }

   int bitmapIndex = index / 8;
   int bitIndex = (7 - index % 8);

   if(page.bitmap[bitmapIndex] & (1 << bitIndex)){
      return nullptr;
   }

   T* view = (T*) ptr;
   page.bitmap[bitmapIndex] |= (1 << bitIndex);

   return &view[bitmapIndex * 8 + (7 - bitIndex)];
}

template<typename T>
T* Pool<T>::Get(int index){
   if(!mem){
      return nullptr;
   }

   Byte* ptr = mem;
   PageInfo page = GetPageInfo(info,ptr);
   while(index >= page.header->allocatedUnits){
      if(!page.header->nextPage){
         return nullptr;
      }

      ptr = page.header->nextPage;
      page = GetPageInfo(info,ptr);
      index -= info.unitsPerPage;
   }

   int bitmapIndex = index / 8;
   int bitIndex = (7 - index % 8);

   if(!(page.bitmap[bitmapIndex] & (1 << bitIndex))){
      return nullptr;
   }

   T* view = (T*) ptr;

   return &view[bitmapIndex * 8 + (7 - bitIndex)];
}

template<typename T>
T& Pool<T>::GetOrFail(int index){
   T* res = Get(index);

   Assert(res);

   return *res;
}

template<typename T>
void Pool<T>::Remove(T* elem){
   Byte* page = (Byte*) ((uintptr_t)elem & ~(GetPageSize() - 1));
   PageInfo pageInfo = GetPageInfo(info,page);

   int pageIndex = ((Byte*) elem - page) / sizeof(T);
   int bitmapIndex = pageIndex / 8;
   int bit = 7 - (pageIndex % 8);

   pageInfo.bitmap[bitmapIndex] &= ~(1 << bit);
   pageInfo.header->allocatedUnits -= 1;
}

template<typename T>
Byte* Pool<T>::GetMemoryPtr(){
   return mem;
}

template<typename T>
Pool<T> Pool<T>::Copy(){
   Pool<T> other = {};

   for(T* ptr : *this){
      T* inst = other.Alloc();
      *inst = *ptr;
   }

   return other;
}

template<typename T>
int Pool<T>::Size(){
   int count = 0;

   Byte* page = mem;
   while(page){
      PageInfo pageInfo = GetPageInfo(info,page);
      count += pageInfo.header->allocatedUnits;
      page = pageInfo.header->nextPage;
   }

   return count;
}

template<typename T>
void Pool<T>::Clear(bool freeMemory){
   if(freeMemory){
      Byte* page = mem;
      while(page){
         PageInfo pageInfo = GetPageInfo(info,page);
         Byte* next = pageInfo.header->nextPage;

         DeallocatePages(page,1);
         page = next;
      }
      mem = nullptr;
   } else {
      Byte* page = mem;
      while(page){
         PageInfo pageInfo = GetPageInfo(info,page);

         memset(pageInfo.bitmap,0,info.bitmapSize);
         pageInfo.header->allocatedUnits = 0;

         page = pageInfo.header->nextPage;
      }
   }
}

template<typename T>
PoolIterator<T> Pool<T>::begin(){
   PoolIterator<T> iter = {};
   iter.Init(this,mem);

   return iter;
}

template<typename T>
PoolIterator<T> Pool<T>::end(){
   PoolIterator<T> iter = {};
   iter.Init(this,nullptr);

   return iter;
}

template<typename T>
PoolIterator<T> Pool<T>::beginNonValid(){
   PoolIterator<T> iter = {};
   iter.Init(this,mem);

   return iter;
}

template<typename T>
int Pool<T>::MemoryUsage(){
   Byte* page = mem;
   int numberPages = 0;
   while(page){
      numberPages += 1;
      PageInfo pageInfo = GetPageInfo(info,page);
      Byte* next = pageInfo.header->nextPage;

      page = next;
   }

   int memoryUsed = numberPages * GetPageSize();
   return memoryUsed;
}

#endif // INCLUDED_MEMORY_INCLUDE
