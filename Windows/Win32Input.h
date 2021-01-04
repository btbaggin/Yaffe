#pragma once

typedef struct
{
	unsigned long eventCount;
	WORD                                wButtons;
	BYTE                                bLeftTrigger;
	BYTE                                bRightTrigger;
	SHORT                               sThumbLX;
	SHORT                               sThumbLY;
	SHORT                               sThumbRX;
	SHORT                               sThumbRY;
} XINPUT_GAMEPAD_EX;

#define INPUT_SIZE 256
#include "../YaffeInput.h"

// returns 0 on success, 1167 on not connected. Might be others.
typedef int(__stdcall* get_gamepad_ex)(int, XINPUT_GAMEPAD_EX*);

struct PlatformInput
{
	get_gamepad_ex XInputGetState;
	HKL layout;
};

enum KEYS
{
	KEY_Backspace = 0x08,
	KEY_Tab = 0x09,
	KEY_Clear = 0x0C,
	KEY_Enter = 0x0D,
	KEY_Shift = 0x10,
	KEY_Control = 0x11,
	KEY_Alt = 0x12,
	KEY_Pause = 0x13,
	KEY_CapsLock = 0x14,
	KEY_Escape = 0x1B,
	KEY_Space = 0x20,
	KEY_PageUp = 0x21,
	KEY_PageDown = 0x22,
	KEY_End = 0x23,
	KEY_Home = 0x24,
	KEY_Left = 0x25,
	KEY_Up = 0x26,
	KEY_Right = 0x27,
	KEY_Down = 0x28,
	KEY_Print = 0x2A,
	KEY_PrintScreen = 0x2C,
	KEY_Insert = 0x2D,
	KEY_Delete = 0x2E,
	KEY_Help = 0x2F,

	KEY_Zero = 0x30,
	KEY_One = 0x31,
	KEY_Two = 0x32,
	KEY_Three = 0x33,
	KEY_Four = 0x34,
	KEY_Five = 0x35,
	KEY_Six = 0x36,
	KEY_Seven = 0x37,
	KEY_Eight = 0x38,
	KEY_Nine = 0x39,

	KEY_A = 0x41,
	KEY_B = 0x42,
	KEY_C = 0x43,
	KEY_D = 0x44,
	KEY_E = 0x45,
	KEY_F = 0x46,
	KEY_G = 0x47,
	KEY_H = 0x48,
	KEY_I = 0x49,
	KEY_J = 0x4A,
	KEY_K = 0x4B,
	KEY_L = 0x4C,
	KEY_M = 0x4D,
	KEY_N = 0x4E,
	KEY_O = 0x4F,
	KEY_P = 0x50,
	KEY_Q = 0x51,
	KEY_R = 0x52,
	KEY_S = 0x53,
	KEY_T = 0x54,
	KEY_U = 0x55,
	KEY_V = 0x56,
	KEY_W = 0x57,
	KEY_X = 0x58,
	KEY_Y = 0x59,
	KEY_Z = 0x5A,

	KEY_Num0 = 0x60,
	KEY_Num1 = 0x61,
	KEY_Num2 = 0x62,
	KEY_Num3 = 0x63,
	KEY_Num4 = 0x64,
	KEY_Num5 = 0x65,
	KEY_Num6 = 0x66,
	KEY_Num7 = 0x67,
	KEY_Num8 = 0x68,
	KEY_Num9 = 0x69,

	KEY_NumLock = 0x90,
	KEY_ScrollLock = 0x91,
	KEY_LShift = 0xA0,
	KEY_RShift = 0xA1,
	KEY_LControl = 0xA2,
	KEY_RControl = 0xA3,

	KEY_Semicolon = 0xBA,
	KEY_Plus = 0xBB,
	KEY_Comma = 0xBC,
	KEY_Minus = 0xBD,
	KEY_Period = 0xBE,
	KEY_Slash = 0xBF,
	KEY_Tilde = 0xC0,
	KEY_LBracket = 0xDB,
	KEY_Backslash = 0xDC,
	KEY_RBracket = 0xDD,
	KEY_Quote = 0xDE
};

inline static bool InputDown(YaffeState* pState, char pInput[INPUT_SIZE], int pKey)
{
	return (pInput[pKey] & 0x80) != 0;
} 
inline static bool InputUp(YaffeState* pState, char pInput[INPUT_SIZE], int pKey)
{
	return (pInput[pKey] & 0x80) == 0;
}
inline static bool IsKeyUp(KEYS pKey);
extern YaffeInput g_input;
static int MapKey(int key)
{
	if (IsKeyUp(KEY_Control))
	{
		UINT result = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
		WCHAR buffer[2];
		int chars = ToUnicodeEx(key, result, (const BYTE*)g_input.current_keyboard_state, buffer, 2, 0, g_input.platform->layout);
		if (chars == 1 && buffer[0] != '\t') return buffer[0];
	}
	return -1;
}