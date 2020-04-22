#pragma once
#include "typedefs.h"
#include "Memory.h"
#include <windows.h>
const u32 QUEUE_ENTRIES = 256;
const u32 MAX_ERROR_COUNT = 8;
const s32 MAX_MODAL_COUNT = 8;

enum ERROR_TYPE
{
	ERROR_TYPE_Error,
	ERROR_TYPE_Warning
};

struct WorkQueue;
#define WORK_QUEUE_CALLBACK(name)void name(WorkQueue* pQueue, void* pData)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);
typedef bool add_work_queue_entry(WorkQueue* pQueue, work_queue_callback pCallback, void* pData);
typedef void complete_all_work(WorkQueue* pQueue);

struct WorkQueueEntry
{
	work_queue_callback* callback;
	void* data;
};

struct WorkQueue
{
	u32 volatile CompletionTarget;
	u32 volatile CompletionCount;
	u32 volatile NextEntryToWrite;
	u32 volatile NextEntryToRead;
	HANDLE Semaphore;

	WorkQueueEntry entries[QUEUE_ENTRIES];
};

typedef void TaskCompleteCallback(void* pData);
struct TaskCallback
{
	TaskCompleteCallback* callback;
	void* data;
};

struct TaskCallbackQueue
{
	TaskCallback callbacks[2][32];
	u64 i;
};

struct Form
{
	u32 width;
	u32 height;

	HWND handle;
	HGLRC rc;
	HDC dc;
};

struct Overlay
{
	u32 width;
	u32 height;
	bool showing;

	HWND handle;
	HDC dc;

	PROCESS_INFORMATION running_rom;
};

struct YaffeTime
{
	float delta_time;
	long current_time;
};

struct ModalWindow;
struct Emulator;
struct YaffeState
{
	Form form;
	Overlay overlay;

	List<Emulator> emulators;
	s32 selected_emulator;
	s32 selected_rom;

	const char* errors[MAX_ERROR_COUNT];
	u32 error_count;
	bool error_is_critical;

	ModalWindow* modals[MAX_MODAL_COUNT];
	volatile LONG current_modal;

	WorkQueue* queue;
	TaskCallbackQueue callbacks;

	bool is_running;
	YaffeTime time;
};

struct win32_thread_info
{
	u32 ThreadIndex;
	WorkQueue* queue;
};

bool QueueUserWorkItem(WorkQueue* pQueue, work_queue_callback* pCallback, void* pData);
void DisplayErrorMessage(const char* pError, ERROR_TYPE pType);