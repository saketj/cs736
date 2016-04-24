#include <inttypes.h>

#ifdef __cplusplus

class LRUHashMap {
 public:
  bool insert(uint64_t blockno, void *node);
  void* find(uint64_t blockno);
  void* erase(uint64_t blockno);
 private:
  std::unordered_map<uint64_t, void *> hash_map_;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

void* lru_hash_map_init();
int lru_hash_map_insert(void *map, uint64_t blockno, void *node);
void* lru_hash_map_find(void *map, uint64_t blockno);
void* lru_hash_map_erase(void *map, uint64_t blockno);

#ifdef __cplusplus
}
#endif
