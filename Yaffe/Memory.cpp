#pragma once

static MemoryStack* CreateMemoryStack(void* pMemory, u64 pSize)
{
	MemoryStack* stack = (MemoryStack*)pMemory;

	stack->size = pSize - sizeof(MemoryStack);
	stack->count = 0;

	return stack;
}

static inline void* MemoryAddress(MemoryStack* pStack, u64 pCount = 0)
{
	return (char*)(pStack + 1) + pCount;
}

static void* PushSize(MemoryStack* pStack, u64 pSize)
{
	assert(pStack->count + pSize <= pStack->size);
	void* alloc = (char*)pStack + sizeof(MemoryStack) + pStack->count;
	pStack->count += pSize;

	return alloc;
}

static void* PushAndZeroSize(MemoryStack* pStack, u64 pSize)
{
	void* size = PushSize(pStack, pSize);
	memset(size, 0, (size_t)pSize);
	return size;
}

#define PushStruct(pStack, pType) (pType*)PushSize(pStack, sizeof(pType))
#define PushArray(pStack, pType, pCount) (pType*)PushSize(pStack, sizeof(pType) * pCount)
#define PushZerodArray(pStack, pType, pCount) (pType*)PushAndZeroSize(pStack, sizeof(pType) * pCount)

MemoryStack* CreateSubStack(MemoryStack* pStack, u64 pSize)
{
	MemoryStack* stack = (MemoryStack*)PushSize(pStack, pSize);
	stack->count = 0;
	stack->size = pSize - sizeof(MemoryStack);

	return stack;
}

TemporaryMemoryHandle BeginTemporaryMemory(MemoryStack* pStack)
{
	TemporaryMemoryHandle handle;
	handle.count = pStack->count;
	handle.stack = pStack;

	return handle;
}

void EndTemporaryMemory(TemporaryMemoryHandle pHandle)
{
	pHandle.stack->count = pHandle.count;
}


MemoryBlock* InsertBlock(MemoryBlock* pPrev, u64 pSize, void* pMemory)
{
	assert(pSize > sizeof(MemoryBlock));
	MemoryBlock* block = (MemoryBlock*)pMemory;
	block->flags = 0;
	block->size = (u32)(pSize - sizeof(MemoryBlock));
	block->previous = pPrev;
	block->next = pPrev->next;
	block->previous->next = block;
	block->next->previous = block;

	return block;
}

MemoryPool* CreateMemoryPool(void* pMemory, u64 pSize)
{
	MemoryPool* pool = (MemoryPool*)pMemory;
	pool->sentinel.flags = MEMORY_FLAGS_Sentinel;
	pool->sentinel.next = &pool->sentinel;
	pool->sentinel.previous = &pool->sentinel;
	pool->sentinel.size = 0;
	pool->free_blocks = &pool->sentinel;

	InsertBlock(&pool->sentinel, pSize - sizeof(MemoryPool), (char*)pMemory + sizeof(MemoryPool));

	return pool;
}

MemoryBlock* FindBlockForSize(MemoryPool* pPool, u64 pSize)
{
	std::lock_guard<std::mutex> guard(pool_mutex);
	MemoryBlock* result = nullptr;
	for (MemoryBlock* block = pPool->sentinel.next;
		block != &pPool->sentinel;
		block = block->next)
	{
		if (!(block->flags & MEMORY_FLAGS_Used) && block->size >= pSize)
		{
			result = block;
			break;
		}
	}

	return result;
}

bool MergeMemoryBlocks(MemoryBlock* pFirst, MemoryBlock* pSecond)
{
	if (pFirst->flags != MEMORY_FLAGS_Sentinel &&
		pSecond->flags != MEMORY_FLAGS_Sentinel)
	{
		if (!(pFirst->flags & MEMORY_FLAGS_Used) &&
			!(pSecond->flags & MEMORY_FLAGS_Used))
		{
			char* expected_second = (char*)pFirst + sizeof(MemoryBlock) + pFirst->size;
			if ((char*)pSecond == expected_second)
			{

				std::lock_guard<std::mutex> guard(pool_mutex);
				pSecond->previous->next = pSecond->next;
				pSecond->next->previous = pSecond->previous;
				pSecond->next = pSecond->previous = nullptr;

				pFirst->size += sizeof(MemoryBlock) + pSecond->size;
				return true;
			}
		}
	}

	return false;
}

void Free(void* pMemory)
{
	if (pMemory)
	{
		MemoryBlock* block = (MemoryBlock*)pMemory - 1;
		block->flags &= ~MEMORY_FLAGS_Used;

		MemoryBlock* previous = block->previous;
		if (MergeMemoryBlocks(previous, block)) block = previous;
		MergeMemoryBlocks(block, block->next);
	}
}

void* Alloc(Assets* pAssets, u64 pSize)
{
	void* result = nullptr;
	MemoryBlock* block = FindBlockForSize(pAssets->memory, pSize);
	for(u32 i = 0; i < 5 && !block; i++)
	{
		EvictOldestAsset(pAssets);
		block = FindBlockForSize(pAssets->memory, pSize);
	}

	assert(block);
	if (pSize <= block->size)
	{
		result = (char*)(block + 1);

		u64 remaining = block->size - pSize;

		std::lock_guard<std::mutex> guard(pool_mutex);
		block->flags |= MEMORY_FLAGS_Used;
		if (remaining > 1024)
		{
			InsertBlock(block, remaining, (char*)result + pSize);
			InterlockedExchangeAdd(&block->size, -(s64)remaining);
		}
	}

	return result;
}

#define AllocStruct(pPool, pType) (pType*)Alloc(pPool, sizeof(pType))
