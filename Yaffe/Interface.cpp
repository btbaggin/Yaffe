inline static v2 CenterText(FONTS pFont, const char* pText, v2 pBounds)
{
	v2 size = MeasureString(pFont, pText);
	return (pBounds - size) / 2.0F;
}

inline static v4 GetFontColor(bool pFocused)
{
	return pFocused ? TEXT_FOCUSED : TEXT_UNFOCUSED;
}

inline static void FocusElement(UI_NAMES pElement)
{
	g_ui.focus[g_ui.focus_index++] = pElement;
	g_ui.elements[pElement]->OnFocusGained();
}

inline static void RevertFocus()
{
	if (g_ui.focus_index > 1) --g_ui.focus_index;
}

inline static UI_NAMES GetFocusedElement()
{
	return g_ui.focus[g_ui.focus_index - 1];
}

static void DisplayErrorMessage(const char* pError, ERROR_TYPE pType)
{
	if (g_state.error_count < MAX_ERROR_COUNT)
	{
		g_state.errors[g_state.error_count++] = pError;
		if (pType == ERROR_TYPE_Error) g_state.error_is_critical = true;
	}
}

static void RenderModalWindow(RenderState* pState, ModalWindow* pModal, PlatformWindow* pWindow)
{
	const float TITLEBAR_SIZE = 32.0F;
	const float ICON_SIZE = 32.0F;
	const float ICON_SIZE_WITH_MARGIN = ICON_SIZE + UI_MARGIN * 2;
	const float BUTTON_SIZE = 24.0F;

	v2 size = V2(UI_MARGIN * 4, UI_MARGIN * 2 + TITLEBAR_SIZE) + pModal->content->size;
	if (pModal->icon != BITMAP_None)
	{
		size.Height = max(ICON_SIZE_WITH_MARGIN, size.Height);
		size.Width += ICON_SIZE_WITH_MARGIN;
	}

	if (pModal->button) size.Height += BUTTON_SIZE;

	v2 window_position = V2((pWindow->width - size.Width) / 2, (pWindow->height - size.Height) / 2);
	v2 icon_position = window_position + V2(UI_MARGIN * 2, UI_MARGIN + TITLEBAR_SIZE); //Window + margin for window + margin for icon
	
	PushQuad(pState, V2(0.0F), V2(pWindow->width, pWindow->height), MODAL_OVERLAY_COLOR);

	//Window
	PushSizedQuad(pState, window_position, size, MODAL_BACKGROUND);

	//Tilebar
	PushSizedQuad(pState, window_position, V2(size.Width, TITLEBAR_SIZE), MODAL_TITLE);
	PushText(pState, FONT_Subtext, pModal->title, window_position + V2(UI_MARGIN, 0), TEXT_FOCUSED);

	//Sidebar highlight
	PushSizedQuad(pState, window_position + V2(0, TITLEBAR_SIZE), V2(UI_MARGIN, size.Height - TITLEBAR_SIZE), ACCENT_COLOR);

	if (pModal->icon != BITMAP_None)
	{
		Bitmap* image = GetBitmap(g_assets, pModal->icon);
		PushSizedQuad(pState, icon_position, V2(ICON_SIZE), image);
		icon_position.X += ICON_SIZE;
	}
	pModal->content->Render(pState, icon_position);

	v2 right = window_position + size - V2(BUTTON_SIZE);
	if (pModal->button)
	{
		PushRightAlignedTextWithIcon(pState, &right, BITMAP_ButtonA, BUTTON_SIZE, FONT_Subtext, pModal->button, TEXT_FOCUSED);
	}
}

static bool DisplayModalWindow(YaffeState* pState, const char* pTitle, ModalContent* pContent, BITMAPS pImage, modal_window_close* pClose, const char* pButtons)
{
	_WriteBarrier();
	if (pState->current_modal < MAX_MODAL_COUNT)
	{
		s32 modal_index = InterlockedIncrement(&pState->current_modal);
		ModalWindow* modal = new ModalWindow();
		modal->title = pTitle;
		modal->icon = pImage;
		modal->content = pContent;
		modal->button = pButtons;
		modal->on_close = pClose;
		pState->modals[modal_index] = modal;
		return true;
	}
	return false;
}
static bool DisplayModalWindow(YaffeState* pState, const char* pTitle, std::string pMessage, BITMAPS pImage, modal_window_close* pClose)
{
	return DisplayModalWindow(pState, pTitle, new StringModal(pMessage), pImage, pClose, "Ok");
}

static MODAL_CLOSE(OnAddApplicationModalClose);
static MODAL_CLOSE(ErrorModalClose)
{ 
	if (pState->error_is_critical) pState->is_running = false; 
}
static MODAL_CLOSE(ExitModalClose)
{ 
	if (pResult == MODAL_RESULT_Ok)
	{
		ListModal<std::string>* content = (ListModal<std::string>*)pContent;
		
		if (content->GetSelected() == "Add Emulator")
		{
			DisplayModalWindow(pState, "Add Platform", new PlatformDetailModal(nullptr, false), BITMAP_None, OnAddApplicationModalClose, "Save");
		}
		else if (content->GetSelected() == "Add Application")
		{
			DisplayModalWindow(pState, "Add Platform", new PlatformDetailModal(nullptr, true), BITMAP_None, OnAddApplicationModalClose, "Save");
		}
		else if (content->GetSelected() == "Settings")
		{
			DisplayModalWindow(&g_state, "Settings", new YaffeSettingsModal(), BITMAP_None, YaffeSettingsModalClose, "Save");
		}
		else if (content->GetSelected() == "Exit Yaffe")
		{
			pState->is_running = false;
		}
		else if (content->GetSelected() == "Shut Down")
		{
			Verify(Shutdown(), "Unable to initiate shutdown", ERROR_TYPE_Warning);
		}
		else assert(false);
	}
}

static void DisplayApplicationErrors(YaffeState* pState)
{
	//Check for and display any errors
	u32 errors = pState->error_count;
	if (errors > 0)
	{
		std::string message;
		for (u32 i = 0; i < errors; i++)
		{
			message += std::string(pState->errors[i]);
			if (i < errors - 1) message += '\n';
		}

		DisplayModalWindow(pState, "Error", message, BITMAP_Error, ErrorModalClose);
		pState->error_count = 0;
	}
}

static void DisplayToolbar(UiRegion pMain, RenderState* pRender)
{
	float title = GetFontSize(FONT_Title);
	v2 menu_position = pMain.size - V2(UI_MARGIN, UI_MARGIN + title);

	//Time
	char buffer[20];
	GetTime(buffer, 20);
	PushRightAlignedTextWithIcon(pRender, &menu_position, BITMAP_None, 0, FONT_Title, buffer, TEXT_FOCUSED); menu_position.X -= UI_MARGIN;

	//Action buttons
	menu_position.Y += (title - GetFontSize(FONT_Normal));
	switch (GetFocusedElement())
	{
		case UI_Roms:
		PushRightAlignedTextWithIcon(pRender, &menu_position, BITMAP_ButtonY, 24, FONT_Normal, "Filter"); menu_position.X -= UI_MARGIN;
		PushRightAlignedTextWithIcon(pRender, &menu_position, BITMAP_ButtonB, 24, FONT_Normal, "Back"); menu_position.X -= UI_MARGIN;
		break;

		case UI_Emulators:
		Platform* plat = GetSelectedPlatform();
		if (plat && plat->type != PLATFORM_Recents)
		{
			PushRightAlignedTextWithIcon(pRender, &menu_position, BITMAP_ButtonX, 24, FONT_Normal, "Info"); menu_position.X -= UI_MARGIN;
		}
		PushRightAlignedTextWithIcon(pRender, &menu_position, BITMAP_ButtonA, 24, FONT_Normal, "Select"); menu_position.X -= UI_MARGIN;
		break;
	}
}

static void DisplayQuitMessage(YaffeState* pState)
{
	if (GetForegroundWindow() == g_state.form->handle &&
		g_state.current_modal < 0 &&
		(IsKeyPressed(KEY_Q) || IsControllerPressed(CONTROLLER_START)))
	{
		//Only allow quitting when current window is focused
		std::vector<std::string> options;
		options.push_back("Exit Yaffe");
		options.push_back("Shut Down");
		options.push_back("Settings");
		options.push_back("Add Emulator");
		options.push_back("Add Application");
		DisplayModalWindow(pState, "Menu", new ListModal<std::string>(options, "", nullptr, MODAL_SIZE_Third), BITMAP_None, ExitModalClose);
	}
}

static UiRegion CreateRegion()
{
	UiRegion region;
	region.position = V2(0);
	region.size = V2(g_state.form->width, g_state.form->height);

	return region;
}

static UiRegion CreateRegion(UiRegion pRegion, v2 pSize)
{
	UiRegion region;
	region.position = pRegion.position;
	region.size = pRegion.size * pSize;

	return region;
}

#include "PlatformList.cpp"
#include "RomMenu.cpp"
#include "FilterBar.cpp"
#include "InfoPane.cpp"
static void RenderUI(YaffeState* pState, RenderState* pRender, Assets* pAssets)
{
	//Render background
	UiRegion main = CreateRegion();
	Bitmap* b = GetBitmap(g_assets, BITMAP_Background);
	PushQuad(pRender, main.position, main.size, b);

	UiRegion list_region = CreateRegion(main, V2(EMU_MENU_PERCENT, 1));
	RenderElement(UI_Emulators, list_region);

	UiRegion menu_region = CreateRegion(main, V2(1 - EMU_MENU_PERCENT, 1)); 
	menu_region.position.X += list_region.size.Width;
	RenderElement(UI_Roms, menu_region);

	UiRegion filter_region = CreateRegion(menu_region, V2(1, 0.05F));
	RenderElement(UI_Search, filter_region);

	RenderElement(UI_Info, main);

	DisplayToolbar(main, pRender);
	if (pState->current_modal >= 0)
	{
		RenderModalWindow(pRender, pState->modals[pState->current_modal], pState->form);
	}
}

static void UpdateUI(YaffeState* pState, float pDeltaTime)
{
	if (pState->overlay.process) return;
	DisplayApplicationErrors(pState);
	DisplayQuitMessage(pState);

	if (pState->current_modal >= 0)
	{
		ModalWindow* modal = pState->modals[pState->current_modal];
		MODAL_RESULTS result = modal->content->Update(pDeltaTime);
		if (result != MODAL_RESULT_None)
		{
			InterlockedDecrement(&pState->current_modal);
			if(modal->on_close) modal->on_close(pState, result, modal->content);
			delete modal->content;
			delete modal;
		}
		return;
	}

	UI_NAMES focused = GetFocusedElement();
	for (u32 i = 0; i < (u32)focused; i++)
	{
		g_ui.elements[i]->UnfocusedUpdate(pState, pDeltaTime);
	}
	g_ui.elements[focused]->Update(pState, pDeltaTime);
	for (u32 i = focused + 1; i < UI_COUNT; i++)
	{
		g_ui.elements[i]->UnfocusedUpdate(pState, pDeltaTime);
	}
}

static void InitializeUI(YaffeState* pState)
{
	g_ui.elements[UI_Emulators] = new PlatformList(pState);
	g_ui.elements[UI_Roms] = new RomMenu();
	g_ui.elements[UI_Info] = new InfoPane();
	g_ui.elements[UI_Search] = new FilterBar();

	FocusElement(UI_Emulators);

	pState->current_modal = -1;
}
