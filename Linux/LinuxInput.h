#pragma once

#define INPUT_SIZE 33
#include "../YaffeInput.h"

struct PlatformInput
{
	int joystick;
};
#include "Joystick.cpp"

enum KEYS
{
	KEY_Backspace = XK_BackSpace,
	KEY_Tab = XK_Tab,
	KEY_Clear = XK_Clear,
	KEY_Enter = XK_Return,
	KEY_Shift = XK_Shift_L,
	KEY_Control = XK_Control_L,
	KEY_Alt = XK_Alt_L,
	KEY_Pause = XK_Pause,
	KEY_CapsLock = XK_Caps_Lock,
	KEY_Escape = XK_Escape,
	KEY_Space = XK_KP_Space,
	KEY_PageUp = XK_Page_Up,
	KEY_PageDown = XK_Page_Down,
	KEY_End = XK_End,
	KEY_Home = XK_Home,
	KEY_Left = XK_Left,
	KEY_Up = XK_Up,
	KEY_Right = XK_Right,
	KEY_Down = XK_Down,
	KEY_Print = XK_Print,
	KEY_PrintScreen = XK_Print,
	KEY_Insert = XK_Insert,
	KEY_Delete = XK_Delete,
	KEY_Help = XK_Help,

	KEY_Zero = XK_0,
	KEY_One = XK_1,
	KEY_Two = XK_2,
	KEY_Three = XK_3,
	KEY_Four = XK_4,
	KEY_Five = XK_5,
	KEY_Six = XK_6,
	KEY_Seven = XK_7,
	KEY_Eight = XK_8,
	KEY_Nine = XK_9,

#undef KEY_A
#undef KEY_B
#undef KEY_C
#undef KEY_D
#undef KEY_E
#undef KEY_F
#undef KEY_G
#undef KEY_H
#undef KEY_I
#undef KEY_J
#undef KEY_K
#undef KEY_L
#undef KEY_M
#undef KEY_N
#undef KEY_O
#undef KEY_P
#undef KEY_Q
#undef KEY_R
#undef KEY_S
#undef KEY_T
#undef KEY_U
#undef KEY_V
#undef KEY_W
#undef KEY_X
#undef KEY_Y
#undef KEY_Z
	KEY_A = XK_A,
	KEY_B = XK_B,
	KEY_C = XK_C,
	KEY_D = XK_D,
	KEY_E = XK_E,
	KEY_F = XK_F,
	KEY_G = XK_G,
	KEY_H = XK_H,
	KEY_I = XK_I,
	KEY_J = XK_J,
	KEY_K = XK_K,
	KEY_L = XK_L,
	KEY_M = XK_M,
	KEY_N = XK_N,
	KEY_O = XK_O,
	KEY_P = XK_R,
	KEY_Q = XK_Q,
	KEY_R = XK_R,
	KEY_S = XK_S,
	KEY_T = XK_T,
	KEY_U = XK_U,
	KEY_V = XK_V,
	KEY_W = XK_W,
	KEY_X = XK_X,
	KEY_Y = XK_Y,
	KEY_Z = XK_Z,

	KEY_Num0 = XK_KP_0,
	KEY_Num1 = XK_KP_1,
	KEY_Num2 = XK_KP_2,
	KEY_Num3 = XK_KP_3,
	KEY_Num4 = XK_KP_4,
	KEY_Num5 = XK_KP_5,
	KEY_Num6 = XK_KP_6,
	KEY_Num7 = XK_KP_7,
	KEY_Num8 = XK_KP_8,
	KEY_Num9 = XK_KP_9,

	KEY_LShift = XK_Shift_L,
	KEY_RShift = XK_Shift_R,
	KEY_LControl = XK_Control_L,
	KEY_RControl = XK_Control_R,

	KEY_Semicolon = XK_semicolon,
	KEY_Plus = XK_plus,
	KEY_Comma = XK_comma,
	KEY_Minus = XK_minus,
	KEY_Period = XK_period,
	KEY_Slash = XK_slash,
	KEY_Tilde = XK_asciitilde,
	KEY_LBracket = XK_braceleft,
	KEY_Backslash = XK_backslash,
	KEY_RBracket = XK_braceright,
	KEY_Quote = XK_quotedbl,
};

inline static bool InputDown(YaffeState* pState, char pInput[INPUT_SIZE], int pKey)
{
	if (pKey <= BUTTON_Middle)
	{
		return pInput[INPUT_SIZE - 1] & pKey;
	}
	else
	{
		KeyCode k = XKeysymToKeycode(pState->form->platform->display, pKey);
    	return (pInput[k >> 3] & (1 << (k & 7)));
	}
} 
inline static bool InputUp(YaffeState* pState, char pInput[INPUT_SIZE], int pKey)
{
	if (pKey <= BUTTON_Middle)
	{
		return !(pInput[INPUT_SIZE - 1] & pKey);
	}
	else
	{
		KeyCode k = XKeysymToKeycode(pState->form->platform->display, pKey);
    	return !(pInput[k >> 3] & (1 << (k & 7)));
	}
}

static int MapKey(int key)
{
	return key;
}