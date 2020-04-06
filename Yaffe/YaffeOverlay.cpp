HWND g_HWND;
BOOL CALLBACK GetRunningRomHwnd(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == lParam)
	{
		g_HWND = hwnd;
		return FALSE;
	}
	return TRUE;
}

class YaffeOverlay : public UiElement
{
public:
	YaffeOverlay() : UiElement(V2(1), UI_Overlay) {}

private:
	bool showing_overlay = false;
	void Render(RenderState* pState)
	{
		if (showing_overlay)
		{
			PushQuad(pState, V2(0), GetAbsoluteSize(), MENU_BACKGROUND);

			return;
		}
	}

	static MODAL_CLOSE(OverlayModalClose)
	{ 
		if (pResult == MODAL_RESULT_Ok && IsApplicationFocused())
		{
			TerminateProcess(g_state.running_rom.hProcess, 0);
			WaitForSingleObject(g_state.running_rom.hProcess, INFINITE);
			CloseHandle(g_state.running_rom.hProcess);
			CloseHandle(g_state.running_rom.hThread);

			RevertFocus();
		}
	}
	void Update(float pDeltaTime)
	{
		if (IsControllerPressed(CONTROLLER_BACK) || IsKeyPressed(KEY_Escape))
		{
			showing_overlay = !showing_overlay;
			HWND handle;
			if (showing_overlay)
			{
				DisplayModalWindow(&g_state, "Are you sure you wish to quit?", BITMAP_Question, OverlayModalClose);
				handle = g_state.form.handle;
			}
			else
			{
				EnumWindows(GetRunningRomHwnd, g_state.running_rom.dwProcessId);
				handle = g_HWND;
			}

			HWND hCurWnd = ::GetForegroundWindow();
			DWORD dwMyID = ::GetCurrentThreadId();
			DWORD dwCurID = ::GetWindowThreadProcessId(hCurWnd, NULL);
			::AttachThreadInput(dwCurID, dwMyID, TRUE);
			::SetWindowPos(handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			::SetWindowPos(handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
			::SetForegroundWindow(handle);
			::SetFocus(handle);
			::SetActiveWindow(handle);
			::AttachThreadInput(dwCurID, dwMyID, FALSE);
			ShowWindow(handle, SW_RESTORE);
		}

		//Check if the program closed without going through the overlay
		if (WaitForSingleObject(g_state.running_rom.hProcess, 100) == 0)
		{
			RevertFocus();
		}
	}

	void OnFocusLost()
	{
		showing_overlay = false;
	}
};