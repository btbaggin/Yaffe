class FilterBar : public UiElement
{
	enum FILTER_MODES
	{
		FILTER_MODE_None,
		FILTER_MODE_Name,
		FILTER_MODE_Players,

		FILTER_MODE_COUNT,
	};

public:
	FilterBar() : UiElement(V2(1, 0.5), UI_Search)
	{ 
		mode = FILTER_MODE_None;
		selected_index = -1;
		position = V2(g_state.form.width * EMU_MENU_PERCENT, -100);
	}

private:
	s8 mode;
	bool exists[26];
	char selected_index;
	char start;
	bool is_active;
	void Render(RenderState* pState)
	{
		const float SELECTOR_PERCENT = 0.1F;
		const float ARROW_HEIGHT = 10;
		const float ARROW_WIDTH = 30;
		float bar_height = GetFontSize(FONT_Normal) + ARROW_HEIGHT * 2;
		v2 bar_start = position; 
		v2 bar_end = bar_start + V2(g_state.form.width * (1 - EMU_MENU_PERCENT), bar_height);
		PushQuad(pState, bar_start, bar_end, MENU_BACKGROUND);

		float filter_name_width = g_state.form.width * SELECTOR_PERCENT;
		v2 filter_start = bar_start + V2(filter_name_width, 0);

		char end = 0;
		const char* name = nullptr;
		switch (mode)
		{
			case FILTER_MODE_None:
				name = "None";
				break;

			case FILTER_MODE_Name:
				name = "Name";
				start = 'A';
				end = 'Z';
				break;

			case FILTER_MODE_Players:
				name = "Players";
				start = '1';
				end = '4';
				break;

			default:
				assert(false);
		}
		float item_space = (bar_end.X - filter_start.X) / (end - start + 1);

		//Filter name
		if (IsFocused() && selected_index < 0)
		{
			PushQuad(pState, bar_start, V2(filter_start.X, bar_end.Y), ACCENT_COLOR);
		}
		v4 font_color = IsFocused() ? TEXT_FOCUSED : TEXT_UNFOCUSED;
		v2 filter_name_position = CenterText(FONT_Normal, name, V2(filter_start.X, bar_end.Y) - bar_start);
		PushText(pState, FONT_Normal, name, bar_start + filter_name_position, font_color);
		if (mode > 0)
		{
			v2 arrow_start = bar_start + V2(filter_name_width / 2 - ARROW_WIDTH / 2, 0);
			PushQuad(pState, arrow_start, arrow_start + V2(ARROW_WIDTH, ARROW_HEIGHT), font_color, GetBitmap(g_assets, BITMAP_ArrowUp));
		}
		if (mode < FILTER_MODE_COUNT - 1)
		{
			v2 arrow_start = bar_start + V2(filter_name_width / 2 - ARROW_WIDTH / 2, bar_height - ARROW_HEIGHT);
			PushQuad(pState, arrow_start, arrow_start + V2(ARROW_WIDTH, ARROW_HEIGHT), font_color, GetBitmap(g_assets, BITMAP_ArrowDown));
		}

		//Filter items
		for (char i = start; i <= end; i++)
		{
			v2 position = V2(filter_start.X + item_space * (i - start), bar_start.Y);
			if (selected_index + start == i && (IsFocused() || is_active))
			{
				PushQuad(pState, position, position + V2(item_space, bar_end.Y), ACCENT_COLOR);
			}

			char text[2]; text[0] = i; text[1] = '\0';
			v4 color = IsFocused() && exists[i - start] ? TEXT_FOCUSED : TEXT_UNFOCUSED;

			PushText(pState, FONT_Normal, text, position + CenterText(FONT_Normal, text, V2(item_space, bar_height)), color);
		}
	}

	void Update(float pDeltaTime) 
	{
		//Cycle the index until we get to one that exists, or the filter bar
		if (IsLeftPressed())
		{
			if (selected_index < 0) selected_index = ArrayCount(exists) - 1;
			while (!exists[--selected_index] && selected_index != -1);
			FilterRoms();
		}
		else if (IsRightPressed())
		{
			while (!exists[++selected_index] && selected_index != -1)
			{
				if (selected_index == ArrayCount(exists) - 1) selected_index = -2;
			}
			FilterRoms();
		}
		else if (IsEnterPressed())
		{
			is_active = true;
			FocusElement(UI_Roms);
		}
		else if (IsEscPressed())
		{
			is_active = false;
			RevertFocus();
			ResetRoms();
		}

		//selected_index == -1, means we are currently on the filter name
		if (selected_index < 0)
		{
			if (IsDownPressed())
			{
				mode = min(mode + 1, FILTER_MODE_COUNT - 1);
				SetExists();
			}
			else if (IsUpPressed())
			{
				mode = max(0, mode - 1);
				SetExists();
			}
		}

		if (IsFocused())
		{
			position = Lerp(position, pDeltaTime * 6, V2(g_state.form.width * EMU_MENU_PERCENT, 0));
		}
	}

	void OnEmulatorChanged(Emulator* pEmulator)
	{
		ResetRoms();
		is_active = false;
		selected_index = -1;
		UiElement::OnEmulatorChanged(pEmulator);
	}

	void ResetRoms()
	{
		List<Rom> roms = GetSelectedEmulator()->roms;
		for (u32 i = 0; i < roms.count; i++)
		{
			Rom* rom = roms.GetItem(i);
			rom->flags = ROM_DISPLAY_None;
		}
	}

	void FilterRoms()
	{
		//Don't do any filtering when moving to the filter bar
		if (selected_index < 0)
		{
			ResetRoms();
			return;
		}

		//Hide roms that don't match our particular filter item
		bool index_set = false;
		char c = selected_index + start;
		List<Rom> roms = GetSelectedEmulator()->roms;

		for (u32 i = 0; i < roms.count; i++)
		{
			Rom* rom = roms.GetItem(i);
			bool filter = true;
			switch (mode)
			{
				case FILTER_MODE_Name:
					filter = (toupper(rom->name[0]) == c);
					break;

				case FILTER_MODE_Players:
					filter = rom->players >= (u8)selected_index + 1;
					break;
			}

			if (filter)
			{
				rom->flags = ROM_DISPLAY_None;
				if (!index_set)
				{
					g_state.selected_rom = i;
					index_set = true;
				}
			}
			else
			{
				rom->flags |= ROM_DISPLAY_Filtered;
			}
		}
	}

	void SetExists()
	{
		//Set if a particular filter item exists for our set of roms
		for (u32 i = 0; i < ArrayCount(exists); i++) exists[i] = false;
		List<Rom> roms = GetSelectedEmulator()->roms;
		for (u32 i = 0; i < roms.count; i++)
		{
			Rom* rom = roms.GetItem(i);
			s32 index = 0;
			switch (mode)
			{
			case FILTER_MODE_Name:
				index = toupper(rom->name[0]) - 'A';
				break;
			case FILTER_MODE_Players:
				index = rom->players - 1;
				break;
			}
			assert(index < ArrayCount(exists));
			exists[index] = true;
		}
	}

	void OnFocusGained()
	{
		SetExists();
	}

	void OnFocusLost()
	{
		if (!is_active)
		{
			position = V2(g_state.form.width * EMU_MENU_PERCENT, -100);
		}
	}
};