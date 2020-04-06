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

static UiElement* __FindElement(UI_ELEMENT_NAME pTag, UiElement* pElement)
{
	if (pElement->tag == pTag)
	{
		return pElement;
	}

	for (u32 i = 0; i < pElement->child_count; i++)
	{
		UiElement* node = __FindElement(pTag, pElement->children[i]);
		if (node) return node;
	}

	return nullptr;
}
static UiElement* FindElement(UI_ELEMENT_NAME pTag)
{
	return __FindElement(pTag, g_ui.root);
}


static void __ResetFocus(UiElement* pElement)
{
	pElement->SetFocus(false);
	for (u32 i = 0; i < pElement->child_count; i++)
	{
		__ResetFocus(pElement->children[i]);
	}
}
static void FocusElement(UI_ELEMENT_NAME pElement)
{
	UiElement* element = FindElement(pElement);
	if (!pElement) return;

	__ResetFocus(g_ui.root);
	element->SetFocus(true);
	g_ui.focus[g_ui.focus_index++] = pElement;
}

static void RevertFocus()
{
	UI_ELEMENT_NAME focus = g_ui.focus[--g_ui.focus_index - 1];
	UiElement* element = FindElement(focus);
	if (!element) return;

	__ResetFocus(g_ui.root);
	element->SetFocus(true);
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
static void RenderUI(YaffeState* pState, RenderState* pRender, Assets* pAssets)
{
	g_ui.root->RenderElement(pRender);

	if (pState->current_modal >= 0)
	{
		RenderModalWindow(pRender, pState->modals[pState->current_modal]);
	}

	//Errors
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
	else if (IsApplicationFocused() && (IsKeyPressed(KEY_Q) || IsControllerPressed(CONTROLLER_START)))
	{
		//Only allow quitting when current window is focused
		std::vector<std::string> options;
		options.push_back("Quit");
		options.push_back("Shut Down");
		DisplayModalWindow(pState, new SelectorModal(options, (char*)""), BITMAP_None, ExitModalClose);
	}
}

static void UpdateUI(YaffeState* pState, float pDeltaTime)
{
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

	g_ui.root->UpdateElement(pDeltaTime);
}

#include "Root.cpp"
#include "EmulatorList.cpp"
#include "RomMenu.cpp"
#include "FilterBar.cpp"
#include "InfoPane.cpp"
#include "YaffeOverlay.cpp"
static void InitializeUI(YaffeState* pState)
{
	UiRoot* background = new UiRoot();
	EmulatorList* emu = new EmulatorList(pState);
	RomMenu* rom = new RomMenu(pState, emu);
	FilterBar* toast = new FilterBar();
	InfoPane* pane = new InfoPane(emu, rom);
	YaffeOverlay* overlay = new YaffeOverlay();

	background->AddChild(emu);
	background->AddChild(rom);
	background->AddChild(overlay);

	rom->AddChild(toast);
	rom->AddChild(pane);

	g_ui.root = background;
	FocusElement(UI_Emulators);

	pState->current_modal = -1;
}
