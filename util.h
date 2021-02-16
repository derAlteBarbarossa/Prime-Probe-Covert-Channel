#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <x86intrin.h>
#include <sched.h>
#include <time.h>
#include <stdbool.h>

#define ROUNDS				100
#define CPU_AFFINITY		1
#define L1_CACHE_SIZE		32*1024
#define L2_CACHE_SIZE		8*32*1024
#define SETS				64
#define ASSOCIATIVITY		8
#define CACHE_LINE_SIZE		64
#define INDEX_LENGTH		6
#define OFFSET_LENGTH		6
#define L2_L1_RATIO			8
#define BITS				8
#define PIVOT				3

#define TIME_MASK			0xFFFFFF
#define JITTER				0x100
#define INTERVAL			0xFFFFF			

int cmpfunc (const void * a, const void * b);
void assign_to_core(int core_id);

uint64_t* get_eviction_set(uint64_t* base_pointer, int set, int way);
void create_eviction_chain(uint64_t* base_pointer, int associativity);
extern void traverse_eviction_chain(uint64_t* base_pointer, int set);

void sender(char character, uint64_t* base_pointer);
void receiver(uint64_t* base_pointer, int* counts);

void char_to_bool(char character, bool* boolean);
char bool_to_char(bool* boolean);

void demo(char* string, uint64_t* prime_array, uint64_t* probe_array);
void demo_parent_child(char* string, uint64_t* prime_array, uint64_t* probe_array);

// uint64_t synchronise();
uint64_t synchronise_child();
uint64_t synchronise_parent();

void dump_eviction_set(uint64_t* base_pointer);
void validate(uint64_t* base_pointer, int set, int way);