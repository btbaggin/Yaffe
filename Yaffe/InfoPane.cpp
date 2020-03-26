class InfoPane : public UiElement
{
	#define INFO_PANE_WIDTH 0.5F
public:
	InfoPane(EmulatorList* pList, RomMenu* pRoms) : UiElement(V2(INFO_PANE_WIDTH, 1), UI_Info)
	{
		emus = pList;
		roms = pRoms;
		position = V2((float)g_state.form.width, 0.0F);
	}

private:
	EmulatorList* emus;
	RomMenu* roms;
	bool last_focused;

	void Render(RenderState* pState)
	{
		if (IsFocused())
		{
			const float PADDING = 20.0F;
			float height = 0.15F * g_state.form.height;
			v2 size = GetAbsoluteSize();
			Rom* rom = GetSelectedRom();
			PushQuad(pState, position, position + size, MODAL_BACKGROUND);
			PushQuad(pState, position, V2(position.X + size.Width, height), GetBitmap(g_assets, &rom->banner));

			Emulator* emu = GetSelectedEmulator();
			std::string overview = GetGameInformation(emu->platform, rom->name);
			if (!overview.empty())
			{
				PushText(pState, FONT_Normal, overview.c_str(), position + V2(PADDING, height + PADDING), V4(1), size.Width - PADDING * 2);
			}
		}	
	}

	void Update(float pDeltaTime) 
	{ 
		position = Lerp(position, pDeltaTime * 5, V2(g_state.form.width * (1 - INFO_PANE_WIDTH), 0));
		if (IsEscPressed())
		{
			RevertFocus();
		}
	}

	void OnFocusLost()
	{
		position = V2((float)g_state.form.width, 0.0F);
	}
};