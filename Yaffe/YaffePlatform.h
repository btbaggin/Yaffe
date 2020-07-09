#pragma once
struct PlatformService;
struct PlatformWorkQueue;
struct PlatformWindow;
struct PlatformProcess;
struct YaffeState;
struct Platform;
struct Executable;
struct Overlay;
struct PlatformInputMessage;


#define WORK_QUEUE_CALLBACK(name)void name(PlatformWorkQueue* pQueue, void* pData)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);

static bool QueueUserWorkItem(PlatformWorkQueue* pQueue, work_queue_callback* pCallback, void* pData);
static void StartProgram(YaffeState* pState, char* pCommand, char* pExe);

static void GetFullPath(const char* pPath, char* pBuffer);
static void CombinePath(char* pBuffer, const char* pBase, const char* pAdditional);
static std::vector<std::string> GetFilesInDirectory(char* pDirectory);
static bool CreateDirectoryIfNotExists(const char* pDirectory);
static bool CopyFileTo(const char* pOld, const char* pNew);

static void SendInputMessage(PlatformInputMessage* pMessage);
static void GetAndSetVolume(float* pVolume, float pDelta);

static void SwapBuffers(PlatformWindow* pWindow);

static void ShowOverlay(Overlay* pOverlay);
static void CloseOverlay(Overlay* pOverlay, bool pTerminate);
static bool ProcessIsRunning(PlatformProcess* pProcess);

static void InitYaffeService(PlatformService* pService);
static void ShutdownYaffeService(PlatformService* pService);

static bool Shutdown();

#define MAX_PATH 260 //Is this smart?
