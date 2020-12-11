static void GetFullPath(const char* pPath, char* pBuffer)
{
    realpath(pPath, pBuffer);
}
static void CombinePath(char* pBuffer, const char* pBase, const char* pAdditional)
{
}
static bool CreateDirectoryIfNotExists(const char* pDirectory)
{
    return true;
}
static bool CopyFileTo(const char* pOld, const char* pNew)
{
    return true;
}
static bool IsValidRomFile(char* pFile)
{
    return true;
}
static std::vector<std::string> GetFilesInDirectory(char* pDirectory)
{
    std::vector<std::string> files;
    return files;
}

static bool OpenFileSelector(char* pPath, bool pFiles)
{
    return true;
}


static void StartProgram(YaffeState* pState, char* pCommand, char* pExe)
{
}
static void ShowOverlay(Overlay* pOverlay)
{
}

static void CloseOverlay(Overlay* pOverlay, bool pTerminate)
{
}
static bool ProcessIsRunning(PlatformProcess* pProcess)
{
    return true;
}
static bool QueueUserWorkItem(PlatformWorkQueue* pQueue, work_queue_callback* pCallback, void* pData)
{
    return true;
}
static bool Shutdown()
{
    return true;
}
static void SwapBuffers(PlatformWindow* pWindow)
{
    glXSwapBuffers(pWindow->display, pWindow->window);
}
static void SendInputMessage(INPUT_ACTIONS pAction, PlatformInputMessage* pMessage)
{
}
static bool GetAndSetVolume(float* pVolume, float pDelta)
{
    return true;
}
static bool RunAtStartUp(STARTUP_INFOS pAction, bool pValue)
{
    return true;
}