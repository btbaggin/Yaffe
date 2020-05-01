class InfoPane : public UiControl
{
public:
	std::string overview;
	v2 position;

	InfoPane() : UiControl(UI_Info) 
	{ 
		position = V2((float)g_state.form.width, 0.0F);
	}

	static void Render(RenderState* pState, UiRegion pRegion, InfoPane* pPane)
	{
		const float PADDING = 20.0F;
		float height = 0.15F * pRegion.size.Height;
		Rom* rom = GetSelectedRom();
		if (rom)
		{
			PushQuad(pState, pPane->position, pPane->position + pRegion.size, MODAL_BACKGROUND);
			PushQuad(pState, pPane->position, V2(pPane->position.X + pRegion.size.Width, height), GetBitmap(g_assets, &rom->banner));

			if (!pPane->overview.empty())
			{
				PushText(pState, FONT_Normal, pPane->overview.c_str(), pPane->position + V2(PADDING, height + PADDING), V4(1), pRegion.size.Width - PADDING * 2);
			}
		}
	}

	void Update(float pDeltaTime) 
	{ 
		if (IsFocused())
		{
			position = Lerp(position, pDeltaTime * 5, V2(g_state.form.width * (1 - INFO_PANE_WIDTH), 0));
			if (IsEscPressed())
			{
				RevertFocus();
			}
		}
		else
		{
			position = Lerp(position, pDeltaTime * 5, V2((float)g_state.form.width, 0));
		}
	}

	void OnFocusGained()
	{
		Rom* rom = GetSelectedRom();
		Application* emu = GetSelectedEmulator();
		overview = GetGameInformation(emu->platform, rom->name);
	}
};