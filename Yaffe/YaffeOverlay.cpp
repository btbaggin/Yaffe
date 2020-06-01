static void UpdateOverlay(Overlay* pOverlay, float pDeltaTime)
{
	if (pOverlay->process)
	{
		if (pOverlay->allow_input)
		{
			INPUT buffer = {};
			buffer.type = INPUT_MOUSE;

			v2 dir = GetLeftStickVector();
			v2 mouse = GetMousePosition();

			//determine how far the controller is pushed
			float magnitude = HMM_Length(dir);
			float normalizedMagnitude = 0;
			//check if the controller is outside a circular dead zone
			if (magnitude > XINPUT_INPUT_DEADZONE)
			{
				if (magnitude > 32767) magnitude = 32767;
				magnitude -= XINPUT_INPUT_DEADZONE;

				//optionally normalize the magnitude with respect to its expected range
				//giving a magnitude value of 0.0 to 1.0
				normalizedMagnitude = magnitude / (32767 - XINPUT_INPUT_DEADZONE);
				dir = dir / (normalizedMagnitude * 250);

				//determine the direction the controller is pushed

				MONITORINFO mi = { sizeof(mi) };
				GetMonitorInfo(MonitorFromWindow(pOverlay->form->handle, MONITOR_DEFAULTTONEAREST), &mi);
				u32 width = mi.rcMonitor.right - mi.rcMonitor.left;
				u32 height = mi.rcMonitor.bottom - mi.rcMonitor.top;

				buffer.mi.dx = ((mouse.X + dir.X) * (0xFFFF / (width + 1)));
				buffer.mi.dy = ((mouse.Y + dir.Y) * (0xFFFF / (height + 1)));
				buffer.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE);

				SendInput(1, &buffer, sizeof(INPUT));
			}



			if (IsControllerPressed(CONTROLLER_A))
			{
				buffer.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN);
				SendInput(1, &buffer, sizeof(INPUT));
			}
			if (IsControllerReleased(CONTROLLER_A))
			{
				buffer.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP);
				SendInput(1, &buffer, sizeof(INPUT));
			}
			
			if (IsControllerPressed(CONTROLLER_X))
			{
				buffer.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN);
				SendInput(1, &buffer, sizeof(INPUT));
			}
			if (IsControllerReleased(CONTROLLER_X))
			{
				buffer.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP);
				SendInput(1, &buffer, sizeof(INPUT));
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