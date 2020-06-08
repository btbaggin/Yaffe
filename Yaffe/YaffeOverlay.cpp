static void UpdateOverlay(Overlay* pOverlay, float pDeltaTime)
{
	if (pOverlay->process)
	{
		if (pOverlay->allow_input)
		{
			v2 dir = GetLeftStickVector();
			v2 mouse = GetMousePosition();

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
				v2 move = normalizedDir * 500 * pDeltaTime;

				SetCursor(mouse + move);
			}

			struct KeyMapping
			{
				CONTROLLER_BUTTONS cont;
				bool is_key;
				u32 input;
				bool do_shift;
			};
			KeyMapping mappings[5] = {
				{CONTROLLER_RIGHT_SHOULDER, true, KEY_Tab},
				{CONTROLLER_LEFT_SHOULDER, true, KEY_Tab, true},
				{CONTROLLER_START, true, KEY_Enter},
				{CONTROLLER_A, false, BUTTON_Left},
				{CONTROLLER_X, false, BUTTON_Right},
			};

			for (u32 i = 0; i < ArrayCount(mappings); i++)
			{
				KeyMapping key = mappings[i];
				if (IsControllerPressed(key.cont))
				{
					if (key.do_shift) SendKeyMessage(KEY_Shift, true);
					if (key.is_key) SendKeyMessage((KEYS)key.input, true);
					else SendMouseMessage((MOUSE_BUTTONS)key.input, true);
				}
				else if (IsControllerReleased(key.cont))
				{
					if (key.do_shift) SendKeyMessage(KEY_Shift, false);
					if (key.is_key) SendKeyMessage((KEYS)key.input, false);
					else SendMouseMessage((MOUSE_BUTTONS)key.input, false);
				}
			}
		}

		if (IsControllerPressed(CONTROLLER_GUIDE) || IsKeyPressed(KEY_Escape))
		{
			pOverlay->showing = !pOverlay->showing;

			if (pOverlay->showing) ShowOverlay(pOverlay);
			else CloseOverlay(pOverlay, false);
		} 
		else if (pOverlay->showing && IsEnterPressed())
		{
			CloseOverlay(pOverlay, true);
			delete pOverlay->process; pOverlay->process = nullptr;
			pOverlay->showing = false;
		}
		else if (!ProcessIsRunning(pOverlay->process))
		{
			//Check if the program closed without going through the overlay
			CloseOverlay(pOverlay, false);
			delete pOverlay->process; pOverlay->process = nullptr;
			pOverlay->showing = false;
		}
	}
}

static void RenderOverlay(YaffeState* pState, RenderState* pRender)
{
	PlatformWindow* overlay = pState->overlay.form;
	if (pState->overlay.showing)
	{
		//Render to overlay window
		wglMakeCurrent(overlay->dc, pState->form->rc);

		v2 size = V2(overlay->width, overlay->height);
		BeginRenderPass(size, pRender);
		glClearColor(1, 1, 1, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		PushQuad(pRender, V2(0), size, OVERLAY_COLOR);

		ModalWindow modal = {};
		modal.title = "Confirm";
		modal.icon = BITMAP_Question;
		modal.content = new StringModal("Are you sure you wish to exit?");

		RenderModalWindow(pRender, &modal, overlay);
		delete modal.content;

		char buffer[20];
		GetTime(buffer, 20);
		v2 position = V2(overlay->width, overlay->height) - V2(UI_MARGIN, GetFontSize(FONT_Title) + UI_MARGIN);
		PushRightAlignedTextWithIcon(pRender, &position, BITMAP_None, 0, FONT_Title, buffer, TEXT_FOCUSED);

		EndRenderPass(size, pRender);
		SwapBuffers(overlay);

		//Switch back to normal window
		wglMakeCurrent(pState->form->dc, pState->form->rc);
	}
}