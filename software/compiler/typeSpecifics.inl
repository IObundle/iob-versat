#pragma once

template<typename T> class std::hash<Array<T>>{
public:
   std::size_t operator()(Array<T> const& s) const noexcept{
   std::size_t res = 0;

   std::size_t prime = 5;
   for(int i = 0; i < s.size; i++){
      res += (std::size_t) s[i] * prime;
      res <<= 4;
      prime += 6; // Some not prime, but will find most of them
   }

   return res;
   }
};

template<> class std::hash<Edge>{
public:
   std::size_t operator()(Edge const& s) const noexcept{
      std::size_t res = std::hash<PortInstance>()(s.units[0]);
      res += std::hash<PortInstance>()(s.units[1]);
      res += s.delay;
      return (std::size_t) res;
   }
};

template<typename First,typename Second> class std::hash<Pair<First,Second>>{
public:
   std::size_t operator()(Pair<First,Second> const& s) const noexcept{
      std::size_t res = std::hash<First>()(s.first);
      res += std::hash<Second>()(s.second);
      return (std::size_t) res;
   }
};

// Try to use this function when not using any std functions, if possible
// I think the hash implementation for pointers is kinda bad, at this implementation, and not possible to specialize as the compiler complains.
#include <type_traits>
template<typename T>
inline int Hash(T const& t){
   int res;
   if(std::is_pointer<T>::value){
      union{
         T val;
         int integer;
      } conv;

      conv.val = t;
      res = (conv.integer >> 2); // TODO: Should be architecture aware
   } else {
      res = std::hash<T>()(t);
   }
   return res;
}

inline bool operator<(const String& lhs,const String& rhs){
   for(int i = 0; i < std::min(lhs.size,rhs.size); i++){
      if(lhs[i] < rhs[i]){
         return true;
      }
      if(lhs[i] > rhs[i]){
         return false;
      }
   }

   if(lhs.size < rhs.size){
      return true;
   }

   return false;
}

inline bool operator==(const String& lhs,const String& rhs){
   bool res = CompareString(lhs,rhs);
   return res;
}

inline bool operator!=(const String& lhs,const String& rhs){
   bool res = !CompareString(lhs,rhs);

   return res;
}

inline bool operator==(const StaticId& id1,const StaticId& id2){
   bool res = CompareString(id1.name,id2.name) && id1.parent == id2.parent;
   return res;
}
