#pragma once

#define STB_TEXTEDIT_CHARTYPE   char
#define STB_TEXTEDIT_STRING     Textbox

#include "stb/stb_textedit.h"
#include "Colors.h"

const float UI_MARGIN = 5.0F;
const float LABEL_WIDTH = 220;
const float ANIMATION_SPEED = 5;

enum UI_NAMES
{
	UI_None,
	UI_Emulators,
	UI_Roms,
	UI_Info,
	UI_Search,

	UI_COUNT,
};

enum MODAL_RESULTS : u8
{
	MODAL_RESULT_None,
	MODAL_RESULT_Ok,
	MODAL_RESULT_Cancel,
};

class ModalContent;
#define MODAL_CLOSE(name) void name(YaffeState* pState, MODAL_RESULTS pResult, ModalContent* pContent)
typedef MODAL_CLOSE(modal_window_close);
struct ModalWindow
{
	BITMAPS icon;
	const char* title;
	ModalContent* content;
	const char* button;
	modal_window_close* on_close;
};

extern YaffeState g_state;
class Widget;
struct Interface
{
	Widget* elements[UI_COUNT];
	Widget* root;

	UI_NAMES focus[8];
	u32 focus_index;
};

#define RelativeToAbsolute(x, y) V2(x, y) * V2(g_state.form->width, g_state.form->height)
inline static UI_NAMES GetFocusedElement();
static bool IsWidgetVisible(Widget* pWidget);
class Widget
{
	UI_NAMES tag;
	v2 ratio_size;
	v2 position;

protected:
	Widget* parent;
	Widget* children[48];
	u32 child_count;

	Widget(UI_NAMES pTag, Interface* pInterface)
	{
		tag = pTag;
		child_count = 0;
		position = {};
		size = {};
		if (pTag != UI_None)
		{
			pInterface->elements[pTag] = this;
		}
	}

public:
	v2 size;
	virtual void Render(RenderState* pState) = 0;
	virtual void Update(YaffeState* pState, float pDeltaTime) = 0;
	virtual void OnFocusGained() { }
	virtual void OnAdded() { }

	void AddChild(Widget* pWidget)
	{
		pWidget->parent = this;
		children[child_count++] = pWidget;
		pWidget->OnAdded();
	}

	void ClearChildren()
	{
		for (u32 i = 0; i < child_count; i++)
		{
			delete children[i];
		}
		child_count = 0;
	}

	void SetSize(float pWidth, float pHeight)
	{
		ratio_size = V2(pWidth, pHeight);
		OnParentSizeChanged(parent ? parent->size : V2(g_state.form->width, g_state.form->height));
	}

	void OnParentSizeChanged(v2 pNewSize)
	{
		size = V2(pNewSize.Width * ratio_size.Width, pNewSize.Height * ratio_size.Height);
		for (u32 i = 0; i < child_count; i++)
		{
			children[i]->OnParentSizeChanged(size);
		}
	}

	void SetPosition(v2 pPosition)
	{
		position = pPosition;
		//We just don't care about children right now since nothing moves
	}

	v2 GetPosition()
	{
		if (parent) return parent->GetPosition() + position;
		return position;
	}

	v2 GetRelativePosition() { return position; }

	virtual void DoRender(RenderState* pState)
	{
		if(IsWidgetVisible(this)) Render(pState);
		for (u32 i = 0; i < child_count; i++)
		{
			children[i]->DoRender(pState);
		}
	}

	void DoUpdate(YaffeState* pState, float pDeltaTime)
	{
		for (u32 i = 0; i < child_count; i++)
		{
			children[i]->DoUpdate(pState, pDeltaTime);
		}
		Update(pState, pDeltaTime);
	}

	bool IsFocused()
	{
		return GetFocusedElement() == tag;
	}
};

#define RenderElement(name, region) g_ui.elements[name]->Render(pRender, region);

struct Textbox
{
	char *string;
	int stringlen;
	FONTS font;
	bool focused;
	bool enabled;
	float width;
	float font_x;
	STB_TexteditState state;
};

struct Checkbox 
{
	bool checked;
	bool enabled;
};

struct FilePathBox
{
	char string[MAX_PATH];
	float width;
	FONTS font;
	bool enabled;
	bool files;
};

static bool DisplayModalWindow(YaffeState* pState, const char* pTitle, ModalContent* pContent, BITMAPS pImage, modal_window_close* pClose, const char* pButton = nullptr);
