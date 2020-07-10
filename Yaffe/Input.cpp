#pragma once

inline static bool IsKeyPressed(KEYS pKey)
{
	if (g_input.last_input + GLOBAL_INPUT_DELAY > clock()) return false;
	CHECK_KEY_INPUT(InputDown(current_keyboard_state, pKey) && InputUp(previous_keyboard_state, pKey));
}
inline static bool IsKeyPressedWithoutDelay(KEYS pKey)
{
	return InputDown(current_keyboard_state, pKey) && InputUp(previous_keyboard_state, pKey);
}
inline static bool IsButtonPressed(MOUSE_BUTTONS pButton)
{
	CHECK_KEY_INPUT(InputDown(current_keyboard_state, pButton) && InputUp(previous_keyboard_state, pButton));
}
inline static bool IsControllerPressed(CONTROLLER_BUTTONS pController)
{
	if (g_input.last_input + GLOBAL_INPUT_DELAY > clock()) return false;
	CHECK_KEY_INPUT((g_input.current_controller_buttons & pController) != 0 &&
					(g_input.previous_controller_buttons & pController) == 0);
}

inline static bool IsKeyReleased(KEYS pKey)
{
	return InputUp(current_keyboard_state, pKey) && InputDown(previous_keyboard_state, pKey);
}
inline static bool IsButtonReleased(MOUSE_BUTTONS pKey)
{
	return InputUp(current_keyboard_state, pKey) && InputDown(previous_keyboard_state, pKey);
}
inline static bool IsControllerReleased(CONTROLLER_BUTTONS pController)
{
	return (g_input.current_controller_buttons & pController) == 0 &&
		   (g_input.previous_controller_buttons & pController) != 0;
}

inline static bool IsKeyDown(KEYS pKey)
{
	CHECK_KEY_INPUT(InputDown(current_keyboard_state, pKey));
}
inline static bool IsButtonDown(MOUSE_BUTTONS pKey)
{
	CHECK_KEY_INPUT(InputDown(current_keyboard_state, pKey));
}
inline static bool IsControllerDown(CONTROLLER_BUTTONS pController)
{
	CHECK_KEY_INPUT((g_input.current_controller_buttons & pController) != 0);
}

inline static bool IsKeyUp(KEYS pKey)
{
	return InputUp(current_keyboard_state, pKey);
}
inline static bool IsButtonUp(MOUSE_BUTTONS pKey)
{
	return InputUp(current_keyboard_state, pKey);
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
		if (magnitude > 32767) magnitude = 32767;
		magnitude -= XINPUT_INPUT_DEADZONE;

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