#ifndef INCLUDED_TYPE_SPECIFICS_INCLUDE
#define INCLUDED_TYPE_SPECIFICS_INCLUDE

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

template<> class std::hash<SizedString>{
public:
   std::size_t operator()(SizedString const& s) const noexcept{
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

template<> class std::hash<PortInstance>{
   public:
   std::size_t operator()(PortInstance const& s) const noexcept{
      std::size_t res = std::hash<ComplexFUInstance*>()(s.inst);
      res += s.port;

      return res;
   }
};

template<> class std::hash<StaticId>{
   public:
   std::size_t operator()(StaticId const& s) const noexcept{
      std::size_t res = std::hash<SizedString>()(s.name);
      res += (std::size_t) s.parent;

      return (std::size_t) res;
   }
};

template<> class std::hash<PortEdge>{
   public:
   std::size_t operator()(PortEdge const& s) const noexcept{
      std::size_t res = std::hash<PortInstance>()(s.units[0]);
      res += std::hash<PortInstance>()(s.units[1]);
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

// Try to use this function when not using any std functions, if possible
// I think the hash implementation for pointers is kinda bad, at this this implementation, and not possible to specialize as the compiler complains.
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

inline bool operator<(const SizedString& lhs,const SizedString& rhs){
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

inline bool operator==(const SizedString& lhs,const SizedString& rhs){
   bool res = CompareString(lhs,rhs);

   return res;
}

inline bool operator==(const PortInstance& p1,const PortInstance& p2){
   bool res = (p1.inst == p2.inst && p1.port == p2.port);
   return res;
}

inline bool operator!=(const PortInstance& p1,const PortInstance& p2){
   bool res = !(p1 == p2);
   return res;
}

inline bool operator==(const PortEdge& e1,const PortEdge& e2){
   bool res = (e1.units[0] == e2.units[0] && e1.units[1] == e2.units[1]);
   return res;
}

inline bool operator!=(const PortEdge& e1,const PortEdge& e2){
   bool res = !(e1 == e2);
   return res;
}

inline bool operator==(const MappingNode& node1,const MappingNode& node2){
   bool res = (node1.edges[0] == node2.edges[0] &&
               node1.edges[1] == node2.edges[1]);
   return res;
}

inline bool operator==(const StaticId& id1,const StaticId& id2){
   bool res = CompareString(id1.name,id2.name) && id1.parent == id2.parent;
   return res;
}

#endif // INCLUDED_TYPE_SPECIFICS_INCLUDE
