#pragma once
const s32 GLOBAL_INPUT_DELAY = 100;
#define CHECK_KEY_INPUT(expression) if(expression) { g_input.last_input = clock(); return true; } else { return false; }
const float XINPUT_INPUT_DEADZONE = 7849.0F;

#define IsUpPressed() (IsKeyPressed(KEY_Up) || IsLeftStickPushed(DIRECTION_Up) || IsControllerPressed(CONTROLLER_DPAD_UP))
#define IsDownPressed() (IsKeyPressed(KEY_Down) || IsLeftStickPushed(DIRECTION_Down) || IsControllerPressed(CONTROLLER_DPAD_DOWN))
#define IsLeftPressed() (IsKeyPressed(KEY_Left) || IsLeftStickPushed(DIRECTION_Left) || IsControllerPressed(CONTROLLER_DPAD_LEFT))
#define IsRightPressed() (IsKeyPressed(KEY_Right) || IsLeftStickPushed(DIRECTION_Right) || IsControllerPressed(CONTROLLER_DPAD_RIGHT))
#define IsEnterPressed() (IsKeyPressed(KEY_Enter) || IsControllerPressed(CONTROLLER_A))
#define IsEscPressed() (IsKeyPressed(KEY_Escape) || IsControllerPressed(CONTROLLER_B))
#define IsFilterPressed() (IsKeyPressed(KEY_F) || IsControllerPressed(CONTROLLER_Y))
#define IsInfoPressed() (IsKeyPressed(KEY_I) || IsControllerPressed(CONTROLLER_X))

const int INPUT_KEY_MAP[] = {
	KEY_Backspace,
	KEY_Tab,
	KEY_Enter,
	KEY_Shift,
	KEY_Control,
	KEY_Alt,
	KEY_CapsLock,
	KEY_Escape,
	KEY_Space,
	KEY_PageUp,
	KEY_PageDown,
	KEY_End,
	KEY_Home,
	KEY_Left,
	KEY_Up,
	KEY_Right,
	KEY_Down,
	KEY_Delete,

	KEY_Zero,
	KEY_One,
	KEY_Two,
	KEY_Three,
	KEY_Four,
	KEY_Five,
	KEY_Six,
	KEY_Seven,
	KEY_Eight,
	KEY_Nine,

	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,

	KEY_LShift,
	KEY_RShift,
	KEY_LControl,
	KEY_RControl,

	KEY_Semicolon,
	KEY_Plus,
	KEY_Comma,
	KEY_Minus,
	KEY_Period,
	KEY_Slash,
	KEY_Tilde,
	KEY_LBracket,
	KEY_Backslash,
	KEY_RBracket,
	KEY_Quote
};

struct InputMessage
{
	v2 cursor;
	bool down;
	union
	{
		MOUSE_BUTTONS button;
		KEYS key;
	};
	float scroll;
};


inline static bool IsKeyPressed(KEYS pKey)
{
	if (g_input.last_input + GLOBAL_INPUT_DELAY > clock()) return false;
	CHECK_KEY_INPUT(InputDown(&g_state, g_input.current_keyboard_state, pKey) && 
					InputUp(&g_state, g_input.previous_keyboard_state, pKey));
}
inline static bool IsKeyPressedWithoutDelay(KEYS pKey)
{
	return InputDown(&g_state, g_input.current_keyboard_state, pKey) && 
		   InputUp(&g_state, g_input.previous_keyboard_state, pKey);
}
inline static bool IsButtonPressed(MOUSE_BUTTONS pButton)
{
	CHECK_KEY_INPUT(InputDown(&g_state, g_input.current_keyboard_state, pButton) && 
					InputUp(&g_state, g_input.previous_keyboard_state, pButton));
}
inline static bool IsControllerPressed(CONTROLLER_BUTTONS pController)
{
	if (g_input.last_input + GLOBAL_INPUT_DELAY > clock()) return false;
	CHECK_KEY_INPUT((g_input.current_controller_buttons & pController) != 0 &&
					(g_input.previous_controller_buttons & pController) == 0);
}
inline static u32 GetPressedButton()
{
	for (u32 i = CONTROLLER_DPAD_UP; i <= CONTROLLER_Y; i++)
	{
		if (IsControllerPressed((CONTROLLER_BUTTONS)i)) return i;
	}
	return 0;
}

inline static u32 GetPressedKey()
{
	for (u32 i = 0; i < ArrayCount(INPUT_KEY_MAP); i++)
	{
		if (IsKeyPressedWithoutDelay((KEYS)INPUT_KEY_MAP[i])) return i;
	}
	return 0;
}


inline static bool IsKeyReleased(KEYS pKey)
{
	return InputUp(&g_state, g_input.current_keyboard_state, pKey) && InputDown(&g_state, g_input.previous_keyboard_state, pKey);
}
inline static bool IsButtonReleased(MOUSE_BUTTONS pKey)
{
	return InputUp(&g_state, g_input.current_keyboard_state, pKey) && InputDown(&g_state, g_input.previous_keyboard_state, pKey);
}
inline static bool IsControllerReleased(CONTROLLER_BUTTONS pController)
{
	return (g_input.current_controller_buttons & pController) == 0 &&
		   (g_input.previous_controller_buttons & pController) != 0;
}

inline static bool IsKeyDown(KEYS pKey)
{
	CHECK_KEY_INPUT(InputDown(&g_state, g_input.current_keyboard_state, pKey));
}
inline static bool IsButtonDown(MOUSE_BUTTONS pKey)
{
	CHECK_KEY_INPUT(InputDown(&g_state, g_input.current_keyboard_state, pKey));
}
inline static bool IsControllerDown(CONTROLLER_BUTTONS pController)
{
	CHECK_KEY_INPUT((g_input.current_controller_buttons & pController) != 0);
}

inline static bool IsKeyUp(KEYS pKey)
{
	return InputUp(&g_state, g_input.current_keyboard_state, pKey);
}
inline static bool IsButtonUp(MOUSE_BUTTONS pKey)
{
	return InputUp(&g_state, g_input.current_keyboard_state, pKey);
}
inline static bool IsControllerUp(CONTROLLER_BUTTONS pController)
{
	return (g_input.current_controller_buttons & pController) == 0;
}

inline static bool IsLeftStickPushed(DIRECTIONS pDirection)
{
	if (g_input.last_input + GLOBAL_INPUT_DELAY > clock()) return false;
	switch (pDirection)
	{
	case DIRECTION_Up:
		CHECK_KEY_INPUT(g_input.left_stick.Y > 0 && abs(g_input.left_stick.Y) > abs(g_input.left_stick.X));
	case DIRECTION_Down:
		CHECK_KEY_INPUT(g_input.left_stick.Y < 0 && abs(g_input.left_stick.Y) > abs(g_input.left_stick.X));
	case DIRECTION_Left:
		CHECK_KEY_INPUT(g_input.left_stick.X < 0 && abs(g_input.left_stick.X) > abs(g_input.left_stick.Y));
	case DIRECTION_Right:
		CHECK_KEY_INPUT(g_input.left_stick.X > 0 && abs(g_input.left_stick.X) > abs(g_input.left_stick.Y));
	}

	return false;
}

inline static v2 NormalizeStickInput(v2 pStick)
{
	v2 dir = pStick;

	//determine how far the controller is pushed
	float magnitude = HMM_Length(dir);
	//check if the controller is outside a circular dead zone
	if (magnitude > XINPUT_INPUT_DEADZONE)
	{
		//giving a magnitude value of 0.0 to 1.0
		v2 normalizedDir = dir / (32767 - XINPUT_INPUT_DEADZONE);
		normalizedDir.Y *= -1;

		return normalizedDir;
	}
	return V2(0);
}

inline static v2 GetMousePosition()
{
	return g_input.mouse_position;
}