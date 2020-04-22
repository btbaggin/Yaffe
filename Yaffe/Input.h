#pragma once
const s32 GLOBAL_INPUT_DELAY = 100;
#define CHECK_KEY_DELAY if(g_input.last_input + GLOBAL_INPUT_DELAY > clock()) return false;
#define CHECK_KEY_INPUT(expression) if(expression) { g_input.last_input = clock(); return true; } else { return false; }

#define IsUpPressed() (IsKeyPressed(KEY_Up) || IsLeftStickPushed(DIRECTION_Up) || IsControllerPressed(CONTROLLER_DPAD_UP))
#define IsDownPressed() (IsKeyPressed(KEY_Down) || IsLeftStickPushed(DIRECTION_Down) || IsControllerPressed(CONTROLLER_DPAD_DOWN))
#define IsLeftPressed() (IsKeyPressed(KEY_Left) || IsLeftStickPushed(DIRECTION_Left) || IsControllerPressed(CONTROLLER_DPAD_LEFT))
#define IsRightPressed() (IsKeyPressed(KEY_Right) || IsLeftStickPushed(DIRECTION_Right) || IsControllerPressed(CONTROLLER_DPAD_RIGHT))
#define IsEnterPressed() (IsKeyPressed(KEY_Enter) || IsControllerPressed(CONTROLLER_A))
#define IsEscPressed() (IsKeyPressed(KEY_Escape) || IsControllerPressed(CONTROLLER_B))
#define IsFilterPressed() (IsKeyPressed(KEY_F1) || IsControllerPressed(CONTROLLER_Y))
#define IsInfoPressed() (IsKeyPressed(KEY_I) || IsControllerPressed(CONTROLLER_X))

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

// returns 0 on success, 1167 on not connected. Might be others.
typedef int(__stdcall * get_gamepad_ex)(int, XINPUT_GAMEPAD_EX*);


//https://forums.tigsource.com/index.php?topic=26792.0
struct YaffeInput
{
	char current_keyboard_state[256];
	char previous_keyboard_state[256];
	v2 mouse_position;

	DWORD current_controller_buttons;
	DWORD previous_controller_buttons;
	v2 left_stick;
	v2 right_stick;

	get_gamepad_ex XInputGetState;

	long last_input;
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

	KEY_F1 = 0x70,
	KEY_F2 = 0x71,
	KEY_F3 = 0x72,
	KEY_F4 = 0x73,
	KEY_F5 = 0x74,
	KEY_F6 = 0x75,
	KEY_F7 = 0x76,
	KEY_F8 = 0x77,
	KEY_F9 = 0x78,
	KEY_F10 = 0x79,
	KEY_F11 = 0x7A,
	KEY_F12 = 0x7B,

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

enum MOUSE_BUTTONS
{
	BUTTON_Left = 0x01,
	BUTTON_Right = 0x02,
	BUTTON_Middle = 0x04,
	BUTTON_X1 = 0x05,
	BUTTON_X2 = 0x06
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