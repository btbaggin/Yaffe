static void GetFullPath(const char* pPath, char* pBuffer)
{
    realpath(pPath, pBuffer);
}

static void CombinePath(char* pBuffer, const char* pBase, const char* pAdditional)
{
    if(pBase == NULL && pAdditional == NULL) {
        strcpy(pBuffer, "");;
    }
    else if(pAdditional == NULL || strlen(pAdditional) == 0) {
        strcpy(pBuffer, pBase);
    }
    else if(pBase == NULL || strlen(pBase) == 0) {
        strcpy(pBuffer, pAdditional);
    } 
    else {
        
#ifdef WIN32
        char directory_separator[] = "\\";
#elif __linux__
        char directory_separator[] = "/";
#else
        static_assert(false);
#endif
        const char *last_char = pBase;
        while(*last_char != '\0')
            last_char++;        

        int append_directory_separator = 0;
        if(strcmp(last_char, directory_separator) != 0) {
            append_directory_separator = 1;
        }
        strcpy(pBuffer, pBase);
        if(append_directory_separator) strcat(pBuffer, directory_separator);
        
        strcat(pBuffer, pAdditional);
    }
}

static bool CreateDirectoryIfNotExists(const char* pDirectory)
{
    if (mkdir(pDirectory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
    {
        return errno == EEXIST;
    }
    return true;
}

static bool CopyFileTo(const char* pOld, const char* pNew)
{
    int source = open(pOld, O_RDONLY, 0);
    int dest = open(pNew, O_WRONLY | O_CREAT /*| O_TRUNC/**/, 0644);

    // struct required, rationale: function stat() exists also
    struct stat stat_source;
    fstat(source, &stat_source);

    sendfile(dest, source, 0, stat_source.st_size);

    close(source);
    close(dest);
    return true;
}

static bool IsValidRomFile(char* pFile)
{
    return true;
}

static void RemovePathExtension(char* pBuffer)
{
}

static std::vector<std::string> GetFilesInDirectory(char* pDirectory)
{
    std::vector<std::string> files;
    DIR* dirp = opendir(pDirectory);
    dirent* dp;
    while ((dp = readdir(dirp)) != NULL)
    {
        if (dp->d_type == DT_REG) files.push_back(dp->d_name);
    }
        
    closedir(dirp);
    return files;
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

static bool QueueUserWorkItem(WorkQueue* pQueue, work_queue_callback* pCallback, void* pData)
{
    u32 newnext = (pQueue->NextEntryToWrite + 1) % QUEUE_ENTRIES;
	if (newnext == pQueue->NextEntryToRead) return false;

	WorkQueueEntry* entry = pQueue->entries + pQueue->NextEntryToWrite;
	entry->data = pData;
	entry->callback = pCallback;

	pQueue->CompletionTarget++;
	WriteBarrier();
	_mm_sfence();

	pQueue->NextEntryToWrite = newnext;
    sem_post(&pQueue->Semaphore);

	return true;
}
static bool Shutdown()
{
    sync();
    reboot(RB_POWER_OFF);
    return true;
}

static bool WindowIsForeground(YaffeState* pState)
{
    Window focused;
    int revert_to;
    XGetInputFocus(pState->form->platform->display, &focused, &revert_to);
	return pState->form->platform->window == focused;
}

static void SwapBuffers(PlatformWindow* pWindow)
{
    glXSwapBuffers(pWindow->display, pWindow->window);
}

static void SendInputMessage(INPUT_ACTIONS pAction, InputMessage* pMessage)
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

char* XPasteType(Atom atom, PlatformWindow* pWindow, Atom pUtf8) {
	XEvent event;
	int format;
	unsigned long N, size;
	char* data, * s = 0;
	Atom target;
	Atom CLIPBOARD = XInternAtom(pWindow->display, "CLIPBOARD", 0);
	Atom XSEL_DATA = XInternAtom(pWindow->display, "XSEL_DATA", 0);
	XConvertSelection(pWindow->display, CLIPBOARD, atom, XSEL_DATA, pWindow->window, CurrentTime);
	XSync(pWindow->display, 0);
	XNextEvent(pWindow->display, &event);
	
	if (event.type == SelectionNotify)
    {
        if(event.xselection.selection != CLIPBOARD) return s;
        if(event.xselection.property) 
        {
            XGetWindowProperty(event.xselection.display, event.xselection.requestor,
                event.xselection.property, 0L,(~0L), 0, AnyPropertyType, &target,
                &format, &size, &N,(unsigned char**)&data);
            if(target == pUtf8 || target == XA_STRING) 
            {
                s = strndup(data, size);
                XFree(data);
            }
            XDeleteProperty(event.xselection.display, event.xselection.requestor, event.xselection.property);
        }
	}
  return s;
}
static char* GetClipboardText(YaffeState* pState)
{
	char* c = 0;
	Atom UTF8 = XInternAtom(pState->form->platform->display, "UTF8_STRING", True);
	if(UTF8 != None) c = XPasteType(UTF8, pState->form->platform, UTF8);
	if(!c) c = XPasteType(XA_STRING, pState->form->platform, UTF8);
	return c;
}

static void SetContext(PlatformWindow* pSource, PlatformWindow* pDest)
{
	glXMakeCurrent(pSource->display, pDest->window, pSource->context);
}