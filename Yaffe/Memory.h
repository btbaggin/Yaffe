#pragma once

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
