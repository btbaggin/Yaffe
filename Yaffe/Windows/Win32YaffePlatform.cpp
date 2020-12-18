#include <comdef.h>
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#define CHECK_COM_ERROR(hr, error, code) if(FAILED(hr)) { \
	YaffeLogError(error, hr);  \
	code \
	CoUninitialize(); \
	return false; }

static void GetFullPath(const char* pPath, char* pBuffer)
{
	GetFullPathNameA(pPath, MAX_PATH, pBuffer, 0);
}
static void CombinePath(char* pBuffer, const char* pBase, const char* pAdditional)
{
	char file[MAX_PATH];
	u32 i = 0;
	for (i = 0; pAdditional[i] != 0; i++)
	{
		char c = pAdditional[i];
		switch (c)
		{
		case '\\':
		case '/':
		case ':':
		case '?':
		case '\"':
		case '<':
		case '>':
		case '|':
			file[i] = ' ';
			break;
		default:
			file[i] = c;
			break;
		}
	}
	file[i] = '\0';

	PathCombineA(pBuffer, pBase, file);
}
static bool CreateDirectoryIfNotExists(const char* pDirectory)
{
	if (!PathIsDirectoryA(pDirectory))
	{
		SECURITY_ATTRIBUTES sa = {};
		return CreateDirectoryA(pDirectory, &sa);
	}

	return true;
}
static bool CopyFileTo(const char* pOld, const char* pNew)
{
	return CopyFileA(pOld, pNew, false);
}

static bool IsValidRomFile(char* pFile)
{
	//TODO change this
	char* extension = PathFindExtensionA(pFile);
	if (strcmp(extension, ".srm") == 0) return false;
	return true;
}

static void RemovePathExtension(char* pBuffer)
{
	PathRemoveExtensionA(pBuffer);
}

static std::vector<std::string> GetFilesInDirectory(char* pDirectory)
{
	char path[MAX_PATH];
	sprintf(path, "%s\\*.*", pDirectory);


	WIN32_FIND_DATAA file;
	HANDLE h;
	std::vector<std::string> files;
	if ((h = FindFirstFileA(path, &file)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(file.cFileName, ".") != 0 &&
				strcmp(file.cFileName, "..") != 0 &&
				(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
				IsValidRomFile(file.cFileName))
			{
				files.push_back(std::string(file.cFileName));
			}
		} while (FindNextFileA(h, &file));
	}
	FindClose(h);

	return files;
}

static void StartProgram(YaffeState* pState, char* pCommand, char* pExe)
{
	Overlay* overlay = &pState->overlay;

	YaffeLogInfo("Starting Process %s", pCommand);

	STARTUPINFOA si = {};
	PROCESS_INFORMATION pi = {};
	if (CreateProcessA(NULL, pCommand, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		overlay->process = new PlatformProcess();
		overlay->process->id = pi.dwProcessId;
		overlay->process->thread_id = pi.dwThreadId;
		overlay->process->handle = pi.hProcess;
		strcpy(overlay->process->path, pExe);
	}
	else
	{
		if (GetLastError() == ERROR_ELEVATION_REQUIRED) DisplayErrorMessage("Operation requires administrator permissions", ERROR_TYPE_Warning);
		else DisplayErrorMessage("Unable to open rom", ERROR_TYPE_Warning);
	}
	ShowCursor(pState->overlay.allow_input);
}
static void ShowOverlay(Overlay* pOverlay)
{
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(MonitorFromWindow(pOverlay->form->platform->handle, MONITOR_DEFAULTTONEAREST), &mi);
	pOverlay->form->width = (float)(mi.rcMonitor.right - mi.rcMonitor.left);
	pOverlay->form->height = (float)(mi.rcMonitor.bottom - mi.rcMonitor.top);

	SetWindowPos(pOverlay->form->platform->handle, 0, mi.rcMonitor.left, mi.rcMonitor.top, (int)pOverlay->form->width, (int)pOverlay->form->height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	ShowWindow(pOverlay->form->platform->handle, SW_SHOW);
	UpdateWindow(pOverlay->form->platform->handle);
}

// retrieves the (first) process ID of the given executable (or zero if not found)
DWORD GetProcessID(const TCHAR* pszExePathName) {
	// attempt to create a snapshot of the currently running processes
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!snapshot) return 0;

	PROCESSENTRY32 entry = { sizeof(PROCESSENTRY32), 0 };
	for (BOOL bContinue = Process32First(snapshot, &entry); bContinue; bContinue = Process32Next(snapshot, &entry)) {
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
		DWORD dwSize = MAX_PATH;
		if (!QueryFullProcessImageName(hProcess, 0, entry.szExeFile, &dwSize)) continue;

		//if(std::equal(a.begin(), a.end(), b.begin(), [](char a, char b) { return tolower(a) == tolower(b); });
		if (wcscmp(entry.szExeFile, pszExePathName) == 0)
			return entry.th32ProcessID; // FOUND
	}

	return 0; // NOT FOUND
}


static void CloseOverlay(Overlay* pOverlay, bool pTerminate)
{
	ShowWindow(pOverlay->form->platform->handle, SW_HIDE);

	if (pTerminate)
	{
		WCHAR path[MAX_PATH];
		mbstowcs(path, pOverlay->process->path, MAX_PATH);
		if (DWORD dwProcessID = GetProcessID(path)) 
		{
			//Send WM_QUIT to all top level windows and the process thread
			BOOL success = PostThreadMessage(pOverlay->process->thread_id, WM_QUIT, 0, 0);

			for (HWND hwnd = GetTopWindow(NULL); hwnd; hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT)) {
				DWORD dwWindowProcessID;
				DWORD dwThreadID = ::GetWindowThreadProcessId(hwnd, &dwWindowProcessID);
				if (dwWindowProcessID == dwProcessID)
				{
					success &= PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
				}
			}

			//We think we posted WM_QUIT
			//Lets check if the process is still running in case a crash occurred
			if (success && WaitForSingleObject(pOverlay->process->handle, 1000) == WAIT_TIMEOUT)
			{
				DWORD n = 0;
				bool exit = GetExitCodeProcess(pOverlay->process->handle, &n);
				if (exit && n == STILL_ACTIVE) success = false;
				else if(!exit) YaffeLogError("Unable to GetExitCodeProcess: %d", GetLastError());
			}

			if (success) YaffeLogInfo("Able to sucessfully post WM_QUIT");
			else
			{
				YaffeLogInfo("Unable to post WM_QUIT, attempting to force close");

				//Go nuclear
				//Find the exe and force kill it
				HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (hProcessSnap != INVALID_HANDLE_VALUE)
				{
					LPWSTR exe_name = PathFindFileNameW(path);
					PROCESSENTRY32 pe32;
					pe32.dwSize = sizeof(PROCESSENTRY32);
					for (bool process = Process32First(hProcessSnap, &pe32); process; process = Process32Next(hProcessSnap, &pe32))
					{
						//Get the name of the exe from the path
						if (StrCmpW(pe32.szExeFile, exe_name) == 0)
						{
							HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
							if (hProcess)
							{
								success = TerminateProcess(hProcess, 0);
								WaitForSingleObject(hProcess, INFINITE);
								CloseHandle(hProcess);
							}
							break;
						}
					}
					CloseHandle(hProcessSnap);
				}
			}

			if (!success) DisplayErrorMessage("Unable to terminate process: %d", ERROR_TYPE_Warning, (int)GetLastError());
		}
		else
		{
			YaffeLogError("Unable to retrieve process ID %s", pOverlay->process->path);
		}
	}
}
static bool ProcessIsRunning(PlatformProcess* pProcess)
{
	return WaitForSingleObject(pProcess->handle, 20) != 0;
}
static bool QueueUserWorkItem(WorkQueue* pQueue, work_queue_callback* pCallback, void* pData)
{
	u32 newnext = (pQueue->NextEntryToWrite + 1) % QUEUE_ENTRIES;
	if (newnext == pQueue->NextEntryToRead) return false;

	WorkQueueEntry* entry = pQueue->entries + pQueue->NextEntryToWrite;
	entry->data = pData;
	entry->callback = pCallback;

	pQueue->CompletionTarget++;
	_WriteBarrier();
	_mm_sfence();

	pQueue->NextEntryToWrite = newnext;
	ReleaseSemaphore(pQueue->Semaphore, 1, 0);

	return true;
}
static bool Shutdown()
{
	HANDLE token;
	TOKEN_PRIVILEGES tkp;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);

	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0)) return false;
	return ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_FLAG_PLANNED);
}

static bool WindowIsForeground(YaffeState* pState)
{
	return GetForegroundWindow() == pState->form->platform->handle;
}

static void SwapBuffers(PlatformWindow* pWindow)
{
	SwapBuffers(pWindow->dc);
}

static void SendInputMessage(INPUT_ACTIONS pAction, InputMessage* pMessage)
{
	INPUT buffer = {};
	switch (pAction)
	{
	case INPUT_ACTION_Key:
		buffer.type = INPUT_KEYBOARD;
		buffer.ki.wVk = pMessage->key;
		if (!pMessage->down) buffer.ki.dwFlags = KEYEVENTF_KEYUP;
		break;

	case INPUT_ACTION_Mouse:
		buffer.type = INPUT_MOUSE;
		buffer.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
		if (pMessage->button == BUTTON_Left) buffer.mi.dwFlags |= (pMessage->down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
		else if (pMessage->button == BUTTON_Right) buffer.mi.dwFlags |= (pMessage->down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
		break;

	case INPUT_ACTION_Cursor:
		SetCursorPos((int)pMessage->cursor.X, (int)pMessage->cursor.Y);
		return;

	case INPUT_ACTION_Scroll:
		buffer.type = INPUT_MOUSE;
		buffer.mi.dwFlags = MOUSEEVENTF_WHEEL;
		buffer.mi.mouseData = (DWORD)(-pMessage->scroll * WHEEL_DELTA);
		break;
	}
	SendInput(1, &buffer, sizeof(INPUT));
}
static bool GetAndSetVolume(float* pVolume, float pDelta)
{
	CoInitialize(NULL);
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
	CHECK_COM_ERROR(hr, "CoCreateInstance failed: %x", );

	IMMDevice *defaultDevice = nullptr;
	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	CHECK_COM_ERROR(hr, "Unable to get default audio endpoint: %x", {
		deviceEnumerator->Release();
		});

	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&endpointVolume);
	CHECK_COM_ERROR(hr, "Unable to activate default audio device: %x", {
		defaultDevice->Release();
		deviceEnumerator->Release();
		});

	hr = endpointVolume->GetMasterVolumeLevelScalar(pVolume);
	CHECK_COM_ERROR(hr, "Unable to get volume level: %x", {
		defaultDevice->Release();
		deviceEnumerator->Release();
		})

	if (pDelta != 0)
	{
		*pVolume = std::min(1.0F, std::max(0.0F, *pVolume + pDelta));
		hr = endpointVolume->SetMasterVolumeLevelScalar(*pVolume, NULL);
		CHECK_COM_ERROR(hr, "Unable to set volume level: %x", {
			defaultDevice->Release();
			deviceEnumerator->Release();
			endpointVolume->Release();
			});
	}

	defaultDevice->Release();
	deviceEnumerator->Release();
	endpointVolume->Release();
	CoUninitialize();
	return true;
}
static bool RunAtStartUp(STARTUP_INFOS pAction, bool pValue)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);

	const char* task_name = "Yaffe";

	char path[MAX_PATH];
	char working_path[MAX_PATH];
	GetFullPath(".", working_path);
	GetFullPath(".\\Yaffe.exe", path);

	//  Create an instance of the Task Service. 
	ITaskService *pService = nullptr;
	hr = CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	CHECK_COM_ERROR(hr, "Failed to create an instance of ITaskService: %x", {
			pService->Release();
		});

	//  Connect to the task service.
	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	CHECK_COM_ERROR(hr, "ITaskService::Connect failed: %x", {
		pService->Release();
		});

	ITaskFolder *pRootFolder = nullptr;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	CHECK_COM_ERROR(hr, "Cannot get Root Folder pointer: %x", {
		pService->Release();
		});

	//  If the same task exists, remove it.
	IRegisteredTask* task = nullptr;
	pRootFolder->GetTask(_bstr_t(task_name), &task);
	if (pAction == STARTUP_INFO_Get)
	{
		pService->Release();
		pRootFolder->Release();
		CoUninitialize();
		return task != nullptr;
	}
	else
	{
		if (!pValue)
		{
			hr = pRootFolder->DeleteTask(_bstr_t(task_name), 0);
			CHECK_COM_ERROR(hr, "Failed to delete task: %x", {
				pService->Release();
				pRootFolder->Release();
				});

			pService->Release();
			pRootFolder->Release();
			CoUninitialize();
			return true;
		}
		else if(!task)
		{
			//  Create the task builder object to create the task.
			ITaskDefinition *pTask = nullptr;
			hr = pService->NewTask(0, &pTask);
			pService->Release();  // COM clean up.  Pointer is no longer used.

			CHECK_COM_ERROR(hr, "Failed to create a task definition: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			IPrincipal* prin = nullptr;
			pTask->get_Principal(&prin);
			hr = prin->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
			prin->Release();
			CHECK_COM_ERROR(hr, "Unable to set admin privileges: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			//  Create the settings for the task
			ITaskSettings *pSettings = nullptr;
			hr = pTask->get_Settings(&pSettings);
			CHECK_COM_ERROR(hr, "Cannot get settings pointer: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			//  Set setting values for the task. 
			hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
			pSettings->Release();
			CHECK_COM_ERROR(hr, "Cannot put setting info: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			//  Add the logon trigger to the task.
			ITriggerCollection *pTriggerCollection = nullptr;
			hr = pTask->get_Triggers(&pTriggerCollection);
			CHECK_COM_ERROR(hr, "Cannot get trigger collection: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			ITrigger *pTrigger = nullptr;
			hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
			pTriggerCollection->Release();
			CHECK_COM_ERROR(hr, "Cannot create the trigger: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			//  Add an Action to the task.
			IActionCollection *pActionCollection = nullptr;
			hr = pTask->get_Actions(&pActionCollection);
			CHECK_COM_ERROR(hr, "Cannot get Task collection pointer: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			IAction *action = nullptr;
			hr = pActionCollection->Create(TASK_ACTION_EXEC, &action);
			pActionCollection->Release();
			CHECK_COM_ERROR(hr, "Cannot create the action: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			IExecAction *pExecAction = nullptr;
			hr = action->QueryInterface(IID_IExecAction, (void**)&pExecAction);
			action->Release();
			CHECK_COM_ERROR(hr, "QueryInterface call failed for IExecAction: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			hr = pExecAction->put_Path(_bstr_t(path));
			CHECK_COM_ERROR(hr, "Cannot set path of executable: %x", {
				pTask->Release();
				pExecAction->Release();
				pRootFolder->Release();
				});

			hr = pExecAction->put_WorkingDirectory(_bstr_t(working_path));
			pExecAction->Release();
			CHECK_COM_ERROR(hr, "Cannot set working directory of executable: %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			//  Save the task in the root folder.
			IRegisteredTask *pRegisteredTask = nullptr;
			hr = pRootFolder->RegisterTaskDefinition(_bstr_t(task_name), pTask, TASK_CREATE_OR_UPDATE, _variant_t(L"Builtin\\Administrators"), 
													 _variant_t(), TASK_LOGON_GROUP, _variant_t(L""), &pRegisteredTask);
			CHECK_COM_ERROR(hr, "Error saving the Task : %x", {
				pTask->Release();
				pRootFolder->Release();
				});

			// Clean up
			pRootFolder->Release();
			pTask->Release();
			pRegisteredTask->Release();
			CoUninitialize();
			return true;
		}

		return true;
	}
}

static char* GetClipboardText(YaffeState* pState)
{
	if (!OpenClipboard(nullptr)) return nullptr;

	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData == nullptr) return nullptr;

	char * pszText = static_cast<char*>(GlobalLock(hData));
	if (pszText == nullptr) return nullptr;

	char* result = pszText;

	GlobalUnlock(hData);
	CloseClipboard();

	return result;
}

static void SetContext(PlatformWindow* pSource, PlatformWindow* pDest)
{
	wglMakeCurrent(pSource->dc, pDest->rc);
}