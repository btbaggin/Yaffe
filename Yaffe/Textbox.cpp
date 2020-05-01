#include <stdlib.h>
#include <string.h> // memmove
#include <ctype.h>  // isspace
#include <WinUser.h>

#define STB_TEXTEDIT_CHARTYPE   char
#define STB_TEXTEDIT_STRING     TextBox

// get the base type
#include "stb/stb_textedit.h"

// define our editor structure
struct TextBox
{
	char *string;
	int stringlen;
	FONTS font;
	v2 position;
	bool focused;
	float width;
	STB_TexteditState state;
};

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
		int result = MapVirtualKeyA(key, MAPVK_VK_TO_CHAR);
		return result;
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

static TextBox CreateTextBox(v2 pPosition, float pWidth, FONTS pFont)
{
	TextBox tc = {};
	tc.position = pPosition;
	tc.width = pWidth;
	tc.font = pFont;
	stb_textedit_initialize_state(&tc.state, true);
	return tc;
}

static void RenderTextBox(RenderState* pState, TextBox* tc)
{
	v2 pos = GetMousePosition() - tc->position;
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
		for (int i = KEY_Backspace; i < KEY_Quote; i++)
		{
			int key = i;
			if (IsKeyPressed((KEYS)key, false))
			{
				if (IsKeyDown(KEY_Shift)) key |= STB_TEXTEDIT_K_SHIFT;
				if (IsKeyDown(KEY_Control)) key |= STB_TEXTEDIT_K_CONTROL;
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
		PushQuad(pState, tc->position - V2(2), tc->position + V2(tc->width, font_size) + V2(2), ACCENT_COLOR);
	}
	//Background
	PushQuad(pState, tc->position, tc->position + V2(tc->width, font_size), V4(0.5F));

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
			PushQuad(pState, tc->position + V2(start.x, start.y), tc->position + V2(end.x, end.y + font_size), V4(0, 0.5F, 1, 1));
		}
	}

	//Text
	if (tc->string)
	{
		PushText(pState, tc->font, tc->string, tc->position);
	}

	//Cursor
	if (tc->focused)
	{
		StbFindState state = {};
		stb_textedit_find_charpos(&state, tc, tc->state.cursor, true);
		PushQuad(pState, tc->position + V2(state.x, state.y), tc->position + V2(state.x + 2, state.y + font_size), V4(1));
	}
}