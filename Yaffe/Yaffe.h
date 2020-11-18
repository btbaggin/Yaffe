#pragma once
#include <algorithm>
#include <vector>
#include <map>
#include <assert.h>
#include "time.h"

#include "typedefs.h"
#include "YaffePlatform.h"
#include "Memory.h"
#include "sqlite/sqlite3.h"

const u32 QUEUE_ENTRIES = 256;
const u32 MAX_ERROR_COUNT = 16;
const s32 MAX_MODAL_COUNT = 8;
u32 UPDATE_FREQUENCY = 30;
float ExpectedSecondsPerFrame = 1.0F / UPDATE_FREQUENCY;

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

struct Form
{
	float width;
	float height;
	PlatformWindow* platform;
};

struct ModalWindow;
struct Overlay
{
	Form* form;
	bool showing;
	bool allow_input;
	float guide_hold;

	ModalWindow* modal;
	PlatformProcess* process;
};

struct YaffeTime
{
	float delta_time;
	long current_time;
};

struct RestrictedMode;
struct YaffeState
{
	Form* form;
	Overlay overlay;

	List<Platform> platforms;
	u32 selected_platform;
	s32 selected_rom;

	StaticList<const char*, MAX_ERROR_COUNT> errors;
	bool error_is_critical;

	StaticList<ModalWindow*, MAX_MODAL_COUNT> modals;

	PlatformWorkQueue* work_queue;
	TaskCallbackQueue callbacks;

	bool is_running;
	bool has_connection;

	PlatformService* service;
	RestrictedMode* restrictions;

	std::mutex db_mutex;
};

inline void Tick(YaffeTime* pTime)
{
	auto current_time = clock();
	pTime->delta_time = (current_time - pTime->current_time) / (float)CLOCKS_PER_SEC;
	pTime->current_time = current_time;
}

void DisplayErrorMessage(const char* pError, ERROR_TYPE pType, ...);
#define Verify(condition, message, type) if (!condition) { DisplayErrorMessage(message, type); return; }