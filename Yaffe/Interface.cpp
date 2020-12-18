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

static bool IsWidgetVisible(Widget* pWidget)
{
	v2 min = pWidget->GetPosition();
	v2 max = min + pWidget->size;

	return min.X < g_state.form->width - 1 && min.Y < g_state.form->height - 1 && max.X > 1 && max.Y > 1;
}

template<typename T>
inline T GetWidget(UI_NAMES pName)
{
	return (T)g_ui.elements[pName];
}

static void DisplayErrorMessage(const char* pError, ERROR_TYPE pType, ...)
{
	if (g_state.errors.CanAdd())
	{
		va_list args;
		va_start(args, pType);

		char* buffer = new char[1000];
		vsprintf(buffer, pError, args);

		g_state.errors.AddItem(buffer);
		if (pType == ERROR_TYPE_Error) g_state.error_is_critical = true;

		va_end(args);

	}
}
static void CloseModalWindow(ModalWindow* pModal, MODAL_RESULTS pResult, YaffeState* pState)
{
	if (pModal->on_close) pModal->on_close(pState, pResult, pModal->content);
	delete pModal->content;
	delete pModal;
	pModal = nullptr;
}

static void RenderModalWindow(RenderState* pState, ModalWindow* pModal, Form* pWindow)
{
	const float TITLEBAR_SIZE = 32.0F;
	const float ICON_SIZE = 32.0F;
	const float ICON_SIZE_WITH_MARGIN = ICON_SIZE + UI_MARGIN * 2;
	const float BUTTON_SIZE = 24.0F;

	v2 size = V2(UI_MARGIN * 4, UI_MARGIN * 2 + TITLEBAR_SIZE) + pModal->content->size;
	if (pModal->icon != BITMAP_None)
	{
		size.Height = std::max(ICON_SIZE_WITH_MARGIN, size.Height);
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
	WriteBarrier();
	if (pState->modals.CanAdd())
	{
		volatile long count = pState->modals.count;
		volatile long new_count = count + 1;
		volatile long modal_index = AtomicCompareExchange(&pState->modals.count, new_count, count);
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
		else if (content->GetSelected() == "Enable Restricted Mode")
		{
			EnableRestrictedMode(pState, pState->restrictions);
		}
		else if (content->GetSelected() == "Disable Restricted Mode")
		{
			DisableRestrictedMode(pState, pState->restrictions);
		}
		else assert(false);
	}
}

static void DisplayApplicationErrors(YaffeState* pState)
{
	//Check for and display any errors
	u32 errors = pState->errors.count;
	if (errors > 0)
	{
		std::string message;
		for (u32 i = 0; i < errors; i++)
		{
			message += std::string(pState->errors[i]);
			if (i < errors - 1) message += '\n';
			delete pState->errors[i];
		}

		DisplayModalWindow(pState, "Error", message, BITMAP_Error, ErrorModalClose);
		pState->errors.Clear();
	}
}

static void DisplayQuitMessage(YaffeState* pState)
{
	if (WindowIsForeground(pState) &&
		pState->modals.count == 0 &&
		(IsKeyPressed(KEY_Q) || IsControllerPressed(CONTROLLER_START)))
	{
		//Only allow quitting when current window is focused
		std::vector<std::string> options;
		options.push_back("Exit Yaffe");
		options.push_back("Shut Down");
		if (pState->restrictions->state == RESTRICTED_MODE_Off) options.push_back("Enable Restricted Mode");
		else if (pState->restrictions->state == RESTRICTED_MODE_On) options.push_back("Disable Restricted Mode");

		options.push_back("Settings");
		options.push_back("Add Emulator");
		options.push_back("Add Application");

		DisplayModalWindow(pState, "Menu", new ListModal<std::string>(options, "", nullptr, MODAL_SIZE_Third), BITMAP_None, ExitModalClose);
	}
}

#include "ExecutableTile.cpp"
#include "TileFlexBox.cpp"
#include "PlatformList.cpp"
#include "FilterBar.cpp"
#include "InfoPane.cpp"
#include "Background.cpp"
#include "Toolbar.cpp"
#include "Div.cpp"
static void RenderUI(YaffeState* pState, RenderState* pRender, Assets* pAssets)
{
	g_ui.root->DoRender(pRender);
	if (pState->modals.count > 0)
	{
		RenderModalWindow(pRender, pState->modals.CurrentItem(), pState->form);
	}
}

static void UpdateUI(YaffeState* pState, float pDeltaTime)
{
	if (pState->restrictions->modal)
	{
		MODAL_RESULTS result = pState->restrictions->modal->content->Update(pDeltaTime);
		if(result != MODAL_RESULT_None) CloseModalWindow(pState->restrictions->modal, result, pState);
		return;
	}

	if (pState->overlay.process || pState->restrictions->modal) return;
	DisplayApplicationErrors(pState);
	DisplayQuitMessage(pState);

	if (pState->modals.count > 0)
	{
		ModalWindow* modal = pState->modals.CurrentItem();
		MODAL_RESULTS result = modal->content->Update(pDeltaTime);
		if (result != MODAL_RESULT_None)
		{
			AtomicAdd(&pState->modals.count, -1);
			CloseModalWindow(modal, result, pState);
		} 
		return;
	}

	g_ui.root->DoUpdate(pState, pDeltaTime);
}

static void InitializeUI(YaffeState* pState, Interface* pInterface)
{
	const float EMU_MENU_PERCENT = 0.2F;
	Div* div = new Div(RelativeToAbsolute(EMU_MENU_PERCENT, 0), 1 - EMU_MENU_PERCENT, 1, pInterface);
	pInterface->root = new Background(pInterface);
	pInterface->root->AddChild(new PlatformList(pInterface, EMU_MENU_PERCENT));
	pInterface->root->AddChild(div);

	div->AddChild(new TileFlexBox(pInterface));
	div->AddChild(new FilterBar(pInterface));
	div->AddChild(new InfoPane(pInterface));
	div->AddChild(new Toolbar(pInterface));

	FocusElement(UI_Emulators);
}
