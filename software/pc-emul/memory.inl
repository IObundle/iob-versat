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
   if(alloc->reserved - alloc->size > 0){
      char* view = (char*) alloc->ptr;
      memset(&view[alloc->size],0,(alloc->reserved - alloc->size) * sizeof(T));
   }

   alloc->size = newSize;
   return didRealloc;
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

#if 0
template<typename Key,typename Data>
Hashmap<Key,Data>::Hashmap(Arena* arena,int maxAmountOfElements){
   Init(arena,maxAmountOfElements);
}
#endif

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
}

template<typename Key,typename Data>
Data* Hashmap<Key,Data>::Insert(Key key,Data data){
   Assert(initialized);
   int mask = memory.size - 1;
   int index = Hash<Key>(key) & mask; // Size is power of 2

   // open addressing, find first empty position
   for(; 1; index = (index + 1) & mask){
      if(!valid.Get(index)){
         break;
      }
   }

   valid.Set(index,1);
   memory[index].key = key;
   memory[index].data = data;
   inserted += 1;

   return &memory[index].data;
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

   for(; 1; index = (index + 1) & mask){
      if(!valid.Get(index)){ // Since using open addressing, the first sign of a non valid position means key doesn't exist.
         return nullptr;
      }

      if(key == memory[index].key){
         return &memory[index].data;
      }
   }
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
PoolIterator<T>::PoolIterator()
:pool(nullptr)
,fullIndex(0)
,bit(7)
,index(0)
,lastVal(nullptr)
{
}

template<typename T>
void PoolIterator<T>::SetPool(Pool<T>* pool){
   this->pool = pool;

   page = pool->mem;

   if(page && pool->allocated){
      pageInfo = GetPageInfo(pool->info,page);

//      if(!IsValid()){
//         ++(*this);
//      }
   }
}

template<typename T>
bool PoolIterator<T>::operator!=(PoolIterator<T>& iter){
   Assert(this->pool == iter.pool);

   if(this->fullIndex < pool->endSize){ // Kinda of a hack, for now
      return true;
   }

   return false;
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
   while(fullIndex < pool->endSize){
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

   //Assert(pageInfo.bitmap[index] & (1 << bit));

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
            allocated += 1;

            fullIndex += index * 8 + (7 - i);

            if(fullIndex + 1 > endSize){
               endSize = fullIndex + 1;
            }

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
   allocated += 1;
   endSize = std::max(index + 1,endSize);

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
void Pool<T>::Remove(T* elem){
   Byte* page = (Byte*) ((uint)elem & ~(GetPageSize() - 1));
   PageInfo pageInfo = GetPageInfo(info,page);

   int pageIndex = ((Byte*) elem - page) / sizeof(T);
   int bitmapIndex = pageIndex / 8;
   int bit = 7 - (pageIndex % 8);

   pageInfo.bitmap[bitmapIndex] &= ~(1 << bit);
   pageInfo.header->allocatedUnits -= 1;

   allocated -= 1;
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

   endSize = 0;
   allocated = 0;
}

template<typename T>
PoolIterator<T> Pool<T>::begin(){
   if(!mem || allocated == 0){
      return end();
   }

   PoolIterator<T> iter = {};

   iter.SetPool(this);

   if(!iter.IsValid()){
      ++iter;
   }

   return iter;
}

template<typename T>
PoolIterator<T> Pool<T>::end(){
   PoolIterator<T> iter = {};

   iter.SetPool(this);

   iter.fullIndex = endSize;

   return iter;
}

template<typename T>
PoolIterator<T> Pool<T>::beginNonValid(){
   if(!mem || allocated == 0){
      return end();
   }

   PoolIterator<T> iter = {};

   iter.SetPool(this);

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
