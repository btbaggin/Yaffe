struct PlatformInput;

struct YaffeInput
{
	char current_keyboard_state[INPUT_SIZE];
	char previous_keyboard_state[INPUT_SIZE];
	v2 mouse_position;

	u32 current_controller_buttons;
	u32 previous_controller_buttons;
	v2 left_stick;
	v2 right_stick;

    PlatformInput* platform;
	long last_input;
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

inline static bool InputDown(YaffeState* pState, char pInput[INPUT_SIZE], int pKey);
inline static bool InputUp(YaffeState* pState, char pInput[INPUT_SIZE], int pKey);
static int MapKey(int key);