const float VOLUME_DELTA = 0.05F;
const float CURSOR_SPEED = 400.0F;

class OverlayModal : public ModalContent
{
	const float ICON_SIZE = 24.0F;
	float volume;
	MODAL_RESULTS Update(float pDeltaTime)
	{
		float delta = 0.0F;
		if (IsLeftPressed()) delta = -VOLUME_DELTA;
		else if (IsRightPressed()) delta = VOLUME_DELTA;

		GetAndSetVolume(&volume, delta);

		return ModalContent::Update(pDeltaTime);
	}

	void Render(RenderState* pState, v2 pPosition)
	{
		float x = ICON_SIZE + UI_MARGIN;
		PushSizedQuad(pState, pPosition, V2(ICON_SIZE), GetBitmap(g_assets, BITMAP_Speaker));
		PushSizedQuad(pState, pPosition + V2(x, 0), V2(size.Width - x, ICON_SIZE), ELEMENT_BACKGROUND);
		PushSizedQuad(pState, pPosition + V2(x, 0), V2((size.Width - x) * volume, ICON_SIZE), ACCENT_COLOR);
	}

public:
	OverlayModal()
	{
		SetSize(MODAL_SIZE_Half, 1);
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
		if (IsControllerPressed(CONTROLLER_GUIDE) || IsKeyPressed(KEY_Escape))
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
		else if (!pOverlay->allow_input && !ProcessIsRunning(pOverlay->process)) //Kind of a hack, but I can't get it to report browsers running correctly
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
		
		//Allow us to control the mouse with XInput
		if (pOverlay->allow_input && !pOverlay->showing)
		{
			PlatformInputMessage message;
			
			v2 mouse = GetMousePosition();
			v2 move = NormalizeStickInput(g_input.left_stick);

			message.cursor = mouse + (move * CURSOR_SPEED * pDeltaTime);
			SendInputMessage(INPUT_ACTION_Cursor, &message);

			v2 scroll = NormalizeStickInput(g_input.right_stick);
			message.scroll = scroll.Y;
			SendInputMessage(INPUT_ACTION_Scroll, &message);

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
					message.down = true;
					if (key.do_shift)
					{
						message.key = KEY_Shift;
						SendInputMessage(INPUT_ACTION_Key, &message);
					}
					message.key = key.input;
					SendInputMessage(INPUT_ACTION_Key, &message);
				}
				else if (IsControllerReleased(key.cont))
				{
					message.down = false;
					if (key.do_shift)
					{
						message.key = KEY_Shift;
						SendInputMessage(INPUT_ACTION_Key, &message);
					}
					message.key = key.input;
					SendInputMessage(INPUT_ACTION_Key, &message);
				}
			}

			for (u32 i = 0; i < ArrayCount(m_mappings); i++)
			{
				MouseMapping key = m_mappings[i];
				if (IsControllerPressed(key.cont))
				{
					message.down = true;
					message.button = key.input;
					SendInputMessage(INPUT_ACTION_Mouse, &message);
				}
				else if (IsControllerReleased(key.cont))
				{
					message.down = false;
					message.button = key.input;
					SendInputMessage(INPUT_ACTION_Mouse, &message);
				}
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
