#pragma once

const float EMU_MENU_PERCENT = 0.2F;
const float ROM_PAGE_PADDING = 0.075F;
const float UI_MARGIN = 5.0F;
const float SELECTED_ROM_SIZE = 0.2F;
const float INFO_PANE_WIDTH = 0.5F;

struct RenderState;
enum UI_ELEMENT_NAME
{
	UI_Emulators,
	UI_Roms,
	UI_Info,
	UI_Search,

	UI_COUNT,
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
class UiElement;
struct Interface
{
	UiElement* elements[UI_COUNT];

	UI_ELEMENT_NAME focus[8];
	u32 focus_index;
};

extern Interface g_ui;
class UiElement
{
public:
	UI_ELEMENT_NAME tag;

	UiElement(UI_ELEMENT_NAME pTag)
	{
		tag = pTag;
	}

	virtual void Update(float pDeltaTime) = 0;
	virtual void OnFocusGained() {}

	bool IsFocused()
	{
		return g_ui.focus[g_ui.focus_index - 1] == tag;
	}
};

#define RenderElement(type, name, region) type::Render(pRender, region, (type*)g_ui.elements[name]);


struct UiRegion
{
	v2 position;
	v2 size;
};


const v4 MENU_BACKGROUND = { 0.25F, 0.25F, 0.25F, 0.5F };
const v4 TEXT_FOCUSED = { 1, 1, 1, 1 };
const v4 TEXT_UNFOCUSED = { 0.6F, 0.6F, 0.6F, 1 };

const v4 MODAL_BACKGROUND = { 0.1F, 0.1F, 0.1F, 1 };

const v4 ACCENT_COLOR = { 0.25, 0.75, 1, 1 };
const v4 ACCENT_UNFOCUSED = { 0.4F, 0.4F, 0.4F, 1 };

static bool DisplayModalWindow(YaffeState* pState, ModalContent* pContent, BITMAPS pImage, modal_window_close* pClose);
