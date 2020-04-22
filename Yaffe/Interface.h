#pragma once

const float EMU_MENU_PERCENT = 0.2F;
const float ROM_PAGE_PADDING = 0.05F;
const float UI_MARGIN = 5.0F;

struct RenderState;
enum UI_ELEMENT_NAME
{
	UI_None,
	UI_Emulators,
	UI_Roms,
	UI_Info,
	UI_Search,
};

enum MODAL_RESULTS
{
	MODAL_RESULT_None,
	MODAL_RESULT_Ok,
	MODAL_RESULT_Cancel,
};

class ModalContent;
#define MODAL_CLOSE(name) void name(MODAL_RESULTS pResult, ModalContent* pContent)
typedef MODAL_CLOSE(modal_window_close);
struct ModalWindow
{
	BITMAPS icon;
	ModalContent* content;
	modal_window_close* on_close;
};

extern YaffeState g_state;
class UiElement
{
	bool focused;
	v2 size;

protected:
	v2 position;

	UiElement(v2 pSize, UI_ELEMENT_NAME pTag = UI_None)
	{
		size = pSize;
		tag = pTag;
		child_count = 0;
		focused = false;
		position = V2(0);
	}

	virtual void Render(RenderState* pState) = 0;
	virtual void Update(float pDeltaTime) = 0;
	virtual void OnFocusGained() {}
	virtual void OnFocusLost() {}

public:
	UI_ELEMENT_NAME tag;
	UiElement* children[4];
	u32 child_count;

	v2 GetAbsoluteSize()
	{
		return V2(this->size.Width * g_state.form.width, this->size.Height * g_state.form.height);
	}

	bool IsFocused() { return focused; }

	void SetFocus(bool pFocused)
	{
		if (focused != pFocused)
		{
			focused = pFocused;
			if (focused) OnFocusGained();
			else OnFocusLost();
		}
	}

	void AddChild(UiElement* pElement)
	{
		assert(child_count < 4);
		children[child_count++] = pElement;
	}

	void RenderElement(RenderState* pState)
	{
		Render(pState);
		for (u32 i = 0; i < child_count; i++)
		{
			children[i]->RenderElement(pState);
		}
	}

	void UpdateElement(float pDeltaTime)
	{
		for (u32 i = 0; i < child_count; i++)
		{
			children[i]->UpdateElement(pDeltaTime);
		}

		if(focused) Update(pDeltaTime);
	}

	virtual void OnEmulatorChanged(Emulator* pEmulator)
	{
		for (u32 i = 0; i < child_count; i++)
		{
			children[i]->OnEmulatorChanged(pEmulator);
		}
	}
};

struct Interface
{
	UiElement* root;

	UI_ELEMENT_NAME focus[8];
	u32 focus_index;
};

const v4 MENU_BACKGROUND = { 0.25F, 0.25F, 0.25F, 0.5F };
const v4 TEXT_FOCUSED = { 1, 1, 1, 1 };
const v4 TEXT_UNFOCUSED = { 0.6F, 0.6F, 0.6F, 1 };

const v4 MODAL_BACKGROUND = { 0.1F, 0.1F, 0.1F, 1 };

const v4 ACCENT_COLOR = { 0.25, 0.75, 1, 1 };
const v4 ACCENT_UNFOCUSED = { 0.4F, 0.4F, 0.4F, 1 };

static bool DisplayModalWindow(YaffeState* pState, ModalContent* pContent, BITMAPS pImage, modal_window_close* pClose);
