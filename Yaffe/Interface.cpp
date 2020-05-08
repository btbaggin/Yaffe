inline static v2 CenterText(FONTS pFont, const char* pText, v2 pBounds)
{
	v2 size = MeasureString(pFont, pText);

	return V2((pBounds.Width - size.Width) / 2.0F, (pBounds.Height - size.Height - (size.Height / 2.0F)) / 2.0F);
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

inline static UiControl* GetControl(UI_NAMES pName)
{
	return g_ui.elements[pName];
}

static void DisplayErrorMessage(const char* pError, ERROR_TYPE pType)
{
	assert(g_state.error_count < MAX_ERROR_COUNT);

	g_state.errors[g_state.error_count++] = pError;
	if (pType == ERROR_TYPE_Error) g_state.error_is_critical = true;
}

static void RenderModalWindow(RenderState* pState, ModalWindow* pWindow)
{
	const float TITLEBAR_SIZE = 32.0F;
	const float ICON_SIZE = 32.0F;
	const float ICON_SIZE_WITH_MARGIN = ICON_SIZE + UI_MARGIN * 2;
	v2 size = V2(UI_MARGIN * 4, UI_MARGIN * 2 + TITLEBAR_SIZE) + pWindow->content->size;
	if (pWindow->icon != BITMAP_None)
	{
		size.Height = max(ICON_SIZE_WITH_MARGIN, size.Height);
		size.Width += ICON_SIZE_WITH_MARGIN;
	}

	v2 window_position = V2((g_state.form->width - size.Width) / 2, (g_state.form->height - size.Height) / 2);
	v2 icon_position = window_position + V2(UI_MARGIN * 2, UI_MARGIN + TITLEBAR_SIZE); //Window + margin for window + margin for icon
	
	PushQuad(pState, V2(0.0F), V2((float)g_state.form->width, (float)g_state.form->height), V4(0.0F, 0.0F, 0.0F, 0.4F));

	//Window
	PushSizedQuad(pState, window_position, size, MODAL_BACKGROUND);
	//Tilebar
	PushSizedQuad(pState, window_position, V2(size.Width, TITLEBAR_SIZE), MODAL_TITLE);
	PushText(pState, FONT_Subtext, pWindow->title, window_position + V2(UI_MARGIN, 0), TEXT_FOCUSED);

	//Sidebar highlight
	PushSizedQuad(pState, window_position + V2(0, TITLEBAR_SIZE), V2(UI_MARGIN, size.Height - TITLEBAR_SIZE), ACCENT_COLOR);

	if (pWindow->icon != BITMAP_None)
	{
		Bitmap* image = GetBitmap(g_assets, pWindow->icon);
		PushSizedQuad(pState, icon_position, V2(ICON_SIZE), image);
		icon_position.X += ICON_SIZE;
	}
	pWindow->content->Render(pState, icon_position);
}

static bool DisplayModalWindow(YaffeState* pState, const char* pTitle, ModalContent* pContent, BITMAPS pImage, modal_window_close* pClose)
{
	_WriteBarrier();
	if (pState->current_modal < MAX_MODAL_COUNT)
	{
		s32 modal_index = InterlockedIncrement(&pState->current_modal);
		ModalWindow* modal = new ModalWindow();
		modal->title = pTitle;
		modal->icon = pImage;
		modal->content = pContent;
		modal->on_close = pClose;
		pState->modals[modal_index] = modal;
		return true;
	}
	return false;
}
static bool DisplayModalWindow(YaffeState* pState, const char* pTitle, std::string pMessage, BITMAPS pImage, modal_window_close* pClose)
{
	return DisplayModalWindow(pState, pTitle, new StringModal(pMessage), pImage, pClose);
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
			Verify(Shutdown(), "Unable to initiate shutdown", ERROR_TYPE_Warning);
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

		DisplayModalWindow(pState, "Error", message, BITMAP_Error, ErrorModalClose);
		pState->error_count = 0;
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
		options.push_back("Quit");
		options.push_back("Shut Down");
		DisplayModalWindow(pState, "Exit", new SelectorModal(options, nullptr), BITMAP_None, ExitModalClose);
	}
}

static UiRegion CreateRegion()
{
	UiRegion region;
	region.position = V2(0);
	region.size = V2((float)g_state.form->width, (float)g_state.form->height);

	return region;
}

static UiRegion CreateRegion(UiRegion pRegion, v2 pSize)
{
	UiRegion region;
	region.position = pRegion.position;
	region.size = pRegion.size * pSize;

	return region;
}

#include "EmulatorList.cpp"
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
	RenderElement(EmulatorList, UI_Emulators, list_region);

	UiRegion menu_region = CreateRegion(main, V2(1 - EMU_MENU_PERCENT, 1)); 
	menu_region.position.X += list_region.size.Width;
	RenderElement(RomMenu, UI_Roms, menu_region);

	UiRegion filter_region = CreateRegion(menu_region, V2(1, 0.05F));
	RenderElement(FilterBar, UI_Search, filter_region);

	RenderElement(InfoPane, UI_Info, main);

	v2 menu_position = main.size - V2(UI_MARGIN);
	switch (GetFocusedElement())
	{
		case UI_Roms:
		{
			menu_position = PushRightAlignedTextWithIcon(pRender, menu_position, BITMAP_ButtonY, 24, FONT_Normal, "Filter");
			menu_position = PushRightAlignedTextWithIcon(pRender, menu_position, BITMAP_ButtonB, 24, FONT_Normal, "Back");
		}
		break;

		case UI_Emulators:
		{
			menu_position = PushRightAlignedTextWithIcon(pRender, menu_position, BITMAP_ButtonX, 24, FONT_Normal, "Add");
			menu_position = PushRightAlignedTextWithIcon(pRender, menu_position, BITMAP_ButtonA, 24, FONT_Normal, "Select");
		}
		break;
	}
	if (pState->current_modal >= 0)
	{
		RenderModalWindow(pRender, pState->modals[pState->current_modal]);
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
}
