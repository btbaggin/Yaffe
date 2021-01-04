#define MAX_PATH PATH_MAX
static void GetFullPath(const char* pPath, char* pBuffer)
{
    realpath(pPath, pBuffer);
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
    int dest = open(pNew, O_WRONLY | O_CREAT /*| O_TRUNC*/, 0644);

    struct stat stat_source;
    fstat(source, &stat_source);

    sendfile(dest, source, 0, stat_source.st_size);

    close(source);
    close(dest);
    return true;
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

static void StartProgram(YaffeState* pState, const char* pApplication, const char* pExecutable, const char* pArgs)
{
	Overlay* overlay = &pState->overlay;
    pid_t process = fork();
    if (process < 0)
    {
        DisplayErrorMessage("Unable to start program", ERROR_TYPE_Warning);
    }
    else if (process == 0)
    {
        int result;
        if(pExecutable) result = execl(pApplication, pArgs, pExecutable);
        else result = execl(pApplication, pArgs);

        if(result == -1)
        {
            DisplayErrorMessage("Unable to start program", ERROR_TYPE_Warning);
            return;
        }

        overlay->process = new PlatformProcess();
		overlay->process->id = process;
    }
}

static void ShowOverlay(Overlay* pOverlay)
{
	XMapWindow(pOverlay->form->platform->display, pOverlay->form->platform->window);
}

static void CloseOverlay(Overlay* pOverlay, bool pTerminate)
{
    XUnmapWindow(pOverlay->form->platform->display, pOverlay->form->platform->window);

    if(pTerminate)
    {
        kill(pOverlay->process->id, SIGKILL);
    }
}

static bool ProcessIsRunning(PlatformProcess* pProcess)
{
    int status;
    return waitpid(pProcess->id, &status, WNOHANG) == 0;
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
    XKeyEvent event;

    // event.display = display;
    // event.window = win;
    // event.root = winRoot;
    // event.subwindow = None;
    // event.time = CurrentTime;
    // event.x = 1;
    // event.y = 1;
    // event.x_root = 1;
    // event.y_root = 1;
    // event.same_screen = True;
    // event.keycode = XKeysymToKeycode(display, keycode);
    // event.state = modifiers;

    // if (press)
    //     event.type = KeyPress;
    // else
    //     event.type = KeyRelease;

    // return event;
    //https://stackoverflow.com/questions/1262310/simulate-keypress-in-a-linux-c-console-application
    switch (pAction)
    {
    case INPUT_ACTION_Key:
    // keycode = XKeysymToKeycode(display, XK_Pause);
    //XTestFakeKeyEvent(display, keycode, True, 0);
    // XTestFakeKeyEvent(display, keycode, False, 0);
        break;

    case INPUT_ACTION_Mouse:
        // buffer.type = INPUT_MOUSE;
        // buffer.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
        // if (pMessage->button == BUTTON_Left) buffer.mi.dwFlags |= (pMessage->down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
        // else if (pMessage->button == BUTTON_Right) buffer.mi.dwFlags |= (pMessage->down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
        break;

    case INPUT_ACTION_Cursor:
        // dpy = XOpenDisplay(0);
        // root_window = XRootWindow(dpy, 0);
        // XSelectInput(dpy, root_window, KeyReleaseMask);
        // XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, 100, 100);
        // XFlush(dpy); // Flushes the output buffer, therefore updates the cursor's position.
        return;

    case INPUT_ACTION_Scroll:
        // buffer.type = INPUT_MOUSE;
        // buffer.mi.dwFlags = MOUSEEVENTF_WHEEL;
        // buffer.mi.mouseData = (DWORD)(-pMessage->scroll * WHEEL_DELTA);
        break;
    }
    // XFlush(display);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent*)&event);
}

static bool GetAndSetVolume(float* pVolume, float pDelta)
{
    return true;
}
static bool RunAtStartUp(STARTUP_INFOS pAction, bool pValue)
{
    if(pAction == STARTUP_INFO_Set)
    {
        char path[MAX_PATH];
        GetFullPath("./Yaffe", path);

        char buffer[1000];
        sprintf(buffer, "ln -s %s /etc/init.d/", path);
        int result = pValue ? system(buffer) : system("rm /etc/init.d/yaffe");
        return WEXITSTATUS(result) == 0;
    }
    else 
    {
        struct stat buf;
        int result = lstat("/etc/init.d/Yaffe", &buf);
        return result == 0;
    }
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