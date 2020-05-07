#pragma once
struct PlatformWorkQueue;
struct PlatformWindow;
struct PlatformProcess;
struct YaffeState;
struct Application;
struct Executable;
struct Overlay;

#define WORK_QUEUE_CALLBACK(name)void name(PlatformWorkQueue* pQueue, void* pData)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);

static bool QueueUserWorkItem(PlatformWorkQueue* pQueue, work_queue_callback* pCallback, void* pData);
static void StartProgram(YaffeState* pState, Application* pApplication, Executable* pRom);

static void GetFullPath(const char* pPath, char* pBuffer);
static void CombinePath(char* pBuffer, const char* pBase, const char* pAdditional);
static std::vector<std::string> GetFilesInDirectory(char* pDirectory);
static bool CreateShortcut(const char* pTargetfile, const char* pTargetargs, char* pLinkfile);
static bool CreateDirectoryIfNotExists(const char* pDirectory);
static bool CopyFileTo(const char* pOld, const char* pNew);

static void SwapBuffers(PlatformWindow* pWindow);

static void ShowOverlay(Overlay* pOverlay);
static void CloseOverlay(Overlay* pOverlay, bool pTerminate);
static bool ProcessIsRunning(PlatformProcess* pProcess);

static bool Shutdown();

#define MAX_PATH 260 //Is this smart?
