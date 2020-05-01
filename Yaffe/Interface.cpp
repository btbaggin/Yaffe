#include "Modal.cpp"
#include "SelectorModal.cpp"

static bool IsApplicationFocused()
{
	return GetForegroundWindow() == g_state.form.handle;
}

static v2 CenterText(FONTS pFont, const char* pText, v2 pBounds)
{
	v2 size = MeasureString(pFont, pText);

	return V2((pBounds.Width - size.Width) / 2.0F, (pBounds.Height - size.Height - (size.Height / 2.0F)) / 2.0F);
}

static void FocusElement(UI_ELEMENT_NAME pElement)
{
	g_ui.focus[g_ui.focus_index++] = pElement;
	g_ui.elements[pElement]->OnFocusGained();
}

static void RevertFocus()
{
	if (g_ui.focus_index > 1)
	{
		--g_ui.focus_index;
	}
}

static v4 GetFontColor(bool pFocused)
{
	return pFocused ? TEXT_FOCUSED : TEXT_UNFOCUSED;
}

static void RenderModalWindow(RenderState* pState, ModalWindow* pWindow)
{
	const float ICON_SIZE = 32.0F;
	const float ICON_SIZE_WITH_MARGIN = ICON_SIZE + UI_MARGIN * 2;
	v2 size = V2(UI_MARGIN * 4, UI_MARGIN * 2) + pWindow->content->size;
	if (pWindow->icon != BITMAP_None)
	{
		size.Height = max(ICON_SIZE_WITH_MARGIN, size.Height);
		size.Width += ICON_SIZE_WITH_MARGIN;
	}

	v2 window_position = V2((g_state.form.width - size.Width) / 2, (g_state.form.height - size.Height) / 2);
	v2 icon_position = window_position + V2(UI_MARGIN * 2, UI_MARGIN); //Window + margin for window + margin for icon

	PushQuad(pState, window_position, window_position + size, MODAL_BACKGROUND);
	PushQuad(pState, window_position, window_position + V2(UI_MARGIN / 2.0F, size.Height), ACCENT_COLOR);
	if (pWindow->icon != BITMAP_None)
	{
		Bitmap* image = GetBitmap(g_assets, pWindow->icon);
		PushQuad(pState, icon_position, icon_position + V2(ICON_SIZE), image);
		icon_position.X += ICON_SIZE;
	}
	pWindow->content->Render(pState, icon_position);
}

static bool DisplayModalWindow(YaffeState* pState, ModalContent* pContent, BITMAPS pImage, modal_window_close* pClose)
{
	_WriteBarrier();
	if (pState->current_modal < MAX_MODAL_COUNT)
	{
		s32 modal_index = InterlockedIncrement(&pState->current_modal);
		ModalWindow* modal = new ModalWindow();
		modal->icon = pImage;
		modal->content = pContent;
		modal->on_close = pClose;
		pState->modals[modal_index] = modal;
		return true;
	}
	return false;
}

static bool DisplayModalWindow(YaffeState* pState, std::string pMessage, BITMAPS pImage, modal_window_close* pClose)
{
	return DisplayModalWindow(pState, new StringModal(pMessage), pImage, pClose);
}

MODAL_CLOSE(ErrorModalClose) { if (g_state.error_is_critical) g_state.is_running = false; }
MODAL_CLOSE(ExitModalClose)
{ 
	if (pResult == MODAL_RESULT_Ok)
	{
		SelectorModal* content = (SelectorModal*)pContent;
		if (content->GetSelected() == "Quit")
		{
			g_state.is_running = false;
		}
		else
		{
			HANDLE token;
			TOKEN_PRIVILEGES tkp;
			OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);

			LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

			tkp.PrivilegeCount = 1;
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if(!AdjustTokenPrivileges(token, FALSE, &tkp, 0, NULL, 0))
			{
				DisplayErrorMessage("Unable to initiate shut down", ERROR_TYPE_Warning);
			}

			if (ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_FLAG_PLANNED))
			{
				DisplayErrorMessage("Unable to initiate show down", ERROR_TYPE_Warning);
			}
		}
	}
}

static void DisplayApplicationErrors(YaffeState* pState)
{
	//Check for and display any errors
	if (pState->error_count > 0)
	{
		std::string message;
		for (u32 i = 0; i < pState->error_count; i++)
		{
			message += std::string(pState->errors[i]);
			if (i < pState->error_count - 1) message += '\n';
		}

		DisplayModalWindow(pState, message, BITMAP_Error, ErrorModalClose);
		pState->error_count = 0;
	}
}

static void DisplayQuitMessage(YaffeState* pState)
{
	if (IsApplicationFocused() && (IsKeyPressed(KEY_Q) || IsControllerPressed(CONTROLLER_START)))
	{
		//Only allow quitting when current window is focused
		std::vector<std::string> options;
		options.push_back("Quit");
		options.push_back("Shut Down");
		DisplayModalWindow(pState, new SelectorModal(options, (char*)""), BITMAP_None, ExitModalClose);
	}
}

static UiRegion CreateRegion(v2 pSize)
{
	UiRegion region;
	region.position = V2(0);
	region.size = V2(g_state.form.width * pSize.Width, g_state.form.height * pSize.Height);

	return region;
}

static UiRegion CreateRegion(UiRegion pRegion, v2 pSize)
{
	UiRegion region;
	region.position = pRegion.position;
	region.size = pRegion.size * pSize;

	return region;
}

#include "Textbox.cpp"
TextBox mtc;
#include "EmulatorList.cpp"
#include "RomMenu.cpp"
#include "FilterBar.cpp"
#include "InfoPane.cpp"
static void RenderUI(YaffeState* pState, RenderState* pRender, Assets* pAssets)
{
	//Render background
	UiRegion main = CreateRegion(V2(1));
	Bitmap* b = GetBitmap(g_assets, BITMAP_Background);
	PushQuad(pRender, main.position, main.size, b);

	UiRegion list_region = CreateRegion(main, V2(EMU_MENU_PERCENT, 1));
	RenderElement(EmulatorList, UI_Emulators, list_region);

	UiRegion menu_region = CreateRegion(main, V2(1 - EMU_MENU_PERCENT, 1)); 
	menu_region.position.X += list_region.size.Width;
	RenderElement(RomMenu, UI_Roms, menu_region);

	UiRegion filter_region = CreateRegion(menu_region, V2(1, 0.05F));
	RenderElement(FilterBar, UI_Search, filter_region);

	RenderElement(InfoPane, UI_Info, main);

	DisplayInputHelp(pRender, main);

	if (pState->current_modal >= 0)
	{
		RenderModalWindow(pRender, pState->modals[pState->current_modal]);
	}

	RenderTextBox(pRender, &mtc);
}

static void UpdateUI(YaffeState* pState, float pDeltaTime)
{
	if (pState->overlay.running_rom.dwProcessId != 0) return;
	DisplayApplicationErrors(pState);
	DisplayQuitMessage(pState);

	if (pState->current_modal >= 0)
	{
		ModalWindow* modal = pState->modals[pState->current_modal];
		MODAL_RESULTS result = modal->content->Update(pDeltaTime);
		if (result != MODAL_RESULT_None)
		{
			if(modal->on_close) modal->on_close(result, modal->content);
			InterlockedDecrement(&pState->current_modal);
			delete modal->content;
			delete modal;
		}
		return;
	}

	for (u32 i = 0; i < UI_COUNT; i++)
	{
		g_ui.elements[i]->Update(pDeltaTime);
	}
}

static void InitializeUI(YaffeState* pState)
{
	g_ui.elements[UI_Emulators] = new EmulatorList(pState);
	g_ui.elements[UI_Roms] = new RomMenu(pState);
	g_ui.elements[UI_Info] = new InfoPane();
	g_ui.elements[UI_Search] = new FilterBar();

	FocusElement(UI_Emulators);

	pState->current_modal = -1;
	mtc = CreateTextBox(V2(100), 200, FONT_Normal);
}
