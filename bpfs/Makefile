CC = gcc
CXX = g++
CXXFLAGS = -std=c++11
CFLAGS = -Wall -g
LDFLAGS = -lpthread
# Remove debug checks:
#CFLAGS += -DNDEBUG
# Enable optimizations:
#CFLAGS += -O3 -march=native # GCC >4.4: -flto
# Enable Nehalem optimizations (GCC 4.4 -march only knows up to Core2):
#CFLAGS += -msahf -msse4 -msse4.1 -msse4.2
# Enable gprof:
#CFLAGS += -pg

.PHONY: all clean

BIN = bpfs mkfs.bpfs pwrite
OBJS = bpfs.o anti_cache_manager.o crawler.o diskmanager.o indirect_cow.o \
       mkfs.bpfs.o mkbpfs.o dcache.o hash_map.o vector.o unordered_map.o
TAGS = tags TAGS
SRCS = bpfs_structs.h bpfs.h bpfs.c anti_cache_manager.c anti_cache_manager.h \
			 crawler.h crawler.c dcache.h dcache.c \
       indirect_cow.h indirect_cow.c mkbpfs.h mkbpfs.c mkfs.bpfs.c \
       util.h hash_map.h hash_map.c vector.h vector.c pool.h pwrite.c diskmanager.h diskmanager.c
# Non-compile sources (at least, for this Makefile):
NCSRCS = bench/bpramcount.cpp bench/microbench.py

all: $(BIN) $(TAGS)

clean:
	rm -f $(BIN) $(OBJS) $(TAGS)

tags: $(SRCS) $(NCSRCS)
	@echo + ctags tags
	@if ctags --version | grep -q Exuberant; then ctags $(SRCS) $(NCSRCS); else touch $@; fi
TAGS: $(SRCS) $(NCSRCS)
	@echo + ctags TAGS
	@if ctags --version | grep -q Exuberant; then ctags -e $(SRCS) $(NCSRCS); else touch $@; fi

bpfs.o: bpfs.c anti_cache_manager.h bpfs_structs.h bpfs.h crawler.h  \
	indirect_cow.h mkbpfs.h dcache.h util.h hash_map.h diskmanager.h
	$(CC) $(CFLAGS) `pkg-config --cflags fuse` -c -o $@ $<

mkfs.bpfs.o: mkfs.bpfs.c mkbpfs.h util.h
	$(CC) $(CFLAGS) -c -o $@ $<

anti_cache_manager.o: anti_cache_manager.c anti_cache_manager.h lru_hash_map_interface.h
		$(CC) $(CFLAGS) -c -o $@ $<

diskmanager.o: diskmanager.c diskmanager.h
	$(CC) $(CFLAGS) -c -o $@ $<

indirect_cow.o: indirect_cow.c indirect_cow.h bpfs.h bpfs_structs.h util.h \
	hash_map.h
	$(CC) $(CFLAGS) -c -o $@ $<

crawler.o: crawler.c crawler.h bpfs.h bpfs_structs.h util.h
	$(CC) $(CFLAGS) -c -o $@ $<

mkbpfs.o: mkbpfs.c mkbpfs.h bpfs.h bpfs_structs.h util.h
	$(CC) $(CFLAGS) -c -o $@ $<

dcache.o: dcache.c dcache.h hash_map.h util.h
	$(CC) $(CFLAGS) -c -o $@ $<

lru_hash_map.o: lru_hash_map.cpp lru_hash_map_interface.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

vector.o: vector.c vector.h
	$(CC) $(CFLAGS) -c -o $@ $<

hash_map.o: hash_map.c hash_map.h vector.h pool.h
	$(CC) $(CFLAGS) -c -o $@ $<

bpfs: bpfs.o anti_cache_manager.o crawler.o indirect_cow.o mkbpfs.o dcache.o hash_map.o vector.o diskmanager.o lru_hash_map.o
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^ `pkg-config --libs fuse` -luuid -lrt

mkfs.bpfs: mkfs.bpfs.o mkbpfs.o
	$(CC) $(CFLAGS) -o $@ $^ -luuid
