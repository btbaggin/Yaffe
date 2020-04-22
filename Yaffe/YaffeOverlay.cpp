static void StartRunning(YaffeState* pState, Emulator* pEmulator, Rom* pRom)
{
	Overlay* overlay = &pState->overlay;
	char* path = pRom->path;
	char safe_path[1000];
	//We allow passing command line args in with the path, if the path is wrapped in quotes
	if (pEmulator->start_path[0] == '"') sprintf(safe_path, "%s \"%s\"", pEmulator->start_path, path);
	else sprintf(safe_path, "\"%s\" \"%s\"", pEmulator->start_path, path);

	STARTUPINFOA si = {};
	if (!CreateProcessA(NULL, safe_path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &overlay->running_rom))
	{
		DWORD error = GetLastError();
		switch (error)
		{
		case 740:
			DisplayErrorMessage("Operation requires administrator permissions", ERROR_TYPE_Warning);
		default:
			DisplayErrorMessage("Unable to open rom", ERROR_TYPE_Warning);
		}
		return;
	}

	//Since we aren't on the application, it's a good time to update the database
	//We don't want to do it while the application is running because we could block it
	si = {};
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_MINIMIZE;
	PROCESS_INFORMATION pi = {};
	CreateProcessA("YaffeScraper.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

static void CloseOverlay(Overlay* pOverlay)
{
	ShowWindow(pOverlay->handle, SW_HIDE);
	pOverlay->running_rom = {};
	pOverlay->showing = false;
}

static void UpdateOverlay(Overlay* pOverlay)
{
	if (pOverlay->running_rom.dwProcessId != 0)
	{
		if (IsControllerPressed(CONTROLLER_GUIDE) || IsKeyPressed(KEY_Escape))
		{
			pOverlay->showing = !pOverlay->showing;

			if (pOverlay->showing)
			{
				MONITORINFO mi = { sizeof(mi) };
				GetMonitorInfo(MonitorFromWindow(pOverlay->handle, MONITOR_DEFAULTTONEAREST), &mi);
				pOverlay->width = mi.rcMonitor.right - mi.rcMonitor.left;
				pOverlay->height = mi.rcMonitor.bottom - mi.rcMonitor.top;

				SetWindowPos(pOverlay->handle, 0, mi.rcMonitor.left, mi.rcMonitor.top, pOverlay->width, pOverlay->height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

				ShowWindow(pOverlay->handle, SW_SHOW);
				UpdateWindow(pOverlay->handle);
			}
			else
			{
				ShowWindow(pOverlay->handle, SW_HIDE);
			}
		} 
		else if (pOverlay->showing && IsEnterPressed())
		{
			TerminateProcess(pOverlay->running_rom.hProcess, 0);
			WaitForSingleObject(pOverlay->running_rom.hProcess, INFINITE);
			CloseHandle(pOverlay->running_rom.hProcess);
			CloseHandle(pOverlay->running_rom.hThread);

			CloseOverlay(pOverlay);
		}
		
		//Check if the program closed without going through the overlay
		if (WaitForSingleObject(pOverlay->running_rom.hProcess, 100) == 0)
		{
			CloseOverlay(pOverlay);
		}
	}
}

static void RenderOverlayModal(RenderState* pState, const char* pMessage)
{
	v2 content_size = MeasureString(FONT_Normal, pMessage);

	const float ICON_SIZE = 32.0F;
	const float ICON_SIZE_WITH_MARGIN = ICON_SIZE + UI_MARGIN * 2;
	v2 size = V2(UI_MARGIN * 4, UI_MARGIN * 2) + content_size;
	size.Height = max(ICON_SIZE_WITH_MARGIN, size.Height);
	size.Width += ICON_SIZE_WITH_MARGIN;

	v2 window_position = V2((g_state.overlay.width - size.Width) / 2, (g_state.overlay.height - size.Height) / 2);
	v2 icon_position = window_position + V2(UI_MARGIN * 2, UI_MARGIN); //Window + margin for window + margin for icon

	PushQuad(pState, window_position, window_position + size, MODAL_BACKGROUND);
	PushQuad(pState, window_position, window_position + V2(UI_MARGIN / 2.0F, size.Height), ACCENT_COLOR);

	Bitmap* image = GetBitmap(g_assets, BITMAP_Question);
	PushQuad(pState, icon_position, icon_position + V2(ICON_SIZE), image);
	icon_position.X += ICON_SIZE;

	PushText(pState, FONT_Normal, pMessage, icon_position, TEXT_FOCUSED);
}

static void RenderOverlay(YaffeState* pState, RenderState* pRender)
{
	if (pState->overlay.showing)
	{
		//Render to overlay window
		wglMakeCurrent(pState->overlay.dc, pState->form.rc);

		float width = (float)pState->overlay.width;
		float height = (float)pState->overlay.height;
		v2 size = V2(width, height);
		BeginRenderPass(size, pRender);
		glClearColor(1, 1, 1, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		PushQuad(pRender, V2(0.0F), V2(width, height), V4(0.0F, 0.0F, 0.0F, 0.9F));

		RenderOverlayModal(pRender, "Are you sure you wish to exit?");

		EndRenderPass(size, pRender);
		SwapBuffers(g_state.overlay.dc);

		//Switch back to normal window
		wglMakeCurrent(pState->form.dc, pState->form.rc);
	}
}