#pragma comment(lib, "XInput.lib") 

/*
TODO
Changed back to holding start(?) for X seconds?
Figure out scraper stuff?
Don't hardcode emulator allocation count
*/

#define GLEW_STATIC
#include <glew\glew.h>
#include <gl/GL.h>
#include "gl/wglext.h"
#include "sqlite/sqlite3.h"
#include <Shlwapi.h>
#include <dwmapi.h>
#include <mmreg.h>
#include "intrin.h"
#include <assert.h>
#include <Xinput.h>
#include <algorithm>
#include <vector>

#include "Yaffe.h"
#include "Assets.h"
#include "Input.h"
#include "Interface.h"
#include "Emulators.h"
#include "Render.h"

YaffeState g_state = {};
YaffeInput g_input = {};
Assets* g_assets;

Interface g_ui = {};

#include "Memory.cpp"
#include "Assets.cpp"
#include "Input.cpp"
#include "Render.cpp"
#include "Database.cpp"
#include "Emulators.cpp"
#include "Interface.cpp"
#include "YaffeOverlay.cpp"

#define WINDOW_CLASS L"Yaffe"
#define OVERLAY_CLASS L"Overlay"
const u32 UPDATE_FREQUENCY = 30;
const float ExpectedSecondsPerFrame = 1.0F / UPDATE_FREQUENCY;

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CreateOpenGLWindow(Form* pForm, HINSTANCE hInstance, u32 pWidth, u32 pHeight, LPCWSTR pTitle, bool pFullscreen)
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
		pForm->width = (u32)(rect.right - rect.left);
		pForm->height = (u32)(rect.bottom - rect.top);

		RECT primaryDisplaySize;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &primaryDisplaySize, 0);	// system taskbar and application desktop toolbars not included
		x = (u32)(primaryDisplaySize.right - pForm->width) / 2;
		y = (u32)(primaryDisplaySize.bottom - pForm->height) / 2;
	}

	pForm->handle = CreateWindowW(WINDOW_CLASS, pTitle, style, x, y, pForm->width, pForm->height, NULL, NULL, hInstance, NULL);
	pForm->dc = GetDC(pForm->handle);

	if (pFullscreen)
	{
		SetWindowLong(pForm->handle, GWL_STYLE, wcex.style & ~(WS_CAPTION | WS_THICKFRAME));

		// On expand, if we're given a window_rect, grow to it, otherwise do
		// not resize.
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(pForm->handle, MONITOR_DEFAULTTONEAREST), &mi);
		pForm->width = mi.rcMonitor.right - mi.rcMonitor.left;
		pForm->height = mi.rcMonitor.bottom - mi.rcMonitor.top;

		SetWindowPos(pForm->handle, NULL, mi.rcMonitor.left, mi.rcMonitor.top,
			pForm->width, pForm->height,
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
	pOverlay->handle = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, OVERLAY_CLASS, L"Yaffe Overlay", WS_POPUP, 0, 0, pOverlay->width, pOverlay->height, NULL, NULL, hInstance, NULL);
	pOverlay->dc = GetDC(pOverlay->handle);

	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.fEnable = true;
	bb.hRgnBlur = CreateRectRgn(0, 0, 1, 1);
	if (!SUCCEEDED(DwmEnableBlurBehindWindow(pOverlay->handle, &bb))) return false;

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
	const bool status = wglChoosePixelFormatARB(pOverlay->dc, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	if (!status || !numFormats) return false;

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(pOverlay->dc, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(pOverlay->dc, pixelFormatID, &PFD);

	pOverlay->showing = false;

	return true;
}

static void DestroyGlWindow(Form* pForm)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(pForm->rc);
	ReleaseDC(pForm->handle, pForm->dc);
	DestroyWindow(pForm->handle);
}

static void DisplayErrorMessage(const char* pError, ERROR_TYPE pType)
{
	assert(g_state.error_count < MAX_ERROR_COUNT);

	g_state.errors[g_state.error_count++] = pError;
	if (pType == ERROR_TYPE_Error) g_state.error_is_critical = true;
}

bool QueueUserWorkItem(WorkQueue* pQueue, work_queue_callback* pCallback, void* pData)
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

bool Win32DoNextWorkQueueEntry(WorkQueue* pQueue)
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

void Win32CompleteAllWork(WorkQueue* pQueue)
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

inline void Tick(YaffeTime* pTime)
{
	auto current_time = clock();
	pTime->delta_time = (current_time - pTime->current_time) / (float)CLOCKS_PER_SEC;
	pTime->current_time = current_time;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
{
	if (!CreateOpenGLWindow(&g_state.form, hInstance, 720, 480, L"Yaffe", true))
	{
		MessageBoxA(nullptr, "Unable to initialize window", "Error", MB_OK);
		return 1;
	}
	ShowWindow(g_state.form.handle, nCmdShow);
	UpdateWindow(g_state.form.handle);

	if (!CreateOverlayWindow(&g_state.overlay, hInstance))
	{
		MessageBoxA(nullptr, "Unable to initialize overlay", "Error", MB_OK);
		return 1;
	}

	//Set up work queue
	const u32 THREAD_COUNT = 8;
	win32_thread_info threads[THREAD_COUNT];
	WorkQueue queue = {};
	HANDLE sem = CreateSemaphoreEx(0, 0, THREAD_COUNT, 0, 0, SEMAPHORE_ALL_ACCESS);
	queue.Semaphore = sem;
	for (u32 i = 0; i < THREAD_COUNT; i++)
	{
		win32_thread_info* thread = threads + i;
		thread->queue = &queue;
		thread->ThreadIndex = i;

		DWORD id;
		HANDLE t = CreateThread(0, 0, ThreadProc, thread, 0, &id);
		CloseHandle(t);
	}
	g_state.queue = &queue;

	LARGE_INTEGER i;
	QueryPerformanceFrequency(&i);
	double computer_frequency = double(i.QuadPart);

	glewExperimental = true; // Needed for core profile
	glewInit();

	//Initialization
	void* asset_memory = malloc(Megabytes(4));

	RenderState render_state = {};
	InitializeRenderer(&render_state);
	InitializeAssetFiles();
	g_assets = LoadAssets(asset_memory, Megabytes(4));
	InitializeUI(&g_state);
	GetConfiguredEmulators(&g_state);
	GetRoms(&g_state, GetSelectedEmulator());


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

	//Get the address of ordinal 100.
	FARPROC lpfnGetProcessID = GetProcAddress(xinput_dll, (LPCSTR)100); // load ordinal 100

	//Assign it to getControllerData for easier use
	g_input.XInputGetState = (get_gamepad_ex)lpfnGetProcessID;


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

		Win32GetInput(&g_input, g_state.form.handle);
		Tick(&g_state.time);

		UpdateUI(&g_state, g_state.time.delta_time);

		v2 size = V2((float)g_state.form.width, (float)g_state.form.height);
		ProcessTaskCallbacks(&g_state.callbacks);
		BeginRenderPass(size, &render_state);
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderUI(&g_state, &render_state, g_assets);

		EndRenderPass(size, &render_state);
		SwapBuffers(g_state.form.dc);

		UpdateOverlay(&g_state.overlay);
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

	FreeAllAssets(&g_state, g_assets);
	DisposeRenderState(&render_state);
	DestroyGlWindow(&g_state.form);
	CloseHandle(queue.Semaphore);
	UnregisterClassW(WINDOW_CLASS, hInstance);
	UnregisterClassW(OVERLAY_CLASS, hInstance);

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
			g_state.form.width = 0;
			g_state.form.height = 0;
		}
		else
		{
			g_state.form.width = lParam & 0xFFFF;
			g_state.form.height = (lParam >> 16) & 0xFFFF;
			if (g_state.is_running)
			{
				for (u32 i = 0; i < FONT_COUNT; i++)
				{
					FreeAsset(g_assets->fonts + i);
				}
			}
		}

		glViewport(0, 0, g_state.form.width, g_state.form.height);
		return 0;
	}

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}