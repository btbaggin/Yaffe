#pragma comment(lib, "XInput.lib") 

/*
TODO
Don't hardcode emulator allocation count
Don't hardcode Widget children count
High prio background queue
*/

#define GLEW_STATIC
#define NOMINMAX
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

#include <json.h>
#include "../Yaffe.h"
#include "../Logger.cpp"
#include "../RestrictedMode.h"
#include "../Memory.h"
#include "../Assets.h"
#include "Win32Input.h"
#include "../Render.h"
#include "../Interface.h"
#include "../Platform.h"
#include "../Database.h"
#include "../Server.h"
#include "TextureAtlas.h"
#include "../WorkQueue.h"

struct PlatformWindow
{
	HWND handle;
	HGLRC rc;
	HDC dc;struct win32_thread_info
{
	u32 ThreadIndex;
	WorkQueue* queue;
};

	char path[MAX_PATH];
};

struct PlatformProcess
{
	HANDLE handle;
	DWORD id;
	DWORD thread_id;
	char path[MAX_PATH];
};

YaffeState g_state = {};
YaffeInput g_input = {};
Assets* g_assets;
Interface g_ui = {};

#include "../Memory.cpp"
#include "../Assets.cpp"
#include "../Input.cpp"
#include "../Render.cpp"
#include "../UiElements.cpp"
#include "../Modal.cpp"
#include "../ListModal.cpp"
#include "../PlatformDetailModal.cpp"
#include "../YaffeSettingsModal.cpp"
#include "Win32Server.cpp"
#include "../Database.cpp"
#include "../Platform.cpp"
#include "../Interface.cpp"
#include "../YaffeOverlay.cpp"
#include "../RestrictedMode.cpp"

#include "Win32YaffePlatform.cpp"

//
// Main Yaffe loop
//
const LPCWSTR WINDOW_CLASS = L"Yaffe";
const LPCWSTR OVERLAY_CLASS = L"Overlay";

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
		pForm->width = (float)(rect.right - rect.left);
		pForm->height = (float)(rect.bottom - rect.top);

		RECT primaryDisplaySize;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &primaryDisplaySize, 0);	// system taskbar and application desktop toolbars not included
		x = (u32)((float)primaryDisplaySize.right - pForm->width) / 2;
		y = (u32)((float)primaryDisplaySize.bottom - pForm->height) / 2;
	}

	pForm->platform->handle = CreateWindowW(WINDOW_CLASS, pTitle, style, x, y, (int)pForm->width, (int)pForm->height, NULL, NULL, hInstance, NULL);
	pForm->platform->dc = GetDC(pForm->platform->handle);

	if (pFullscreen)
	{
		SetWindowLong(pForm->platform->handle, GWL_STYLE, wcex.style & ~(WS_CAPTION | WS_THICKFRAME));

		// On expand, if we're given a window_rect, grow to it, otherwise do
		// not resize.
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(MonitorFromWindow(pForm->platform->handle, MONITOR_DEFAULTTONEAREST), &mi);
		pForm->width = (float)(mi.rcMonitor.right - mi.rcMonitor.left);
		pForm->height = (float)(mi.rcMonitor.bottom - mi.rcMonitor.top);

		SetWindowPos(pForm->platform->handle, NULL, mi.rcMonitor.left, mi.rcMonitor.top,
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
	const bool status = wglChoosePixelFormatARB(pForm->platform->dc, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	if (!status || !numFormats) return false;

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(pForm->platform->dc, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(pForm->platform->dc, pixelFormatID, &PFD);

	const int major_min = 4, minor_min = 0;
	const int contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, major_min,
		WGL_CONTEXT_MINOR_VERSION_ARB, minor_min,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		//		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				0
	};


	pForm->platform->rc = wglCreateContextAttribsARB(pForm->platform->dc, 0, contextAttribs);
	if (!pForm->platform->rc) return false;

	// delete temporary context and window
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeRc);
	ReleaseDC(fakeHwnd, fakeDc);
	DestroyWindow(fakeHwnd);

	if (!wglMakeCurrent(pForm->platform->dc, pForm->platform->rc)) return false;

	return true;
}

static bool CreateOverlayWindow(Form* pOverlay, HINSTANCE hInstance)
{
	WNDCLASSW wcex = {};
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = OverlayWndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = OVERLAY_CLASS;
	RegisterClassW(&wcex);

	// | WS_EX_LAYERED
	pOverlay->platform->handle = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, OVERLAY_CLASS, L"Yaffe Overlay", WS_POPUP, 0, 0, (int)pOverlay->width, (int)pOverlay->height, NULL, NULL, hInstance, NULL);
	pOverlay->platform->dc = GetDC(pOverlay->platform->handle);

	DWM_BLURBEHIND bb = { 0 };
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.fEnable = true;
	bb.hRgnBlur = CreateRectRgn(0, 0, 1, 1);
	if (!SUCCEEDED(DwmEnableBlurBehindWindow(pOverlay->platform->handle, &bb))) return false;

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
	const bool status = wglChoosePixelFormatARB(pOverlay->platform->dc, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	if (!status || !numFormats) return false;

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(pOverlay->platform->dc, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(pOverlay->platform->dc, pixelFormatID, &PFD);

	return true;
}

static void DestroyGlWindow(PlatformWindow* pForm)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(pForm->rc);
	ReleaseDC(pForm->handle, pForm->dc);
	DestroyWindow(pForm->handle);
}

DWORD WINAPI ThreadProc(LPVOID pParameter)
{
	ThreadInfo* thread = (ThreadInfo*)pParameter;
	while (true)
	{
		if (DoNextWorkQueueEntry(thread->queue))
		{
			WaitForSingleObjectEx(thread->queue->Semaphore, INFINITE, false);
		}
	}
}

void Win32GetInput(YaffeInput* pInput, HWND pHandle)
{
	memcpy(pInput->previous_keyboard_state, pInput->current_keyboard_state, INPUT_SIZE);
	pInput->previous_controller_buttons = pInput->current_controller_buttons;

	GetKeyboardState((PBYTE)pInput->current_keyboard_state);

	POINT point;
	GetCursorPos(&point);
	ScreenToClient(pHandle, &point);

	pInput->mouse_position = V2((float)point.x, (float)point.y);

	XINPUT_GAMEPAD_EX state = {};
	DWORD result = g_input.platform->XInputGetState(0, &state);
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

	Form form = {};
	PlatformWindow platform = {};
	g_state.form = &form;
	g_state.form->platform = &platform;
	if (!CreateOpenGLWindow(&form, hInstance, 720, 480, L"Yaffe", true))
	{
		MessageBoxA(nullptr, "Unable to initialize window", "Error", MB_OK);
		return 1;
	}
	ShowWindow(g_state.form->platform->handle, nCmdShow);
	UpdateWindow(g_state.form->platform->handle);

	Form overlay = {};
	PlatformWindow overlay_platform = {};
	g_state.overlay.form = &overlay;
	g_state.overlay.form->platform = &overlay_platform;
	if (!CreateOverlayWindow(&overlay, hInstance))
	{
		MessageBoxA(nullptr, "Unable to initialize overlay", "Error", MB_OK);
		return 1;
	}

	//Set up work queue
	const u32 THREAD_COUNT = 4;
	ThreadInfo threads[THREAD_COUNT];
	WorkQueue work_queue = {};
	work_queue.Semaphore = CreateSemaphoreEx(0, 0, THREAD_COUNT, 0, 0, SEMAPHORE_ALL_ACCESS);
	for (u32 i = 0; i < THREAD_COUNT; i++)
	{
		ThreadInfo* thread = threads + i;
		thread->ThreadIndex = i;
		thread->queue = &work_queue;

		DWORD id;
		HANDLE t = CreateThread(0, 0, ThreadProc, thread, 0, &id);
		CloseHandle(t);
	}
	g_state.work_queue = &work_queue;

	LARGE_INTEGER i;
	QueryPerformanceFrequency(&i);
	double computer_frequency = (double)i.QuadPart;

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
	if (!InitializeRenderer(&render_state))
	{
		CloseLogger();
		return 1;
	}
	
	if (!LoadAssets(asset_memory, asset_size, &g_assets))
	{
		CloseLogger();
		return 1;
	}

	InitializeUI(&g_state, &g_ui);
	InitailizeDatbase(&g_state);

	g_state.has_connection = InternetGetConnectedState(0, 0);
	InitYaffeService();
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

	PlatformInput input;
	input.XInputGetState = (get_gamepad_ex)GetProcAddress(xinput_dll, (LPCSTR)100);
	input.layout = GetKeyboardLayout(0);
	g_input.platform = &input;

	YaffeTime time = {};
	g_state.restrictions = new RestrictedMode();

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

		Win32GetInput(&g_input, g_state.form->platform->handle);
		Tick(&time);

		UpdateUI(&g_state, time.delta_time);
		ProcessTaskCallbacks(&g_state.callbacks);

		v2 size = BeginRenderPassAndClear(g_state.form, &render_state);

		RenderUI(&g_state, &render_state, g_assets);		
		if (!g_state.overlay.showing && g_state.restrictions->modal) RenderModalWindow(&render_state, g_state.restrictions->modal, g_state.form);

		EndRenderPass(size, &render_state, g_state.form->platform);

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

	ShutdownYaffeService();
	FreeAllAssets(g_assets);
	DisposeRenderState(&render_state);
	DestroyGlWindow(g_state.form->platform);
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
		break;
	}

	case WM_ACTIVATE:
	{
		//Scale down FPS to 10 when not focused
		UPDATE_FREQUENCY = wParam == WA_INACTIVE ? 15 : 30;
		ExpectedSecondsPerFrame = 1.0F / (float)UPDATE_FREQUENCY;
		break;
	}

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}