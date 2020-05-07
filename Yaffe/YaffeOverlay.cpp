static void UpdateOverlay(Overlay* pOverlay)
{
	if (pOverlay->process)
	{
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

static void RenderOverlayModal(RenderState* pState, const char* pMessage)
{
	v2 content_size = MeasureString(FONT_Normal, pMessage);

	const float ICON_SIZE = 32.0F;
	const float ICON_SIZE_WITH_MARGIN = ICON_SIZE + UI_MARGIN * 2;
	v2 size = V2(UI_MARGIN * 4, UI_MARGIN * 2) + content_size;
	size.Height = max(ICON_SIZE_WITH_MARGIN, size.Height);
	size.Width += ICON_SIZE_WITH_MARGIN;

	v2 window_position = V2((g_state.overlay.form->width - size.Width) / 2, (g_state.overlay.form->height - size.Height) / 2);
	v2 icon_position = window_position + V2(UI_MARGIN * 2, UI_MARGIN); //Window + margin for window + margin for icon

	PushQuad(pState, window_position, window_position + size, MODAL_BACKGROUND);
	PushSizedQuad(pState, window_position, V2(UI_MARGIN / 2.0F, size.Height), ACCENT_COLOR);

	Bitmap* image = GetBitmap(g_assets, BITMAP_Question);
	PushSizedQuad(pState, icon_position, V2(ICON_SIZE), image);
	icon_position.X += ICON_SIZE;

	PushText(pState, FONT_Normal, pMessage, icon_position, TEXT_FOCUSED);
}

static void RenderOverlay(YaffeState* pState, RenderState* pRender)
{
	if (pState->overlay.showing)
	{
		//Render to overlay window
		wglMakeCurrent(pState->overlay.form->dc, pState->form->rc);

		float width = (float)pState->overlay.form->width;
		float height = (float)pState->overlay.form->height;
		v2 size = V2(width, height);
		BeginRenderPass(size, pRender);
		glClearColor(1, 1, 1, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		PushQuad(pRender, V2(0.0F), V2(width, height), V4(0.0F, 0.0F, 0.0F, 0.9F));

		RenderOverlayModal(pRender, "Are you sure you wish to exit?");

		EndRenderPass(size, pRender);
		SwapBuffers(g_state.overlay.form);

		//Switch back to normal window
		wglMakeCurrent(pState->form->dc, pState->form->rc);
	}
}