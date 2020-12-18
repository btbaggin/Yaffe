#pragma once

struct XINPUT_GAMEPAD_EX
{
};

#define XBOX_BUTTON int
#define INPUT_SIZE 33

// returns 0 on success, 1167 on not connected. Might be others.
typedef int get_gamepad_ex(int, XINPUT_GAMEPAD_EX*);


//https://forums.tigsource.com/index.php?topic=26792.0
struct YaffeInput
{
	char current_keyboard_state[INPUT_SIZE];
	char previous_keyboard_state[INPUT_SIZE];
	v2 mouse_position;

	XBOX_BUTTON current_controller_buttons;
	XBOX_BUTTON previous_controller_buttons;
	v2 left_stick;
	v2 right_stick;

	get_gamepad_ex XInputGetState;

	long last_input;
};

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

	KEY_F1 = XK_F1,
	KEY_F2 = XK_F2,
	KEY_F3 = XK_F3,
	KEY_F4 = XK_F4,
	KEY_F5 = XK_F5,
	KEY_F6 = XK_F6,
	KEY_F7 = XK_F7,
	KEY_F8 = XK_F8,
	KEY_F9 = XK_F9,
	KEY_F10 = XK_F10,
	KEY_F11 = XK_F11,
	KEY_F12 = XK_F12,

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
	KEY_Quote = XK_quotedbl
};

enum MOUSE_BUTTONS
{
	BUTTON_Left = 0x01,
	BUTTON_Right = 0x02,
	BUTTON_Middle = 0x04,
};

enum CONTROLLER_BUTTONS
{
	CONTROLLER_DPAD_UP = 0x0001,
	CONTROLLER_DPAD_DOWN = 0x0002,
	CONTROLLER_DPAD_LEFT = 0x0004,
	CONTROLLER_DPAD_RIGHT = 0x0008,
	CONTROLLER_START = 0x0010,
	CONTROLLER_BACK = 0x0020,
	CONTROLLER_LEFT_THUMB = 0x0040,
	CONTROLLER_RIGHT_THUMB = 0x0080,
	CONTROLLER_LEFT_SHOULDER = 0x0100,
	CONTROLLER_RIGHT_SHOULDER = 0x0200,
	CONTROLLER_GUIDE = 0x0400,
	CONTROLLER_A = 0x1000,
	CONTROLLER_B = 0x2000,
	CONTROLLER_X = 0x4000,
	CONTROLLER_Y = 0x8000,
};

enum DIRECTIONS
{
	DIRECTION_Up,
	DIRECTION_Down,
	DIRECTION_Left,
	DIRECTION_Right,
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