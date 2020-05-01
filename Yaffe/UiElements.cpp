//
// TEXTBOX
//
#include <stdlib.h>
#include <string.h> // memmove
#include <ctype.h>  // isspace
#include <WinUser.h>

// define the functions we need
void layout_func(StbTexteditRow *row, STB_TEXTEDIT_STRING *str, int start_i)
{
	int remaining_chars = str->stringlen - start_i;
	row->num_chars = remaining_chars; // should do real word wrap here
	row->x0 = 0;
	row->x1 = 0;
	for (int i = 0; i < str->stringlen; i++)
	{
		float char_width = CharWidth(str->font, str->string[i]);
		if (i < start_i) row->x0 += char_width;
		row->x1 += char_width;
	}
	row->baseline_y_delta = 1.25;
	row->ymin = -1;
	row->ymax = 0;
}

int delete_chars(STB_TEXTEDIT_STRING *str, int pos, int num)
{
	memmove(&str->string[pos], &str->string[pos + num], str->stringlen - (pos + num));
	str->stringlen -= num;
	str->string[str->stringlen] = '\0';
	return 1; // always succeeds
}

int insert_chars(STB_TEXTEDIT_STRING *str, int pos, STB_TEXTEDIT_CHARTYPE *newtext, int num)
{
	str->string = (char*)realloc(str->string, str->stringlen + num);
	memmove(&str->string[pos + num], &str->string[pos], str->stringlen - pos);
	memcpy(&str->string[pos], newtext, num);
	str->stringlen += num;
	str->string[str->stringlen] = '\0';
	return 1; // always succeeds
}

static int MapKey(int key)
{
	if (IsKeyUp(KEY_Control))
	{
		UINT result = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
		WCHAR buffer[2];
		int chars = ToUnicodeEx(key, result, (const BYTE*)g_input.current_keyboard_state, buffer, 2, 0, g_input.layout);
		if (chars == 1) return buffer[0];
	}
	return -1;
}
// define all the #defines needed 

#define KEYDOWN_BIT                    0x80000000

#define STB_TEXTEDIT_STRINGLEN(tc)     ((tc)->stringlen)
#define STB_TEXTEDIT_LAYOUTROW         layout_func
#define STB_TEXTEDIT_GETWIDTH(tc,n,i)  CharWidth(tc->font, tc->string[i])
#define STB_TEXTEDIT_KEYTOTEXT(key)    MapKey(key)
#define STB_TEXTEDIT_GETCHAR(tc,i)     ((tc)->string[i])
#define STB_TEXTEDIT_NEWLINE           '\n'
#define STB_TEXTEDIT_IS_SPACE(ch)      isspace(ch)
#define STB_TEXTEDIT_DELETECHARS       delete_chars
#define STB_TEXTEDIT_INSERTCHARS       insert_chars

#define STB_TEXTEDIT_K_SHIFT           0x40000000
#define STB_TEXTEDIT_K_CONTROL         0x20000000
#define STB_TEXTEDIT_K_LEFT            KEY_Left
#define STB_TEXTEDIT_K_RIGHT           KEY_Right
#define STB_TEXTEDIT_K_UP              KEY_Up
#define STB_TEXTEDIT_K_DOWN            KEY_Down
#define STB_TEXTEDIT_K_LINESTART       KEY_Home
#define STB_TEXTEDIT_K_LINEEND         KEY_End
#define STB_TEXTEDIT_K_TEXTSTART       (STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTEND         (STB_TEXTEDIT_K_LINEEND   | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_DELETE          KEY_Delete
#define STB_TEXTEDIT_K_BACKSPACE       KEY_Backspace
#define STB_TEXTEDIT_K_UNDO            (STB_TEXTEDIT_K_CONTROL | KEY_Z)
#define STB_TEXTEDIT_K_REDO            (STB_TEXTEDIT_K_CONTROL | KEY_Y)
#define STB_TEXTEDIT_K_INSERT          KEY_Insert
#define STB_TEXTEDIT_K_WORDLEFT        (STB_TEXTEDIT_K_LEFT  | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_WORDRIGHT       (STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_PGUP            KEY_PageUp
#define STB_TEXTEDIT_K_PGDOWN          KEY_PageDown

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb/stb_textedit.h"

static Textbox CreateTextbox(float pWidth, FONTS pFont)
{
	Textbox tc = {};
	tc.width = pWidth;
	tc.font = pFont;
	tc.focused = false;
	stb_textedit_initialize_state(&tc.state, true);
	return tc;
}

static char* GetClipboardText()
{
	if (!OpenClipboard(nullptr)) return nullptr;

	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData == nullptr) return nullptr;

	char * pszText = static_cast<char*>(GlobalLock(hData));
	if (pszText == nullptr) return nullptr;

	char* result = pszText;

	GlobalUnlock(hData);
	CloseClipboard();

	return result;
}

static void RenderTextbox(RenderState* pState, Textbox* tc, v2 pPosition)
{
	v2 pos = GetMousePosition() - pPosition;
	float font_size = GetFontSize(tc->font);
	//
	// HANDLE INPUT
	//
	if (IsButtonPressed(BUTTON_Left))
	{
		//Toggle focus
		tc->focused = (pos.X > 0 && pos.Y > 0 && pos < V2(tc->width, font_size));
		if (tc->focused)
		{
			//Cursor placement
			stb_textedit_click(tc, &tc->state, pos.X, pos.Y);
		}
	}
	else if (IsButtonDown(BUTTON_Left) && tc->focused)
	{
		//Text selection
		stb_textedit_drag(tc, &tc->state, pos.X, pos.Y);
	}

	//Handle keyboard input
	if (tc->focused)
	{
		bool shift = IsKeyDown(KEY_Shift);
		bool control = IsKeyDown(KEY_Control);

		if (control && IsKeyPressed(KEY_V, false))
		{
			char* text = GetClipboardText();
			if(text) stb_textedit_paste(tc, &tc->state, text, (int)strlen(text));
		}

		for (int i = KEY_Backspace; i < KEY_Quote; i++)
		{
			int key = i;
			if (IsKeyPressed((KEYS)key, false))
			{
				if (shift) key |= STB_TEXTEDIT_K_SHIFT;
				if (control) key |= STB_TEXTEDIT_K_CONTROL;
				stb_textedit_key(tc, &tc->state, key);
			}
		}
	}

	//
	// RENDER
	//	
	//Focus highlight
	if (tc->focused)
	{
		PushQuad(pState, pPosition - V2(2), pPosition + V2(tc->width, font_size) + V2(2), ACCENT_COLOR);
	}
	//Background
	PushQuad(pState, pPosition, pPosition + V2(tc->width, font_size), V4(0.5F));

	//Selection
	if (tc->focused)
	{
		int start_index = min(tc->state.select_start, tc->state.select_end);
		int end_index = max(tc->state.select_start, tc->state.select_end);
		if (end_index > 0)
		{
			StbFindState start = {};
			StbFindState end = {};
			stb_textedit_find_charpos(&start, tc, start_index, true);
			stb_textedit_find_charpos(&end, tc, end_index, true);
			PushQuad(pState, pPosition + V2(start.x, start.y), pPosition + V2(end.x, end.y + font_size), V4(0, 0.5F, 1, 1));
		}
	}

	//Text
	if (tc->string)
	{
		PushText(pState, tc->font, tc->string, pPosition);
	}

	//Cursor
	if (tc->focused)
	{
		StbFindState state = {};
		stb_textedit_find_charpos(&state, tc, tc->state.cursor, true);
		PushQuad(pState, pPosition + V2(state.x, state.y), pPosition + V2(state.x + 2, state.y + font_size), V4(1));
	}
}


//
// CHECKBOX
//
static Checkbox CreateCheckbox()
{
	Checkbox box;
	box.checked = false;
	return box;
}

static void RenderCheckbox(RenderState* pState, Checkbox* pCheck, v2 pPosition)
{
	v2 pos = GetMousePosition() - pPosition;
	if (pos > V2(0) && pos < V2(24))
	{
		if (IsButtonPressed(BUTTON_Left)) pCheck->checked = !pCheck->checked;
	}

	PushQuad(pState, pPosition, pPosition + V2(24), V4(0.5F));
	if (pCheck->checked)
	{
		PushQuad(pState, pPosition + V2(3), pPosition + V2(21), ACCENT_COLOR);
	}
}


//
// LABELEDELEMENT
//
#define RenderElementWithLabel(state, element_type, element, position, label) \
PushText(state, FONT_Normal, label, position, V4(1)); \
Render##element_type##(pState, &element, position + V2(LABEL_WIDTH, 0));
