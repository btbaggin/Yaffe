#pragma once
#include <algorithm>
#include <vector>
#include <assert.h>
#include "time.h"

#include "typedefs.h"
#include "YaffePlatform.h"
#include "Memory.h"
#include "sqlite/sqlite3.h"

const u32 QUEUE_ENTRIES = 256;
const u32 MAX_ERROR_COUNT = 8;
const s32 MAX_MODAL_COUNT = 8;
const u32 UPDATE_FREQUENCY = 30;
const float ExpectedSecondsPerFrame = 1.0F / UPDATE_FREQUENCY;

enum ERROR_TYPE
{
	ERROR_TYPE_Error,
	ERROR_TYPE_Warning
};

typedef bool add_work_queue_entry(PlatformWorkQueue* pQueue, work_queue_callback pCallback, void* pData);
typedef void complete_all_work(PlatformWorkQueue* pQueue);

struct WorkQueueEntry
{
	work_queue_callback* callback;
	void* data;
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

struct Overlay
{
	PlatformWindow* form;
	bool showing;

	PlatformProcess* process;
};

struct YaffeTime
{
	float delta_time;
	long current_time;
};

struct ModalWindow;
struct YaffeState
{
	PlatformWindow* form;
	Overlay overlay;

	List<Platform> platforms;
	s32 selected_platform;
	s32 selected_rom;

	const char* errors[MAX_ERROR_COUNT];
	u32 error_count;
	bool error_is_critical;

	ModalWindow* modals[MAX_MODAL_COUNT];
	volatile LONG current_modal;

	PlatformWorkQueue* queue;
	TaskCallbackQueue callbacks;

	bool is_running;
	YaffeTime time;

	PlatformService* service;
};

void DisplayErrorMessage(const char* pError, ERROR_TYPE pType);
#define Verify(condition, message, type) if (!condition) { DisplayErrorMessage(message, type); return; }