class RestrictedPinModal : public ModalContent
{
	u32 index;
	PIN_CODE pin[PIN_SIZE];
	MODAL_RESULTS Update(float pDeltaTime)
	{
		if (IsKeyPressed(KEY_Enter) || IsControllerPressed(CONTROLLER_START)) return MODAL_RESULT_Ok;
		else if (IsKeyPressed(KEY_Escape) || IsControllerPressed(CONTROLLER_BACK)) return MODAL_RESULT_Cancel;

		if (index < PIN_SIZE)
		{
			u32 key = GetPressedKey() | GetPressedButton();
			if (key) pin[index++] = key;
		}
		return MODAL_RESULT_None;
	}

	void Render(RenderState* pState, v2 pPosition)
	{
		PushText(pState, FONT_Normal, "Enter up to 8 buttons, press Start to accept", pPosition, TEXT_FOCUSED);

		v2 char_size = MeasureString(FONT_Normal, "*");
		pPosition.Y += char_size.Height + UI_MARGIN;

		for (u32 i = 0; i < PIN_SIZE; i++)
		{
			v2 pos = pPosition + V2((char_size.Width + UI_MARGIN) * 2 * i, 0);
			PushSizedQuad(pState, pos, V2(char_size.Width * 2, char_size.Height), TEXT_UNFOCUSED);
			if (i < index) PushText(pState, FONT_Normal, "*", pos, TEXT_FOCUSED);
		}
	}

public:
	RestrictedPinModal()
	{
		SetSize(MODAL_SIZE_Full, 2);
		index = 0;
	}

	PIN_CODE* GetPin(u32* pLength)
	{
		*pLength = index;
		return pin;
	}
};
MODAL_CLOSE(OnPinClose)
{
	RestrictedMode* mode = pState->restrictions;
	if (pResult == MODAL_RESULT_Ok)
	{
		u32 length;
		PIN_CODE* pin = ((RestrictedPinModal*)pContent)->GetPin(&length);

		memcpy(mode->pin, pin, sizeof(PIN_CODE) * length);
		mode->pin_length = length;
		mode->state = RESTRICTED_MODE_On;
	}
	else
	{
		mode->state = RESTRICTED_MODE_Off;
	}
}

class VerifyActionModal : public ModalContent
{
	u32 tries;
	u32 index;
	PIN_CODE pin[PIN_SIZE];
	YaffeState* state;
	void* data;
	restricted_action* action;

	MODAL_RESULTS Update(float pDeltaTime)
	{
		if (IsKeyPressed(KEY_Escape) || IsControllerPressed(CONTROLLER_BACK) || tries == 3)
		{
			return MODAL_RESULT_Cancel;
		}

		if (index >= state->restrictions->pin_length)
		{
			index = 0;
			tries++;
		}

		u32 key = GetPressedKey() | GetPressedButton();
		if (key) pin[index++] = key;

		if (index == state->restrictions->pin_length &&
			memcmp(pin, state->restrictions->pin, sizeof(PIN_CODE) * state->restrictions->pin_length) == 0)
		{
			state->restrictions->last_access = clock();
			action(data);
			return MODAL_RESULT_Ok;
		}

		return MODAL_RESULT_None;
	}

	void Render(RenderState* pState, v2 pPosition)
	{
		PushText(pState, FONT_Normal, "This action requires approval. Enter PIN:", pPosition, TEXT_FOCUSED);

		v2 char_size = MeasureString(FONT_Normal, "*");
		pPosition.Y += char_size.Height + UI_MARGIN;

		for (u32 i = 0; i < state->restrictions->pin_length; i++)
		{
			v2 pos = pPosition + V2((char_size.Width + UI_MARGIN) * 2 * i, 0);
			PushSizedQuad(pState, pos, V2(char_size.Width * 2, char_size.Height), TEXT_UNFOCUSED);
			if (i < index) PushText(pState, FONT_Normal, "*", pos, TEXT_FOCUSED);
		}
	}

public:
	VerifyActionModal(YaffeState* pState, restricted_action* pAccept, void* pData)
	{
		SetSize(MODAL_SIZE_Full, 2);
		index = 0;
		state = pState;
		action = pAccept;
		data = pData;
		tries = 0;
	}
};

static void VerifyRestrictedAction(YaffeState* pState, restricted_action pResult, void* pData)
{
	RestrictedMode* mode = pState->restrictions;
	if (mode->state == RESTRICTED_MODE_On && 
		(clock() - mode->last_access) / (float)CLOCKS_PER_SEC > SECONDS_BEFORE_RETRY_VERIFY)
	{
		mode->modal = new ModalWindow();
		mode->modal->title = "Enter PIN";
		mode->modal->icon = BITMAP_None;
		mode->modal->button = nullptr;
		mode->modal->content = new VerifyActionModal(pState, pResult, pData);
		mode->modal->on_close = nullptr;
	}
	else
	{
		pResult(pData);
	}
}

static void EnableRestrictedMode(YaffeState* pState, RestrictedMode* pRestrictions)
{
	assert(pRestrictions->state == RESTRICTED_MODE_Off);

	pRestrictions->state = RESTRICTED_MODE_Pending;
	pRestrictions->last_access = (u64)(CLOCKS_PER_SEC * SECONDS_BEFORE_RETRY_VERIFY); //Force request on first try

	pRestrictions->modal = new ModalWindow();
	pRestrictions->modal->title = "Enter PIN";
	pRestrictions->modal->icon = BITMAP_None;
	pRestrictions->modal->button = nullptr;
	pRestrictions->modal->content = new RestrictedPinModal();
	pRestrictions->modal->on_close = OnPinClose;
}

static RESTRICTED_ACTION(DisableProtection) { g_state.restrictions->state = RESTRICTED_MODE_Off; }
static void DisableRestrictedMode(YaffeState* pState, RestrictedMode* pRestrictions)
{
	assert(pRestrictions->state == RESTRICTED_MODE_On);

	VerifyRestrictedAction(pState, DisableProtection, nullptr);
}