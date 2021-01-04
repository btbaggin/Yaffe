#ifdef _WIN32
typedef HANDLE YaffeSem;
#elif __linux__
typedef sem_t YaffeSem;
#else
static_assert(false);
#endif

struct WorkQueue
{
	u32 volatile CompletionTarget;
	u32 volatile CompletionCount;
	u32 volatile NextEntryToWrite;
	u32 volatile NextEntryToRead;
	YaffeSem Semaphore;

	WorkQueueEntry entries[QUEUE_ENTRIES];
};

struct ThreadInfo
{
	u32 ThreadIndex;
	WorkQueue* queue;
};

bool DoNextWorkQueueEntry(WorkQueue* pQueue)
{
	u32 oldnext = pQueue->NextEntryToRead;
	u32 newnext = (oldnext + 1) % QUEUE_ENTRIES;
	if (oldnext != pQueue->NextEntryToWrite)
	{
		u32 index = AtomicCompareExchange(&pQueue->NextEntryToRead, newnext, oldnext);
		if (index == oldnext)
		{
			WorkQueueEntry entry = pQueue->entries[index];
			entry.callback(pQueue, entry.data);

			AtomicAdd(&pQueue->CompletionCount, 1);
		}
	}
	else
	{
		return true;
	}

	return false;
}

void CompleteAllWork(WorkQueue* pQueue)
{
	while (pQueue->CompletionTarget != pQueue->CompletionCount)
	{
		DoNextWorkQueueEntry(pQueue);
	}

	pQueue->CompletionCount = 0;
	pQueue->CompletionTarget = 0;
}