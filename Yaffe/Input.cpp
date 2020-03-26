#pragma once
#include "Input.h"

inline static bool IsKeyPressed(KEYS pKey)
{
	CHECK_KEY_DELAY
	//Current state is down
	//Previous state is up
	CHECK_KEY_INPUT((g_input.current_keyboard_state[pKey] & 0x80) != 0 &&
		(g_input.previous_keyboard_state[pKey] & 0x80) == 0)
}

inline static bool IsButtonPressed(MOUSE_BUTTONS pButton)
{
	CHECK_KEY_DELAY
		//Current state is down
		//Previous state is up
	CHECK_KEY_INPUT((g_input.current_keyboard_state[pButton] & 0x80) != 0 &&
		(g_input.previous_keyboard_state[pButton] & 0x80) == 0);
}
inline static bool IsControllerPressed(CONTROLLER_BUTTONS pController)
{
	CHECK_KEY_DELAY
		CHECK_KEY_INPUT((g_input.current_controller_buttons & pController) != 0 &&
		(g_input.previous_controller_buttons & pController) == 0);
}

inline static bool IsKeyReleased(KEYS pKey)
{
	//Current state is up
	//Previous state is down
	return (g_input.current_keyboard_state[pKey] & 0x80) == 0 &&
		(g_input.previous_keyboard_state[pKey] & 0x80) != 0;
}
inline static bool IsButtonReleased(MOUSE_BUTTONS pKey)
{
	//Current state is up
	//Previous state is down
	return (g_input.current_keyboard_state[pKey] & 0x80) == 0 &&
		(g_input.previous_keyboard_state[pKey] & 0x80) != 0;
}
inline static bool IsControllerReleased(CONTROLLER_BUTTONS pController)
{
	//Current state is up
	//Previous state is down
	return (g_input.current_controller_buttons & pController) == 0 &&
		(g_input.previous_controller_buttons & pController) != 0;
}

inline static bool IsKeyDown(KEYS pKey)
{
	CHECK_KEY_DELAY
	CHECK_KEY_INPUT((g_input.current_keyboard_state[pKey] & 0x80) != 0);
}
inline static bool IsButtonDown(MOUSE_BUTTONS pKey)
{
	CHECK_KEY_DELAY
		CHECK_KEY_INPUT((g_input.current_keyboard_state[pKey] & 0x80) != 0);
}
inline static bool IsControllerDown(CONTROLLER_BUTTONS pController)
{
	CHECK_KEY_DELAY
		CHECK_KEY_INPUT((g_input.current_controller_buttons & pController) != 0);
}

inline static bool IsKeyUp(KEYS pKey)
{
	return (g_input.current_keyboard_state[pKey] & 0x80) == 0;
}
inline static bool IsButtonUp(MOUSE_BUTTONS pKey)
{
	return (g_input.current_keyboard_state[pKey] & 0x80) == 0;
}
inline static bool IsControllerUp(CONTROLLER_BUTTONS pController)
{
	return (g_input.current_controller_buttons & pController) == 0;
}

inline static bool IsLeftStickPushed(DIRECTIONS pDirection)
{
	CHECK_KEY_DELAY
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

inline static bool IsRightStickPushed(DIRECTIONS pDirection)
{
	CHECK_KEY_DELAY
	switch (pDirection)
	{
	case DIRECTION_Up:
		CHECK_KEY_INPUT(g_input.right_stick.Y > 0 && abs(g_input.right_stick.Y) > abs(g_input.right_stick.X));
	case DIRECTION_Down:
		CHECK_KEY_INPUT(g_input.right_stick.Y < 0 && abs(g_input.right_stick.Y) > abs(g_input.right_stick.X));
	case DIRECTION_Left:
		CHECK_KEY_INPUT(g_input.right_stick.X < 0 && abs(g_input.right_stick.X) > abs(g_input.right_stick.Y));
	case DIRECTION_Right:
		CHECK_KEY_INPUT(g_input.right_stick.X > 0 && abs(g_input.right_stick.X) > abs(g_input.right_stick.Y));
	}

	return false;
}
inline static v2 GetMousePosition()
{
	return g_input.mouse_position;
}

void Win32GetInput(YaffeInput* pInput, HWND pHandle)
{
	memcpy(pInput->previous_keyboard_state, pInput->current_keyboard_state, 256);
	pInput->previous_controller_buttons = pInput->current_controller_buttons;

	GetKeyboardState((PBYTE)pInput->current_keyboard_state);

	POINT point;
	GetCursorPos(&point);
	ScreenToClient(pHandle, &point);

	pInput->mouse_position = { (float)point.x, (float)point.y };

	XINPUT_STATE state = {};
	DWORD result = XInputGetState(0, &state);// pInput->get_state(0, &state);
	if (result == ERROR_SUCCESS)
	{
		pInput->current_controller_buttons = state.Gamepad.wButtons;
		pInput->left_stick = { (float)state.Gamepad.sThumbLX, (float)state.Gamepad.sThumbLY };
		pInput->right_stick = { (float)state.Gamepad.sThumbRX, (float)state.Gamepad.sThumbRY };


		float length = HMM_LengthSquaredVec2(pInput->left_stick);
		if (length <= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE * XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			pInput->left_stick = { 0, 0 };
		}
		length = HMM_LengthSquaredVec2(pInput->right_stick);
		if (length <= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE * XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		{
			pInput->right_stick = { 0, 0 };
		}
	}
}