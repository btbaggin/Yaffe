#pragma once
struct WorkQueue;
struct PlatformWindow;
struct PlatformProcess;
struct YaffeState;
struct Platform;
struct Executable;
struct Overlay;
struct InputMessage;
enum INPUT_ACTIONS
{
	INPUT_ACTION_Scroll,
	INPUT_ACTION_Key,
	INPUT_ACTION_Mouse,
	INPUT_ACTION_Cursor,
};
enum STARTUP_INFOS
{
	STARTUP_INFO_Get,
	STARTUP_INFO_Set,
};

#define WORK_QUEUE_CALLBACK(name)void name(WorkQueue* pQueue, void* pData)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);

static bool QueueUserWorkItem(WorkQueue* pQueue, work_queue_callback* pCallback, void* pData);
static void StartProgram(YaffeState* pState, const char* pApplication, const char* pExecutable, const char* pArgs);
static bool RunAtStartUp(STARTUP_INFOS pAction, bool pValue);

static void GetFullPath(const char* pPath, char* pBuffer);
static std::vector<std::string> GetFilesInDirectory(char* pDirectory);
static bool CreateDirectoryIfNotExists(const char* pDirectory);
static bool CopyFileTo(const char* pOld, const char* pNew);

static void SendInputMessage(INPUT_ACTIONS pAction, InputMessage* pMessage);
static bool GetAndSetVolume(float* pVolume, float pDelta);

static void SwapBuffers(PlatformWindow* pWindow);

static void ShowOverlay(Overlay* pOverlay);
static void CloseOverlay(Overlay* pOverlay, bool pTerminate);
static bool ProcessIsRunning(PlatformProcess* pProcess);

static bool Shutdown();
static bool WindowIsForeground(YaffeState* pState);

static char* GetClipboardText(YaffeState* pState);
static void SetContext(PlatformWindow* pSource, PlatformWindow* pDest);