#include <linux/joystick.h>

static bool CheckForJoystick(PlatformInput* pInput, const char* pName) 
{
     // Opens device in blocking mode.
    if (pInput->joystick == -1)
    {
        pInput->joystick = open(pName, O_RDONLY);
        if (pInput->joystick == -1) return false;

        // Changes into a NON-BLOCKING-MODE.
        fcntl(pInput->joystick, F_SETFL, O_NONBLOCK);
    }

    return true;
}

void endDeviceConnection(PlatformInput* pInput) 
{
    if(pInput->joystick != -1) close(pInput->joystick);
}

static void ProcessEvent(YaffeInput* pInput, struct js_event e) 
{
    /*
     * The JS_EVENT_INIT bits will be deactivated as we do not want to distinguish
     * between synthetic and real events.
     */
    switch (e.type & ~JS_EVENT_INIT) 
    {
        case JS_EVENT_AXIS:
            switch (e.number) 
            {
				case 0:	pInput->left_stick.X = e.value; break;
				case 1:	pInput->left_stick.Y = -e.value; break;
				case 3:	pInput->right_stick.X = e.value; break;
				case 4:	pInput->right_stick.Y = -e.value; break;
                //2 is left trigger
                //5 is right trigger
            }
            break;

        case JS_EVENT_BUTTON:
            int button;
            switch(e.number)
            {
	            case 0: button = CONTROLLER_A; break;
				case 1: button = CONTROLLER_B; break;
				case 2: button = CONTROLLER_X; break;
				case 3: button = CONTROLLER_Y; break;
				case 4: button = CONTROLLER_LEFT_SHOULDER; break;
				case 5: button = CONTROLLER_RIGHT_SHOULDER; break;
				case 6: button = CONTROLLER_BACK; break;
				case 7: button = CONTROLLER_START; break;
				case 8: button = CONTROLLER_GUIDE; break; 
				case 9: button = CONTROLLER_LEFT_THUMB; break;
				case 10: button = CONTROLLER_RIGHT_THUMB; break;
				default: button = 0; break;
            }

            if (e.value) pInput->current_controller_buttons |= button;
            else pInput->current_controller_buttons ^= button;
            break;

        default:
            break;
    }
}


void GetJoystickInput(YaffeInput* pInput, int pJoystick) 
{
    struct js_event jsEvent;

    /* read all events from the driver stack! */
    while (read(pJoystick, &jsEvent, sizeof(struct js_event)) > 0) 
    {
        ProcessEvent(pInput, jsEvent);
    }
}