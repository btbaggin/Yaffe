//
// TEXTBOX
//
#include <stdlib.h>
#include <string.h> // memmove
#include <ctype.h>  // isspace

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
	str->string = (char*)realloc(str->string, str->stringlen + num + 1);
	memmove(&str->string[pos + num], &str->string[pos], str->stringlen - pos);
	memcpy(&str->string[pos], newtext, num);
	str->stringlen += num;
	str->string[str->stringlen] = '\0';
	return 1; // always succeeds
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
	tc.enabled = true;
	tc.font_x = 0;
	stb_textedit_initialize_state(&tc.state, true);
	return tc;
}



static void RenderTextbox(RenderState* pState, Textbox* tc, v2 pPosition)
{
	v2 pos = GetMousePosition() - pPosition;
	float font_size = GetFontSize(tc->font);
	//
	// HANDLE INPUT
	//
	if (tc->enabled)
	{
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

			if (control && IsKeyPressedWithoutDelay(KEY_V))
			{
				char* text = GetClipboardText(&g_state);
				if(text) stb_textedit_paste(tc, &tc->state, text, (int)strlen(text));
			}

			for (int i = 0; i <= ArrayCount(INPUT_KEY_MAP); i++)
			{
				int key = INPUT_KEY_MAP[i];
				if (IsKeyPressedWithoutDelay((KEYS)key))
				{
					if (shift) key |= STB_TEXTEDIT_K_SHIFT;
					if (control) key |= STB_TEXTEDIT_K_CONTROL;
					stb_textedit_key(tc, &tc->state, key);
				}
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
	PushSizedQuad(pState, pPosition, V2(tc->width, font_size), tc->enabled ? ELEMENT_BACKGROUND : ELEMENT_BACKGROUND_NOT_ENABLED);

	//Selection
	if (tc->focused)
	{
		int start_index = std::min(tc->state.select_start, tc->state.select_end);
		int end_index = std::max(tc->state.select_start, tc->state.select_end);
		start_index = std::min(start_index, tc->stringlen);
		end_index = std::min(end_index, tc->stringlen);
		if (end_index > 0)
		{
			StbFindState start = {};
			StbFindState end = {};
			stb_textedit_find_charpos(&start, tc, start_index, true);
			stb_textedit_find_charpos(&end, tc, end_index, true);
			PushQuad(pState, pPosition + V2(std::max(start.x - tc->font_x, 0.0F), start.y), pPosition + V2(std::min(end.x, tc->width), end.y + font_size), V4(0, 0.5F, 1, 1));
		}
	}

	//Text
	if (tc->string)
	{
		v2 position = pPosition - V2(tc->font_x, 0);
		PushClippedText(pState, tc->font, tc->string, position, TEXT_FOCUSED, pPosition, V2(tc->width, font_size));
	}

	//Cursor
	if (tc->focused)
	{
		StbFindState state = {};
		stb_textedit_find_charpos(&state, tc, tc->state.cursor, true);
		
		if (state.x - tc->font_x > tc->width)
		{
			//Move font left if the cursor is too far to the right
			tc->font_x += (state.x - tc->font_x - tc->width);
		}
		else if (state.x < tc->font_x)
		{
			//Move font right if the cursor is too far to the left
			tc->font_x -= tc->font_x - state.x;
		}

		v2 position = pPosition - V2(tc->font_x, 0) + V2(state.x, state.y);
		PushSizedQuad(pState, position, V2(2, font_size), V4(1));
	}
}

static void SetText(Textbox* pTextbox, char* pText)
{
	pTextbox->stringlen = (int)strlen(pText);
	pTextbox->string = (char*)realloc(pTextbox->string, pTextbox->stringlen);
	strcpy(pTextbox->string, pText);
}

static void SetText(FilePathBox* pTextbox, char* pText)
{
	strcpy(pTextbox->string, pText);
}


//
// CHECKBOX
//
static Checkbox CreateCheckbox()
{
	Checkbox box;
	box.checked = false;
	box.enabled = true;
	return box;
}

static void RenderCheckbox(RenderState* pState, Checkbox* pCheck, v2 pPosition)
{
	const float SIZE = 24.0F;

	if (pCheck->enabled)
	{
		v2 pos = GetMousePosition() - pPosition;
		if (pos > V2(0) && pos < V2(SIZE))
		{
			if (IsButtonPressed(BUTTON_Left)) pCheck->checked = !pCheck->checked;
		}
	}

	PushSizedQuad(pState, pPosition, V2(SIZE), pCheck->enabled ? ELEMENT_BACKGROUND : ELEMENT_BACKGROUND_NOT_ENABLED);
	if (pCheck->checked)
	{
		PushSizedQuad(pState, pPosition + V2(3), V2(18), ACCENT_COLOR);
	}
}

//
// FILEPATHBOX
//
static FilePathBox CreateFilePathBox(float pWidth, FONTS pFont, bool pAllowFiles)
{
	FilePathBox fpb = {};
	fpb.width = pWidth;
	fpb.font = pFont;
	fpb.enabled = true;
	fpb.files = pAllowFiles;
	return fpb;
}

static void RenderFilePathBox(RenderState* pState, FilePathBox* pBox, v2 pPosition)
{
	v2 pos = GetMousePosition() - pPosition;
	float font_size = GetFontSize(pBox->font);

	if (pBox->enabled && IsButtonPressed(BUTTON_Left))
	{
		//Toggle focus
		if(pos.X > 0 && pos.Y > 0 && pos < V2(pBox->width, font_size))
		{
			char* result = nullptr;
#ifdef __linux
			tinyfd_allowCursesDialogs = 1;
#endif
			if (pBox->files) result = tinyfd_openFileDialog("Choose file", "", 0, NULL, NULL, 0);
			else result = tinyfd_selectFolderDialog("Choose folder", NULL);
			
			if (result) strcpy(pBox->string, result);
		}
	}

	//Background
	PushSizedQuad(pState, pPosition, V2(pBox->width, font_size), pBox->enabled ? ELEMENT_BACKGROUND : ELEMENT_BACKGROUND_NOT_ENABLED);

	//Text
	if (pBox->string)
	{
		PushClippedText(pState, pBox->font, pBox->string, pPosition, TEXT_FOCUSED, pPosition, V2(pBox->width, font_size));
	}
}