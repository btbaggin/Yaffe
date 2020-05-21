class InfoPane : public UiControl
{
public:
	std::string overview;
	v2 position;

	InfoPane() : UiControl(UI_Info) 
	{ 
		position = V2(g_state.form->width, 0.0F);
	}

	void Render(RenderState* pState, UiRegion pRegion)
	{
		const float PADDING = 20.0F;
		Executable* rom = GetSelectedExecutable();
		if (rom && position.X < pRegion.size.Width)
		{
			float width = pRegion.size.Width * INFO_PANE_WIDTH;
			PushSizedQuad(pState, position, V2(width, pRegion.size.Height), MODAL_BACKGROUND);
			
			//Calculate correct height based on bitmap size
			float height = 0;
			Bitmap* b = GetBitmap(g_assets, rom->banner);
			if (b)
			{
				//Banner
				height = (width / b->width) * pRegion.size.Height;
				PushQuad(pState, position, V2(position.X + width, height), b);
			}
			//Accent bar
			PushSizedQuad(pState, position + V2(0, height), V2(UI_MARGIN, pRegion.size.Height - height), ACCENT_COLOR);

			//Description
			if (!overview.empty())
			{
				PushText(pState, FONT_Normal, overview.c_str(), position + V2(PADDING, height + PADDING), TEXT_FOCUSED);
			}
		}
	}

	void Update(YaffeState* pState, float pDeltaTime) 
	{ 
		position = Lerp(position, pDeltaTime * ANIMATION_SPEED, V2(pState->form->width * (1 - INFO_PANE_WIDTH), 0));
		if (IsEscPressed())
		{
			RevertFocus();
		}
	}

	void UnfocusedUpdate(YaffeState* pState, float pDeltaTime)
	{
		position = Lerp(position, pDeltaTime * ANIMATION_SPEED, V2(pState->form->width, 0));
	}

	void OnFocusGained()
	{
		Executable* rom = GetSelectedExecutable();
		Platform* emu = GetSelectedPlatform();
		overview = rom->overview;
		if (overview.empty()) overview = "No information available";

		WrapText((char*)overview.c_str(), (u32)overview.size(), FONT_Normal, g_state.form->width * INFO_PANE_WIDTH - 20 * 2);
	}
};