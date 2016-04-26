#include <iostream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <inttypes.h>

#include "lru_hash_map_interface.h"

typedef std::unordered_map<uint64_t, void*>::iterator hash_map_iterator;

bool LRUHashMap::insert(uint64_t blockno, void *node) {
  hash_map_.insert(std::make_pair(blockno, node));
  return true;
}

void* LRUHashMap::find(uint64_t blockno) {
  hash_map_iterator itr = hash_map_.find(blockno);
  if (itr != hash_map_.end()) {
    return itr->second;
  } else {
    return nullptr;
  }
}

void* LRUHashMap::erase(uint64_t blockno) {
  hash_map_iterator itr = hash_map_.find(blockno);
  if (itr != hash_map_.end()) {
    void *node = itr->second;
    hash_map_.erase(itr);
    return node;
  } else {
    return nullptr;
  }
}

void* lru_hash_map_init() {
  LRUHashMap *obj = new LRUHashMap();
  return ((void*)obj);
}

int lru_hash_map_insert(void *map, uint64_t blockno, void *node) {
  LRUHashMap *obj = (LRUHashMap *)map;
  bool ret = obj->insert(blockno, node);
  if (ret) {
    return 0;
  } else {
    return 1;
  }
}

void* lru_hash_map_find(void *map, uint64_t blockno) {
  LRUHashMap *obj = (LRUHashMap *)map;
  void *node = obj->find(blockno);
  return node;
}

void* lru_hash_map_erase(void *map, uint64_t blockno) {
  LRUHashMap *obj = (LRUHashMap *)map;
  void *node = obj->erase(blockno);
  return node;
}
