class EmulatorList : public UiElement
{
	YaffeState* state;
	List<Emulator>* emulators;

	void Render(RenderState* pRender)
	{
		v4 text_color = IsFocused() ? TEXT_FOCUSED : TEXT_UNFOCUSED;
		v4 foreground = IsFocused() ? ACCENT_COLOR : ACCENT_UNFOCUSED;

		v2 size = this->GetAbsoluteSize();
		PushQuad(pRender, position, size, MENU_BACKGROUND);

		v2 text_size = MeasureString(FONT_Normal, emulators->GetItem(state->selected_emulator)->display_name);
		for (u32 i = 0; i < emulators->count; i++)
		{
			char* item = emulators->GetItem(i)->display_name;
			v2 item_size = V2(size.Width, text_size.Height + UI_MARGIN * 2);
			v2 item_position = { position.X, position.Y + (i * item_size.Height) };
			if (i == state->selected_emulator)
			{
				PushQuad(pRender, item_position, item_position + item_size, foreground);
			}

			PushText(pRender, FONT_Normal, (char*)item, item_position + CenterText(FONT_Normal, item, item_size), text_color);
		}
	}

	void Update(float pDeltaTime)
	{
		if (IsUpPressed())
		{
			--state->selected_emulator;
			if (state->selected_emulator < 0) state->selected_emulator = emulators->count - 1;
			GetRoms(state, emulators->GetItem(state->selected_emulator));
		}
		else if (IsDownPressed())
		{
			state->selected_emulator = (state->selected_emulator + 1) % emulators->count;
			GetRoms(state, emulators->GetItem(state->selected_emulator));
		}

		if (IsEnterPressed())
		{
			FocusElement(UI_Roms);
		}
	}

public:
	EmulatorList(YaffeState* pState) : UiElement(V2(EMU_MENU_PERCENT, 1),  UI_Emulators)
	{
		state = pState;
		emulators = &pState->emulators;
	}
};