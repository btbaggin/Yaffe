class OverlayModal : public ModalContent
{
	float volume;
	MODAL_RESULTS Update(float pDeltaTime)
	{
		CoInitialize(NULL);
		IMMDeviceEnumerator *deviceEnumerator = NULL;
		HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
		IMMDevice *defaultDevice = NULL;

		hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
		deviceEnumerator->Release();
		deviceEnumerator = NULL;

		IAudioEndpointVolume *endpointVolume = NULL;
		hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&endpointVolume);
		defaultDevice->Release();
		defaultDevice = NULL;

		hr = endpointVolume->GetMasterVolumeLevelScalar(&volume);

		if (IsLeftPressed())
		{
			volume = max(0.0F, volume - 0.05F);
			hr = endpointVolume->SetMasterVolumeLevelScalar(volume, NULL);
		}
		else if (IsRightPressed())
		{
			volume = min(1.0F, volume + 0.05F);
			hr = endpointVolume->SetMasterVolumeLevelScalar(volume, NULL);
		}

		endpointVolume->Release();
		CoUninitialize();

		return ModalContent::Update(pDeltaTime);
	}

	void Render(RenderState* pState, v2 pPosition)
	{
		float x = 24 + UI_MARGIN;
		PushSizedQuad(pState, pPosition, V2(24), GetBitmap(g_assets, BITMAP_Speaker));
		PushSizedQuad(pState, pPosition + V2(x, 0), V2(720 - x, 24), ELEMENT_BACKGROUND);
		PushSizedQuad(pState, pPosition + V2(x, 0), V2((720 - x) * volume, 24), ACCENT_COLOR);
	}

public:
	OverlayModal()
	{
		size = V2(720, 24);
	}
};

static void DisposeOverlay(Overlay* pOverlay)
{
	delete pOverlay->process; pOverlay->process = nullptr;
	pOverlay->showing = false;
	delete pOverlay->modal->content;
	delete pOverlay->modal;
}

static void UpdateOverlay(Overlay* pOverlay, float pDeltaTime)
{
	if (pOverlay->process)
	{
		if (IsControllerPressed(CONTROLLER_GUIDE) || IsEscPressed())
		{
			pOverlay->showing = !pOverlay->showing;

			if (pOverlay->showing)
			{
				ShowOverlay(pOverlay);

				if (pOverlay->modal)
				{
					delete pOverlay->modal->content;
					delete pOverlay->modal;
				}

				pOverlay->modal = new ModalWindow();
				pOverlay->modal->title = "Yaffe";
				pOverlay->modal->icon = BITMAP_None;
				pOverlay->modal->button = "Quit Application";
				pOverlay->modal->content = new OverlayModal();
			}
			else
			{
				CloseOverlay(pOverlay, false);
			}
		}
		else if (!pOverlay->allow_input && !ProcessIsRunning(pOverlay->process))
		{
			//Check if the program closed without going through the overlay
			CloseOverlay(pOverlay, false);
			DisposeOverlay(pOverlay);
		}

		if (pOverlay->showing)
		{
			MODAL_RESULTS result = pOverlay->modal->content->Update(pDeltaTime);
			if (result == MODAL_RESULT_Ok)
			{
				CloseOverlay(pOverlay, true);
				DisposeOverlay(pOverlay);
			}
		}
		

		if (pOverlay->allow_input && !pOverlay->showing)
		{
			v2 mouse = GetMousePosition();
			v2 move = NormalizeStickInput(g_input.left_stick);
			SetCursor(mouse + (move * 400 * pDeltaTime));

			v2 scroll = NormalizeStickInput(g_input.right_stick);
			INPUT buffer = {};
			buffer.type = INPUT_MOUSE;
			buffer.mi.dwFlags = MOUSEEVENTF_WHEEL;
			buffer.mi.mouseData = (DWORD)(-scroll.Y * WHEEL_DELTA);
			SendInput(1, &buffer, sizeof(INPUT));

			struct KeyMapping { CONTROLLER_BUTTONS cont; KEYS input; bool do_shift; };
			struct MouseMapping { CONTROLLER_BUTTONS cont; MOUSE_BUTTONS input; };
			KeyMapping k_mappings[3] = {
				{CONTROLLER_RIGHT_SHOULDER, KEY_Tab},
				{CONTROLLER_LEFT_SHOULDER, KEY_Tab, true},
				{CONTROLLER_START, KEY_Enter},
			};
			MouseMapping m_mappings[2] = {
				{CONTROLLER_A, BUTTON_Left},
				{CONTROLLER_X, BUTTON_Right},
			};
			
			for (u32 i = 0; i < ArrayCount(k_mappings); i++)
			{
				KeyMapping key = k_mappings[i];
				if (IsControllerPressed(key.cont))
				{
					if (key.do_shift) SendKeyMessage(KEY_Shift, true);
					SendKeyMessage(key.input, true);
				}
				else if (IsControllerReleased(key.cont))
				{
					if (key.do_shift) SendKeyMessage(KEY_Shift, false);
					SendKeyMessage(key.input, false);
				}
			}
			for (u32 i = 0; i < ArrayCount(m_mappings); i++)
			{
				MouseMapping key = m_mappings[i];
				if (IsControllerPressed(key.cont)) SendMouseMessage(key.input, true);
				else if (IsControllerReleased(key.cont)) SendMouseMessage(key.input, false);
			}
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
		RenderModalWindow(pRender, pState->overlay.modal, overlay);

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
