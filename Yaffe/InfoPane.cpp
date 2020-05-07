class InfoPane : public UiControl
{
public:
	std::string overview;
	v2 position;

	InfoPane() : UiControl(UI_Info) 
	{ 
		position = V2((float)g_state.form->width, 0.0F);
	}

	static void Render(RenderState* pState, UiRegion pRegion, InfoPane* pPane)
	{
		const float PADDING = 20.0F;
		Executable* rom = GetSelectedExecutable();
		if (rom && pPane->position.X < pRegion.size.Width)
		{
			float width = pRegion.size.Width * INFO_PANE_WIDTH;
			PushSizedQuad(pState, pPane->position, V2(width, pRegion.size.Height), MODAL_BACKGROUND);
			
			//Calculate correct height based on bitmap size
			float height = 0;
			Bitmap* b = GetBitmap(g_assets, &rom->banner);
			if (b)
			{
				//Banner
				height = (width / b->width) * pRegion.size.Height;
				PushQuad(pState, pPane->position, V2(pPane->position.X + width, height), b);
			}
			//Accent bar
			PushSizedQuad(pState, pPane->position, V2(UI_MARGIN, pRegion.size.Height), ACCENT_COLOR);

			//Description
			if (!pPane->overview.empty())
			{
				PushText(pState, FONT_Normal, pPane->overview.c_str(), pPane->position + V2(PADDING, height + PADDING), TEXT_FOCUSED);
			}
		}
	}

	void Update(float pDeltaTime) 
	{ 
		if (IsFocused())
		{
			position = Lerp(position, pDeltaTime * 5, V2(g_state.form->width * (1 - INFO_PANE_WIDTH), 0));
			if (IsEscPressed())
			{
				RevertFocus();
			}
		}
		else
		{
			position = Lerp(position, pDeltaTime * 5, V2((float)g_state.form->width, 0));
		}
	}

	void OnFocusGained()
	{
		Executable* rom = GetSelectedExecutable();
		Application* emu = GetSelectedApplication();
		overview = GetGameInformation(rom->platform, rom->name);

		WrapText((char*)overview.c_str(), overview.size(), FONT_Normal, g_state.form->width * INFO_PANE_WIDTH - 20 * 2);
	}
};