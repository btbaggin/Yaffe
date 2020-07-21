#pragma comment(lib, "XInput.lib") 

/*
TODO
Modals not centered for 1 frame?
Don't hardcode emulator allocation count
*/

#define GLEW_STATIC
#include <glew/glew.h>
#include <gl/GL.h>
#include "gl/wglext.h"
#include <Shlwapi.h>
#include <dwmapi.h>
#include <mmreg.h>
#include "intrin.h"
#include <Windows.h>
#include <ShlObj.h>
#include <WinInet.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include "Yaffe.h"
#include "Memory.h"
#include "Assets.h"
#include "Input.h"
#include "Render.h"
#include "Interface.h"
#include "Platform.h"
#include "Database.h"
#include <json.h>
#include "TextureAtlas.h"

using json = picojson::value;

struct PlatformWorkQueue
{
	u32 volatile CompletionTarget;
	u32 volatile CompletionCount;
	u32 volatile NextEntryToWrite;
	u32 volatile NextEntryToRead;
	HANDLE Semaphore;

	WorkQueueEntry entries[QUEUE_ENTRIES];
};

struct PlatformWindow
{
	float width;
	float height;

	HWND handle;
	HGLRC rc;
	HDC dc;
};

struct PlatformProcess
{
	HANDLE handle;
	DWORD id;
	DWORD thread_id;
	char path[MAX_PATH];
};

struct PlatformInputMessage
{
	v2 cursor;
	bool down;
	union
	{
		MOUSE_BUTTONS button;
		KEYS key;
	};
	float scroll;
};

struct win32_thread_info
{
	u32 ThreadIndex;
	PlatformWorkQueue* queue;
};

YaffeState g_state = {};
YaffeInput g_input = {};
Assets* g_assets;
Interface g_ui = {};

#include "Logger.cpp"
#include "Memory.cpp"
#include "Assets.cpp"
#include "Input.cpp"
#include "Render.cpp"
#include "UiElements.cpp"
#include "Modal.cpp"
#include "ListModal.cpp"
#include "PlatformDetailModal.cpp"
#include "Server.cpp"
#include "Database.cpp"
#include "Platform.cpp"
#include "Interface.cpp"
#include "YaffeOverlay.cpp"

const LPCWSTR WINDOW_CLASS = L"Yaffe";
const LPCWSTR OVERLAY_CLASS = L"Overlay";

//
// Win32 implementation of Platform.h
//
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
	char* extension = PathFindExtensionA(pFile);
	if (strcmp(extension, ".srm") == 0) return false;
	return true;
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

static bool OpenFileSelector(char* pPath, bool pFiles)
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	IFileDialog *pfd = NULL;
	CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	// Before setting, always get the options first in order 
	// not to override existing options.
	DWORD dwFlags;
	pfd->GetOptions(&dwFlags);
	if (pFiles)
	{
		COMDLG_FILTERSPEC rgSpec[] = { { L"Emulator", L"*.exe" } };
		pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
		pfd->SetFileTypes(ArrayCount(rgSpec), rgSpec);
	}
	else
	{
		pfd->SetOptions(dwFlags | FOS_PICKFOLDERS);
	}

	bool result = false;
	// Set the file types to display only. 
	if (SUCCEEDED(pfd->Show(NULL)))
	{
		IShellItem *psiResult;
		if (SUCCEEDED(pfd->GetResult(&psiResult)))
		{
			PWSTR pszFilePath = NULL;
			if (SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
			{
				int len = WideCharToMultiByte(CP_ACP, 0, pszFilePath, lstrlenW(pszFilePath), NULL, 0, NULL, NULL);
				WideCharToMultiByte(CP_ACP, 0, pszFilePath, lstrlenW(pszFilePath), pPath, len, NULL, NULL);
				pPath[len] = '\0';
				CoTaskMemFree(pszFilePath);

				result = true;
			}
			psiResult->Release();
		}
	}
	pfd->Release();
	CoUninitialize();

	return result;
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
}
static void ShowOverlay(Overlay* pOverlay)
{
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(MonitorFromWindow(pOverlay->form->handle, MONITOR_DEFAULTTONEAREST), &mi);
	pOverlay->form->width = (float)(mi.rcMonitor.right - mi.rcMonitor.left);
	pOverlay->form->height = (float)(mi.rcMonitor.bottom - mi.rcMonitor.top);

	SetWindowPos(pOverlay->form->handle, 0, mi.rcMonitor.left, mi.rcMonitor.top, (int)pOverlay->form->width, (int)pOverlay->form->height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	ShowWindow(pOverlay->form->handle, SW_SHOW);
	UpdateWindow(pOverlay->form->handle);
}

static void CloseOverlay(Overlay* pOverlay, bool pTerminate)
{
	ShowWindow(pOverlay->form->handle, SW_HIDE);

	if (pTerminate)
	{
		//Try the nice way of closing the process
		bool success = PostThreadMessage(pOverlay->process->thread_id, WM_QUIT, 0, 0);

		if (success)
		{
			YaffeLogInfo("Able to sucessfully post WM_QUIT");
		}
		else
		{
			//Go nuclear
			//Find the exe and force kill it
			HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hProcessSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 pe32;
				pe32.dwSize = sizeof(PROCESSENTRY32);
				if (Process32First(hProcessSnap, &pe32))
				{
					//Get the name of the exe from the path
					wchar_t wcstring[MAX_PATH];
					mbstowcs_s(0, wcstring, strlen(pOverlay->process->path) + 1, pOverlay->process->path, _TRUNCATE);
					LPWSTR exe_name = PathFindFileNameW(wcstring);

					do
					{
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
					} while (Process32Next(hProcessSnap, &pe32));
				}
				CloseHandle(hProcessSnap);
			}
		}

		if(!success)
		{
			char* error = new char[100];
			sprintf(error, "Unable to terminate process: %d", (int)GetLastError());
			DisplayErrorMessage(error, ERROR_TYPE_Warning);
		}
	}
}
static bool ProcessIsRunning(PlatformProcess* pProcess)
{
	return WaitForSingleObject(pProcess->handle, 20) != 0;
}
static bool QueueUserWorkItem(PlatformWorkQueue* pQueue, work_queue_callback* pCallback, void* pData)
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
static void SwapBuffers(PlatformWindow* pWindow)
{
	SwapBuffers(pWindow->dc);
}
static void SendInputMessage(INPUT_ACTIONS pAction, PlatformInputMessage* pMessage)
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
static void GetAndSetVolume(float* pVolume, float pDelta)
{
	CoInitialize(NULL);
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
	IMMDevice *defaultDevice = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	deviceEnumerator->Release();
	deviceEnumerator = NULL;

	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&endpointVolume);
	defaultDevice->Release();
	defaultDevice = NULL;

	hr = endpointVolume->GetMasterVolumeLevelScalar(pVolume);

	if (pDelta != 0)
	{
		*pVolume = min(1.0F, max(0.0F, *pVolume + pDelta));
		hr = endpointVolume->SetMasterVolumeLevelScalar(*pVolume, NULL);
	}

	endpointVolume->Release();
	CoUninitialize();
}

//
// Main Yaffe loop
//
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CreateOpenGLWindow(PlatformWindow* pForm, HINSTANCE hInstance, u32 pWidth, u32 pHeight, LPCWSTR pTitle, bool pFullscreen)
{
	WNDCLASSW wcex = {};
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = WINDOW_CLASS;
	RegisterClassW(&wcex);

	const long style = WS_OVERLAPPEDWINDOW;
	HWND fakeHwnd = CreateWindowW(WINDOW_CLASS, L"Fake Window", style, 0, 0, 1, 1, NULL, NULL, hInstance, NULL);

	HDC fakeDc = GetDC(fakeHwnd);

	PIXELFORMATDESCRIPTOR fakePfd = {};
	fakePfd.nSize = sizeof(fakePfd);
	fakePfd.nVersion = 1;
	fakePfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	fakePfd.iPixelType = PFD_TYPE_RGBA;
	fakePfd.cColorBits = 32;
	fakePfd.cAlphaBits = 8;
	fakePfd.cDepthBits = 24;

	int id = ChoosePixelFormat(fakeDc, &fakePfd);
	if (!id) return false;

	if (!SetPixelFormat(fakeDc, id, &fakePfd)) return false;

	HGLRC fakeRc = wglCreateContext(fakeDc);
	if (!fakeRc) return false;

	if (!wglMakeCurrent(fakeDc, fakeRc)) return false;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
	wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
	if (!wglChoosePixelFormatARB) return false;

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
	wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (!wglCreateContextAttribsARB) return false;

	u32 x = 0;
	u32 y = 0;
	if (!pFullscreen)
	{
		RECT rect = { 0L, 0L, (LONG)pWidth, (LONG)pHeight };
		AdjustWindowRect(&rect, style, false);
		pForm->width = (float)(rect.right - rect.left);
		pForm->height = (float)(rect.bottom - rect.top);

		RECT primaryDisplaySize;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &primaryDisplaySize, 0);	// system taskbar and application desktop toolbars not included
		x = (u32)(primaryDisplaySize.right - pForm->width) / 2;
		y = (u32)(primaryDisplaySize.bottom - pForm->height) / 2;
	}

	pForm->handle = CreateWindowW(WINDOW_CLASS, pTitle, style, x, y, (int)pForm->width, (int)pForm->height, NULL, NULL, hInstance, NULL);
	pForm->dc = GetDC(pForm->handle);

	if (pFullscreen)
	{
		SetWindowLong(pForm->handle, GWL_STYLE, wcex.style & ~(WS_CAPTION | WS_THICKFRAME));

		// On expand, if we're given a window_rect, grow to it, otherwise do
		// not resize.
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(pForm->handle, MONITOR_DEFAULTTONEAREST), &mi);
		pForm->width = (float)(mi.rcMonitor.right - mi.rcMonitor.left);
		pForm->height = (float)(mi.rcMonitor.bottom - mi.rcMonitor.top);

		SetWindowPos(pForm->handle, NULL, mi.rcMonitor.left, mi.rcMonitor.top,
			(int)pForm->width, (int)pForm->height,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	const int pixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 4,
		0
	};

	int pixelFormatID; UINT numFormats;
	const bool status = wglChoosePixelFormatARB(pForm->dc, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	if (!status || !numFormats) return false;

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(pForm->dc, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(pForm->dc, pixelFormatID, &PFD);

	const int major_min = 4, minor_min = 0;
	const int contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, major_min,
		WGL_CONTEXT_MINOR_VERSION_ARB, minor_min,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		//		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				0
	};

	pForm->rc = wglCreateContextAttribsARB(pForm->dc, 0, contextAttribs);
	if (!pForm->rc) return false;

	// delete temporary context and window
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeRc);
	ReleaseDC(fakeHwnd, fakeDc);
	DestroyWindow(fakeHwnd);

	if (!wglMakeCurrent(pForm->dc, pForm->rc)) return false;

	return true;
}

static bool CreateOverlayWindow(Overlay* pOverlay, HINSTANCE hInstance)
{
	WNDCLASSW wcex = {};
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = OverlayWndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = OVERLAY_CLASS;
	RegisterClassW(&wcex);

	// | WS_EX_LAYERED
	pOverlay->form->handle = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, OVERLAY_CLASS, L"Yaffe Overlay", WS_POPUP, 0, 0, (int)pOverlay->form->width, (int)pOverlay->form->height, NULL, NULL, hInstance, NULL);
	pOverlay->form->dc = GetDC(pOverlay->form->handle);

	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.fEnable = true;
	bb.hRgnBlur = CreateRectRgn(0, 0, 1, 1);
	if (!SUCCEEDED(DwmEnableBlurBehindWindow(pOverlay->form->handle, &bb))) return false;

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
	wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
	if (!wglChoosePixelFormatARB) return false;

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
	wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (!wglCreateContextAttribsARB) return false;

	const int pixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 4,
		0
	};

	int pixelFormatID; UINT numFormats;
	const bool status = wglChoosePixelFormatARB(pOverlay->form->dc, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	if (!status || !numFormats) return false;

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(pOverlay->form->dc, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(pOverlay->form->dc, pixelFormatID, &PFD);

	pOverlay->showing = false;

	return true;
}

static void DestroyGlWindow(PlatformWindow* pForm)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(pForm->rc);
	ReleaseDC(pForm->handle, pForm->dc);
	DestroyWindow(pForm->handle);
}

bool Win32DoNextWorkQueueEntry(PlatformWorkQueue* pQueue)
{
	u32 oldnext = pQueue->NextEntryToRead;
	u32 newnext = (oldnext + 1) % QUEUE_ENTRIES;
	if (oldnext != pQueue->NextEntryToWrite)
	{
		u32 index = InterlockedCompareExchange((LONG volatile*)&pQueue->NextEntryToRead, newnext, oldnext);
		if (index == oldnext)
		{
			WorkQueueEntry entry = pQueue->entries[index];
			entry.callback(pQueue, entry.data);

			InterlockedIncrement((LONG volatile*)&pQueue->CompletionCount);
		}
	}
	else
	{
		return true;
	}

	return false;
}

void Win32CompleteAllWork(PlatformWorkQueue* pQueue)
{
	while (pQueue->CompletionTarget != pQueue->CompletionCount)
	{
		Win32DoNextWorkQueueEntry(pQueue);
	}

	pQueue->CompletionCount = 0;
	pQueue->CompletionTarget = 0;
}

DWORD WINAPI ThreadProc(LPVOID pParameter)
{
	win32_thread_info* thread = (win32_thread_info*)pParameter;

	while (true)
	{
		if (Win32DoNextWorkQueueEntry(thread->queue))
		{
			WaitForSingleObjectEx(thread->queue->Semaphore, INFINITE, false);
		}
	}
}

void Win32GetInput(YaffeInput* pInput, HWND pHandle)
{
	memcpy(pInput->previous_keyboard_state, pInput->current_keyboard_state, 256);
	pInput->previous_controller_buttons = pInput->current_controller_buttons;

	GetKeyboardState((PBYTE)pInput->current_keyboard_state);

	POINT point;
	GetCursorPos(&point);
	ScreenToClient(pHandle, &point);

	pInput->mouse_position = V2((float)point.x, (float)point.y);

	XINPUT_GAMEPAD_EX state = {};
	DWORD result = g_input.XInputGetState(0, &state);
	if (result == ERROR_SUCCESS)
	{
		pInput->current_controller_buttons = state.wButtons;
		pInput->left_stick = V2((float)state.sThumbLX, (float)state.sThumbLY);
		pInput->right_stick = V2((float)state.sThumbRX, (float)state.sThumbRY);

		float length = HMM_LengthSquaredVec2(pInput->left_stick);
		if (length <= XINPUT_INPUT_DEADZONE * XINPUT_INPUT_DEADZONE)
		{
			pInput->left_stick = V2(0);
		}
		length = HMM_LengthSquaredVec2(pInput->right_stick);
		if (length <= XINPUT_INPUT_DEADZONE * XINPUT_INPUT_DEADZONE)
		{
			pInput->right_stick = V2(0);
		}
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
{

	InitializeLogger();

	PlatformWindow form = {};
	g_state.form = &form;
	if (!CreateOpenGLWindow(g_state.form, hInstance, 720, 480, L"Yaffe", true))
	{
		MessageBoxA(nullptr, "Unable to initialize window", "Error", MB_OK);
		return 1;
	}
	ShowWindow(g_state.form->handle, nCmdShow);
	UpdateWindow(g_state.form->handle);

	PlatformWindow overlay = {};
	g_state.overlay.form = &overlay;
	if (!CreateOverlayWindow(&g_state.overlay, hInstance))
	{
		MessageBoxA(nullptr, "Unable to initialize overlay", "Error", MB_OK);
		return 1;
	}

	//Set up work queue
	const u32 THREAD_COUNT = 4;
	win32_thread_info threads[THREAD_COUNT];
	PlatformWorkQueue work_queue = {};
	work_queue.Semaphore = CreateSemaphoreEx(0, 0, THREAD_COUNT, 0, 0, SEMAPHORE_ALL_ACCESS);
	for (u32 i = 0; i < THREAD_COUNT; i++)
	{
		win32_thread_info* thread = threads + i;
		thread->ThreadIndex = i;
		thread->queue = &work_queue;

		DWORD id;
		HANDLE t = CreateThread(0, 0, ThreadProc, thread, 0, &id);
		CloseHandle(t);
	}
	g_state.work_queue = &work_queue;

	LARGE_INTEGER i;
	QueryPerformanceFrequency(&i);
	double computer_frequency = double(i.QuadPart);

	glewExperimental = true; // Needed for core profile
	glewInit();

	//Check if we need admin privileges to write to the current directory
	char base_path[MAX_PATH];
	GetFullPath(".", base_path);
	CombinePath(base_path, base_path, "lock");
	HANDLE h = CreateFileA(base_path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		if (error == ERROR_ACCESS_DENIED) MessageBoxA(nullptr, "Administrator privileges required", "Error", MB_OK);
		else MessageBoxA(nullptr, "Unable to create required directories", "Error", MB_OK);
		return 0;
	}
	else
	{
		CloseHandle(h);
		DeleteFileA(base_path);
	}

	//Initialization
	u32 asset_size = Megabytes(6);
	void* asset_memory = malloc(asset_size);
	ZeroMemory(asset_memory, asset_size);

	RenderState render_state = {};
	InitializeRenderer(&render_state);
	g_assets = LoadAssets(asset_memory, asset_size);
	InitializeUI(&g_state);
	InitailizeDatbase(&g_state);

	g_state.has_connection = InternetGetConnectedState(0, 0);
	g_state.service = new PlatformService();
	InitYaffeService(g_state.service);
	GetAllPlatforms(&g_state);

	HINSTANCE xinput_dll;
	char system_path[MAX_PATH];
	GetSystemDirectoryA(system_path, sizeof(system_path));

	char xinput_path[MAX_PATH];
	PathCombineA(xinput_path, system_path, "xinput1_3.dll");
	if (FileExists(xinput_path))
	{
		xinput_dll = LoadLibraryA(xinput_path);
	}
	else
	{
		PathCombineA(xinput_path, system_path, "xinput1_4.dll");
		xinput_dll = LoadLibraryA(xinput_path);
	}
	assert(xinput_dll);

	//Assign it to getControllerData for easier use
	g_input.XInputGetState = (get_gamepad_ex)GetProcAddress(xinput_dll, (LPCSTR)100);
	g_input.layout = GetKeyboardLayout(0);

	YaffeTime time = {};

	//Game loop
	MSG msg;
	g_state.is_running = true;
	while (g_state.is_running)
	{
		LARGE_INTEGER i2;
		QueryPerformanceCounter(&i2);
		__int64 start = i2.QuadPart;

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) g_state.is_running = false;
		}

		Win32GetInput(&g_input, g_state.form->handle);
		Tick(&time);

		UpdateUI(&g_state, time.delta_time);

		v2 size = V2(g_state.form->width, g_state.form->height);
		ProcessTaskCallbacks(&g_state.callbacks);
		BeginRenderPass(size, &render_state);
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderUI(&g_state, &render_state, g_assets);

		EndRenderPass(size, &render_state);
		SwapBuffers(g_state.form);

		UpdateOverlay(&g_state.overlay, time.delta_time);
		RenderOverlay(&g_state, &render_state);

		{//Sleep until FPS is hit
			LARGE_INTEGER query_counter;
			QueryPerformanceCounter(&query_counter);
			double update_seconds = double(query_counter.QuadPart - start) / computer_frequency;

			if (update_seconds < ExpectedSecondsPerFrame)
			{
				int sleep_time = (int)((ExpectedSecondsPerFrame - update_seconds) * 1000);
				Sleep(sleep_time);
			}
		}
	}

	ShutdownYaffeService(g_state.service);
	FreeAllAssets(g_assets);
	DisposeRenderState(&render_state);
	DestroyGlWindow(g_state.form);
	CloseHandle(work_queue.Semaphore);
	UnregisterClassW(WINDOW_CLASS, hInstance);
	UnregisterClassW(OVERLAY_CLASS, hInstance);

	YaffeLogInfo("Exiting \n\n");
	CloseLogger();

	return 0;
}
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
		{
			g_state.form->width = 0;
			g_state.form->height = 0;
		}
		else
		{
			g_state.form->width = (float)(lParam & 0xFFFF);
			g_state.form->height = (float)((lParam >> 16) & 0xFFFF);
			if (g_state.is_running)
			{
				for (u32 i = 0; i < FONT_COUNT; i++)
				{
					FreeAsset(g_assets->fonts + i);
				}
			}
		}

		glViewport(0, 0, (int)g_state.form->width, (int)g_state.form->height);
		return 0;
	}

	case WM_ACTIVATE:
	{
		//Scale down FPS to 10 when not focused
		UPDATE_FREQUENCY = wParam == WA_INACTIVE ? 15 : 30;
		ExpectedSecondsPerFrame = 1.0F / UPDATE_FREQUENCY;
	}

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}