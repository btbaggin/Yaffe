class EmulatorList : public UiElement
{
public:
	List<Emulator>* emulators;

	static void Render(RenderState* pRender, UiRegion pRegion, EmulatorList* pList)
	{
		v4 foreground = pList->IsFocused() ? ACCENT_COLOR : ACCENT_UNFOCUSED;
		v4 font = GetFontColor(pList->IsFocused());

		PushQuad(pRender, pRegion.position, pRegion.size, MENU_BACKGROUND);

		float text_size = GetFontSize(FONT_Normal);
		for (u32 i = 0; i < pList->emulators->count; i++)
		{
			char* item = pList->emulators->GetItem(i)->display_name;
			v2 item_size = V2(pRegion.size.Width, text_size + UI_MARGIN * 2);
			v2 item_position = { pRegion.position.X, pRegion.position.Y + (i * item_size.Height) };
			if (i == g_state.selected_emulator)
			{
				PushQuad(pRender, item_position, item_position + item_size, foreground);
			}

			PushText(pRender, FONT_Normal, (char*)item, item_position + CenterText(FONT_Normal, item, item_size), font);
		}
	}

	void Update(float pDeltaTime)
	{
		if (IsFocused())
		{
			if (IsUpPressed())
			{
				--g_state.selected_emulator;
				if (g_state.selected_emulator < 0) g_state.selected_emulator = emulators->count - 1;
				GetRoms(&g_state, emulators->GetItem(g_state.selected_emulator));
			}
			else if (IsDownPressed())
			{
				g_state.selected_emulator = (g_state.selected_emulator + 1) % emulators->count;
				GetRoms(&g_state, emulators->GetItem(g_state.selected_emulator));
			}

			if (IsEnterPressed())
			{
				FocusElement(UI_Roms);
			}
		}
	}

	EmulatorList(YaffeState* pState) : UiElement(UI_Emulators)
	{
		emulators = &pState->emulators;
	}
};