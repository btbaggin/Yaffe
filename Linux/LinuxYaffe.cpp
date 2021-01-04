/*
	Currently required utilties: zentity, wget?
*/

#include <stdlib.h>
#include <string.h>
#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <GL/glew.h>
#include <GL/glx.h>
#include <stdio.h>
#include <sys/stat.h>
#include "x86intrin.h"
#include "pthread.h"
#include <semaphore.h>
#include <unistd.h>
#include <sys/reboot.h>
#include "dirent.h"
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

struct PlatformWindow
{
	Display* display;
    Window window;
    XVisualInfo* vis;
	GLXContext context;
};

#include "json.h"
#include "../Yaffe.h"
#include "LinuxInput.h"
#include "../RestrictedMode.h"
#include "../Memory.h"
#include "../Assets.h"
#include "../Render.h"
#include "../Interface.h"
#include "../Platform.h"
#include "../Database.h"
#include "../Server.h"
#include "TextureAtlas.h"
#include "../WorkQueue.h"

YaffeState g_state = {};
YaffeInput g_input = {};
Assets* g_assets;
Interface g_ui = {};

struct PlatformProcess
{
	pid_t id;
};

#include "../Logger.cpp"
#include "../Memory.cpp"
#include "../Assets.cpp"
#include "../Input.cpp"
#include "../Render.cpp"
#include "../UiElements.cpp"
#include "../Modal.cpp"
#include "../ListModal.cpp"
#include "../PlatformDetailModal.cpp"
#include "../YaffeSettingsModal.cpp"
#include "LinuxServer.cpp"
#include "../Database.cpp"
#include "../Platform.cpp"
#include "../Interface.cpp"
#include "../YaffeOverlay.cpp"
#include "../RestrictedMode.cpp"

#include "LinuxYaffePlatform.cpp"

/*
*/
void* ThreadProc(void* pParameter)
{
	ThreadInfo* thread = (ThreadInfo*)pParameter;
	while (true)
	{
		if (DoNextWorkQueueEntry(thread->queue))
		{
			sem_wait(&thread->queue->Semaphore);
		}
	}
}

//https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
static bool CreateOpenGLWindow(Form* pForm)
{
	Display *display = XOpenDisplay(NULL);
	pForm->platform->display = display;

	if (!display) return false;

	// Get a matching FB config
	static int visual_attribs[] =
		{
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
		};


	int fbcount;
	GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
	if (!fbc) return false;

	// Pick the FB config/visual with the most samples per pixel
	int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
	for (int i = 0; i < fbcount; i++)
	{
		XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
		if (vi)
		{
			int samp_buf, samples;
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );
			
			if (best_fbc < 0 || samp_buf && samples > best_num_samp) best_fbc = i, best_num_samp = samples;
			if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp) worst_fbc = i, worst_num_samp = samples;
		}
		XFree( vi );
	}

	GLXFBConfig bestFbc = fbc[best_fbc];

	// Be sure to free the FBConfig list allocated by glXChooseFBConfig()
	XFree(fbc);

	// Get a visual
	XVisualInfo *vi = glXGetVisualFromFBConfig(display, bestFbc);
	pForm->platform->vis = vi;

	Window root = RootWindow(display, vi->screen);

	XSetWindowAttributes swa;
	swa.colormap = XCreateColormap(display, root, vi->visual, AllocNone);
	swa.background_pixmap = None;
	swa.border_pixel      = 0;
	swa.event_mask        = StructureNotifyMask|ButtonPress|ButtonReleaseMask;
	swa.override_redirect = true;

	pForm->width = XDisplayWidth(pForm->platform->display, 0);
    pForm->height = XDisplayHeight(pForm->platform->display, 0);
	Window win = XCreateWindow( display, root,
								0, 0, pForm->width, pForm->height, 0, vi->depth, InputOutput, 
								vi->visual, 
								CWBorderPixel|CWColormap|CWEventMask, &swa );
	pForm->platform->window = win;
	if (!win) return false;

	Atom atoms[2] = { XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False), None };
	XChangeProperty(
		display, 
		win, 
		XInternAtom(display, "_NET_WM_STATE", False),
		XA_ATOM, 32, PropModeReplace, (unsigned char *)atoms, 1
	);

	// Done with the visual info data
	XFree(vi);

	XStoreName(display, win, "Yaffe");
	XMapWindow(display, win);

	// Get the default screen's GLX extension list
	const char *glxExts = glXQueryExtensionsString(display, DefaultScreen(display));

	// NOTE: It is not necessary to create or make current to a context before
	// calling glXGetProcAddressARB
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
																glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

	// Check for the GLX_ARB_create_context extension string and the function.
	// If either is not present, use GLX 1.3 context creation method.
	if (!glXCreateContextAttribsARB)
	{
		printf("glXCreateContextAttribsARB() not found ... using old-style GLX context\n");
		pForm->platform->context = glXCreateNewContext(display, bestFbc, GLX_RGBA_TYPE, 0, True);
	}
	else // If it does, try to get a GL 3.0 context!
	{
		int context_attribs[] =
		{
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 2,
			//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			None
		};

		pForm->platform->context = glXCreateContextAttribsARB(display, bestFbc, 0, True, context_attribs);

		// Sync to ensure any errors generated are processed.
		XSync(display, False);
		if (!pForm->platform->context)
		{
			// Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
			// When a context version below 3.0 is requested, implementations will
			// return the newest context version compatible with OpenGL versions less
			// than version 3.0.
			// GLX_CONTEXT_MAJOR_VERSION_ARB = 1
			context_attribs[1] = 1;
			// GLX_CONTEXT_MINOR_VERSION_ARB = 0
			context_attribs[3] = 0;

			// ctxErrorOccurred = false;

			printf("Failed to create GL 3.0 context ... using old-style GLX context\n");
			pForm->platform->context = glXCreateContextAttribsARB(display, bestFbc, 0, True, context_attribs);
		}
	}

	// Sync to ensure any errors generated are processed.
	XSync(display, False);

	if (!pForm->platform->context) return false;

	glXMakeCurrent(display, win, pForm->platform->context);
    return true;
}

static bool CreateOverlayWindow(Form* pOverlay)
{
	Display* display = XOpenDisplay(NULL);
	pOverlay->platform->display = display;

	static int visual_attribs[] = {
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
		};

	int fbcount;
	GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
	if (!fbc) return false;

	// Pick the FB config/visual with the most samples per pixel
	int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
	for (int i = 0; i < fbcount; i++)
	{
		XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
		if (vi)
		{
			int samp_buf, samples;
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );
			
			if (best_fbc < 0 || samp_buf && samples > best_num_samp) best_fbc = i, best_num_samp = samples;
			if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp) worst_fbc = i, worst_num_samp = samples;
		}
		XFree( vi );
	}

	GLXFBConfig bestFbc = fbc[best_fbc];

	XVisualInfo *vi = glXGetVisualFromFBConfig(display, bestFbc);
	pOverlay->platform->vis = vi;

	Window root = RootWindow(display, vi->screen);

	XSetWindowAttributes swa;
	swa.colormap = XCreateColormap(display, root, vi->visual, AllocNone);
	swa.background_pixmap = None;
	swa.border_pixel      = 0;
	swa.background_pixel = 0;
	//swa.event_mask        = StructureNotifyMask|ButtonPress|ButtonReleaseMask;
	swa.override_redirect = true;

	float width = XDisplayWidth(pOverlay->platform->display, 0);
    float height = XDisplayHeight(pOverlay->platform->display, 0);
	pOverlay->platform->window = XCreateWindow( display, root,
								0, 0, pOverlay->width, pOverlay->height, 0, vi->depth, InputOutput, 
								vi->visual, 
								CWColormap|CWBorderPixel|CWBackPixel, &swa );

	// Atom atoms[2] = { XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False), None };
	// XChangeProperty(
	// 	display, 
	// 	pOverlay->platform->window, 
	// 	XInternAtom(display, "_NET_WM_STATE", False),
	// 	XA_ATOM, 32, PropModeReplace, (unsigned char *)atoms, 1
	// );

	//Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    //XSetWMProtocols(display, pOverlay->platform->window, &wm_delete_window, 1);
	return true;
}

static inline void timespec_diff(struct timespec *a, struct timespec *b,
    struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

void LinuxGetInput(YaffeInput* pInput, PlatformWindow* pDisplay, int pPress, int pRelease)
{
    memcpy(pInput->previous_keyboard_state, pInput->current_keyboard_state, INPUT_SIZE);

    XQueryKeymap(pDisplay->display, pInput->current_keyboard_state);

	pInput->current_keyboard_state[INPUT_SIZE - 1] |= pPress;
	pInput->current_keyboard_state[INPUT_SIZE - 1] &= ~pRelease;

	int win_x, win_y, root_x, root_y = 0;
    unsigned int mask = 0;
    Window child_win, root_win;
	XQueryPointer(pDisplay->display, pDisplay->window, &child_win, &root_win, &root_x, &root_y, &win_x, &win_y, &mask);
	pInput->mouse_position = V2(root_x, root_y);

	if(CheckForJoystick(pInput->platform, "/dev/input/js0"))
	{
		GetJoystickInput(pInput, pInput->platform->joystick);
	}
}

int main(void)
{
    InitializeLogger();

    Form form = {};
	PlatformWindow platform = {};
	g_state.form = &form;
	g_state.form->platform = &platform;

    if(!CreateOpenGLWindow(&form))
    {
        YaffeLogError("unable to initialize window");
		CloseLogger();
        return 1;
    }
	
    glEnable(GL_DEPTH_TEST);

	Form overlay = {};
	PlatformWindow overlay_platform = {};
	g_state.overlay.form = &overlay;
	g_state.overlay.form->platform = &overlay_platform;
	if (!CreateOverlayWindow(&overlay))
	{
		YaffeLogError("Unable to initialize overlay");
	 	return 1;
	}

	// //Set up work queue
	const u32 THREAD_COUNT = 4;
	ThreadInfo threads[THREAD_COUNT];
	WorkQueue work_queue = {};
	sem_init(&work_queue.Semaphore, 0, 0);    struct js_event jsEvent;

	for (u32 i = 0; i < THREAD_COUNT; i++)
	{
		ThreadInfo* thread = threads + i;
		thread->ThreadIndex = i;
		thread->queue = &work_queue;

		pthread_t id;
		if (pthread_create(&id, NULL, ThreadProc, thread) != 0)
		{
			YaffeLogError("Unable to start worker thread");
		}
	}
	g_state.work_queue = &work_queue;

	// //Check if we need admin privileges to write to the current directory
	// char base_path[MAX_PATH];
	// GetFullPath(".", base_path);
	// CombinePath(base_path, base_path, "lock");
	// HANDLE h = CreateFileA(base_path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	// if (h == INVALID_HANDLE_VALUE)
	// {
	// 	DWORD error = GetLastError();
	// 	if (error == ERROR_ACCESS_DENIED) MessageBoxA(nullptr, "Administrator privileges required", "Error", MB_OK);
	// 	else MessageBoxA(nullptr, "Unable to create required directories", "Error", MB_OK);
	// 	return 0;
	// }
	// else
	// {
	// 	CloseHandle(h);
	// 	DeleteFileA(base_path);
	// }

	// //Initialization
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


	PlatformInput input;
	input.joystick = -1;
	g_input.platform = &input;

	InitializeUI(&g_state, &g_ui);
	InitailizeDatbase(&g_state);

	// g_state.has_connection = InternetGetConnectedState(0, 0);
	g_state.service = new PlatformService();
	InitYaffeService(g_state.service);
	GetAllPlatforms(&g_state);

	YaffeTime time = {};
	g_state.restrictions = new RestrictedMode();

	// //Game loop
	// MSG msg;
	g_state.is_running = true;
	while (g_state.is_running)
	{
		timespec start;
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);

		int press_mask = 0, release_mask = 0;
		//https://github.com/glfw/glfw/blob/1adfbde4d7fb862bb36d4a20e05d16bf712170f3/src/x11_window.c#L1156
		while (QLength(g_state.form->platform->display))
		{
			XEvent event;
			XNextEvent(g_state.form->platform->display, &event);
			switch(event.type)
			{
				case ButtonPress:
					switch (event.xbutton.button) 
					{
						case 1:
							press_mask |= BUTTON_Left;
							break;
						case 2:
							press_mask |= BUTTON_Middle;
							break;
						case 3:
							press_mask |= BUTTON_Right;
							break;
					}
					break;

                case ButtonRelease:
					switch (event.xbutton.button) 
					{
						case 1:
							release_mask |= BUTTON_Left;
							break;
						case 2:
							release_mask |= BUTTON_Middle;
							break;
						case 3:
							release_mask |= BUTTON_Right;
							break;
					}
					break;
			}
			//processEvent(&event);
		}

		LinuxGetInput(&g_input, form.platform, press_mask, release_mask);
		Tick(&time);

	 	UpdateUI(&g_state, time.delta_time);
		ProcessTaskCallbacks(&g_state.callbacks);

		v2 size = BeginRenderPassAndClear(g_state.form, &render_state);

		RenderUI(&g_state, &render_state, g_assets);		
		if (!g_state.overlay.showing && g_state.restrictions->modal) RenderModalWindow(&render_state, g_state.restrictions->modal, g_state.form);

		EndRenderPass(size, &render_state, g_state.form->platform);

		UpdateOverlay(&g_state.overlay, time.delta_time);
		RenderOverlay(&g_state, &render_state);

		// {//Sleep until FPS is hit
		// 	timespec end;
		// 	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
			
		// 	timespec result;
		// 	timespec_diff(&end, &start, &result);
		// 	double update_seconds = result.tv_sec + result.tv_nsec / 1000000000.0;

		// 	if (update_seconds < ExpectedSecondsPerFrame)
		// 	{
		// 		nanosleep(NULL, result); //TODO not right
		// 	}
		// }
	}

	ShutdownYaffeService(g_state.service);
	FreeAllAssets(g_assets);
	DisposeRenderState(&render_state);
	sem_destroy(&work_queue.Semaphore);

    glXMakeCurrent(form.platform->display, None, NULL);
 	glXDestroyContext(form.platform->display, form.platform->context);
 	XDestroyWindow(form.platform->display, form.platform->window);
 	XCloseDisplay(form.platform->display);
    
    // XWindowAttributes gwa;
    // XEvent xev;
    // while(1)
    // {
    //     XNextEvent(dpy, &xev);
    //     if(xev.type == Expose) {
    //         printf("Window shown");
    //     	XGetWindowAttributes(dpy, win, &gwa);
    //             glViewport(0, 0, gwa.width, gwa.height);
    //     	DrawAQuad(); 
    //             glXSwapBuffers(dpy, win);
    //     }
                
	// else if(xev.type == KeyPress) {
    //         printf("keypress");
    //     	glXMakeCurrent(dpy, None, NULL);
 	// 	glXDestroyContext(dpy, glc);
 	// 	XDestroyWindow(dpy, win);
 	// 	XCloseDisplay(dpy);
 	// 	exit(0);
    //     }
    // }

    YaffeLogInfo("Exiting \n\n");
	CloseLogger();
    return 0;
}