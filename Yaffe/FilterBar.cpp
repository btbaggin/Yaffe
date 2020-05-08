class FilterBar : public UiControl
{
	enum FILTER_MODES
	{
		FILTER_MODE_None,
		FILTER_MODE_Name,
		FILTER_MODE_Players,

		FILTER_MODE_COUNT,
	};

public:
	FilterBar() : UiControl(UI_Search)
	{ 
		mode = FILTER_MODE_None;
		selected_index = -1;
		position = V2(g_state.form->width * EMU_MENU_PERCENT, -100);
	}

	s8 mode;
	bool exists[26];
	char selected_index;
	char start;
	bool is_active;
	v2 position;
	static void Render(RenderState* pState, UiRegion pRegion, FilterBar* pBar)
	{
		const float SELECTOR_PERCENT = 0.1F;
		const float ARROW_HEIGHT = 10;
		const float ARROW_WIDTH = 30;
		v2 bar_start = pBar->position; 
		v2 bar_end = bar_start + pRegion.size;
		if (bar_end.Y > 0)
		{
			PushQuad(pState, bar_start, bar_end, MENU_BACKGROUND);

			float filter_name_width = pRegion.size.Width * SELECTOR_PERCENT;
			v2 filter_start = bar_start + V2(filter_name_width, 0);

			char end = 0;
			const char* name = nullptr;
			switch (pBar->mode)
			{
				case FILTER_MODE_None:
					name = "None";
					break;

				case FILTER_MODE_Name:
					name = "Name";
					pBar->start = 'A';
					end = 'Z';
					break;

				case FILTER_MODE_Players:
					name = "Players";
					pBar->start = '1';
					end = '4';
					break;

				default:
					assert(false);
			}
			float item_space = (bar_end.X - filter_start.X) / (end - pBar->start + 1);

			//Filter name
			if (pBar->IsFocused() && pBar->selected_index < 0)
			{
				PushQuad(pState, bar_start, V2(filter_start.X, bar_end.Y), ACCENT_COLOR);
			}
			v4 font_color = GetFontColor(pBar->IsFocused());
			v2 filter_name_position = CenterText(FONT_Normal, name, V2(filter_start.X, bar_end.Y) - bar_start);
			PushText(pState, FONT_Normal, name, bar_start + filter_name_position, font_color);

			if (pBar->mode > 0)
			{
				//Up arrow
				v2 arrow_start = bar_start + V2(filter_name_width / 2 - ARROW_WIDTH / 2, 0);
				PushSizedQuad(pState, arrow_start, V2(ARROW_WIDTH, ARROW_HEIGHT), font_color, GetBitmap(g_assets, BITMAP_ArrowUp));
			}
			if (pBar->mode < FILTER_MODE_COUNT - 1)
			{
				//Down arrow
				v2 arrow_start = bar_start + V2(filter_name_width / 2 - ARROW_WIDTH / 2, pRegion.size.Height - ARROW_HEIGHT);
				PushSizedQuad(pState, arrow_start, V2(ARROW_WIDTH, ARROW_HEIGHT), font_color, GetBitmap(g_assets, BITMAP_ArrowDown));
			}

			//Filter items
			for (char i = pBar->start; i <= end; i++)
			{
				v2 position = V2(filter_start.X + item_space * (i - pBar->start), bar_start.Y);
				if (pBar->selected_index + pBar->start == i && pBar->is_active)
				{
					PushSizedQuad(pState, position, V2(item_space, bar_end.Y), ACCENT_COLOR);
				}

				char text[2]; text[0] = i; text[1] = '\0';
				v4 color = GetFontColor(pBar->IsFocused() && pBar->exists[i - pBar->start]);

				PushText(pState, FONT_Normal, text, position + CenterText(FONT_Normal, text, V2(item_space, pRegion.size.Height)), color);
			}
		}
	}

	void Update(float pDeltaTime) 
	{
		if (IsFocused())
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

			position = Lerp(position, pDeltaTime * ANIMATION_SPEED, V2(g_state.form->width * EMU_MENU_PERCENT, 0));
		}
		else if(!is_active)
		{
			position = Lerp(position, pDeltaTime * ANIMATION_SPEED, V2(g_state.form->width * EMU_MENU_PERCENT, -100));
		}
	}

	void ResetRoms()
	{
		List<Executable> roms = GetSelectedApplication()->files;
		for (u32 i = 0; i < roms.count; i++)
		{
			Executable* rom = roms.GetItem(i);
			rom->flags = EXECUTABLE_FLAG_None;
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
		List<Executable> roms = GetSelectedApplication()->files;

		for (u32 i = 0; i < roms.count; i++)
		{
			Executable* rom = roms.GetItem(i);
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
				rom->flags = EXECUTABLE_FLAG_None;
				if (!index_set)
				{
					g_state.selected_rom = i;
					index_set = true;
				}
			}
			else
			{
				rom->flags |= EXECUTABLE_FLAG_Filtered;
			}
		}
	}

	void SetExists()
	{
		//Set if a particular filter item exists for our set of roms
		for (u32 i = 0; i < ArrayCount(exists); i++) exists[i] = false;
		List<Executable> roms = GetSelectedApplication()->files;
		for (u32 i = 0; i < roms.count; i++)
		{
			Executable* rom = roms.GetItem(i);
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
		is_active = true;
	}
};