#pragma once

enum RESTRICTED_MODE_STATES
{
	RESTRICTED_MODE_Off,
	RESTRICTED_MODE_Pending,
	RESTRICTED_MODE_On,
};
const u32 PIN_SIZE = 8;
const float SECONDS_BEFORE_RETRY_VERIFY = 60;

#define RESTRICTED_ACTION(name) void name(void* pData)
typedef RESTRICTED_ACTION(restricted_action);

#define PIN_CODE u32

struct RestrictedMode
{
	RESTRICTED_MODE_STATES state;
	PIN_CODE pin[PIN_SIZE];
	u32 pin_length;
	u64 last_attempt;
	ModalWindow* modal;
	bool access_granted;
};

static void EnableRestrictedMode(YaffeState* pState, RestrictedMode* pRestrictions);
static void DisableRestrictedMode(YaffeState* pState, RestrictedMode* pRestrictions);
static void VerifyRestrictedAction(YaffeState* pState, restricted_action pResult, void* pData, bool pCheckTimeout = true);