#pragma once
#include "typedefs.h"
#include <mutex>

enum MEMORY_FLAGS
{
	MEMORY_FLAGS_Sentinel = 1,
	MEMORY_FLAGS_Used = 2,
};

struct MemoryBlock
{
	MemoryBlock* previous;
	MemoryBlock* next;
	u64 size;
	u8 flags;
};

static std::mutex pool_mutex;
struct MemoryPool
{
	MemoryBlock sentinel;
	MemoryBlock* free_blocks;
};


struct MemoryStack
{
	u64 size;
	u64 count;
};

struct TemporaryMemoryHandle
{
	u64 count;
	MemoryStack* stack;
};
#define Megabytes(size) size * 1024 * 1024
#if _WIN32
#define AtomicCompareExchange(old_val, new_val, comperand) InterlockedCompareExchange(old_val, new_val, comperand)
#define AtomicExchange(old_val, new_val) InterlockedExchange(old_val, new_val)
#define AtomicAdd(val, add) InterlockedExchangeAdd(val, add)
#elif __linux__
#define ZeroMemory(mem, size) memset(mem, 0, size)
#define AtomicCompareExhange(old_val, new_val, comperand) __sync_val_compare_and_swap(old_val, new_val, comperand)
#define AtomicExchange(old_val, new_val) __sync_lock_test_and_set(old_val, new_val)
#define AtomicAdd(val, add) __sync_fetch_and_add(val, add)
#else
static_assert(false);
#endif