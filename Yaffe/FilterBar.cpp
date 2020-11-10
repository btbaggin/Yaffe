class FilterBar : public Widget
{
	enum FILTER_MODES
	{
		FILTER_MODE_None,
		FILTER_MODE_Name,
		FILTER_MODE_Players,

		FILTER_MODE_COUNT,
	};

public:
	FilterBar(Interface* pInterface) : Widget(UI_Search, pInterface)
	{ 
		mode = FILTER_MODE_None;
		selected_index = -1;
	}

	void OnAdded()
	{
		SetPosition(RelativeToAbsolute(0, -0.05F));
		SetSize(1, 0.05F);
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
		v2 bar_start = GetPosition(); 
		v2 bar_end = bar_start + size;

		PushQuad(pState, bar_start, bar_end, MENU_BACKGROUND);

		float filter_name_width = size.Width * SELECTOR_PERCENT;
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
		v4 font_color = GetFontColor(IsFocused());
		v2 filter_name_position = CenterText(FONT_Normal, name, V2(filter_start.X, bar_end.Y) - bar_start);
		PushText(pState, FONT_Normal, name, bar_start + filter_name_position, font_color);

		if (mode > 0)
		{
			//Up arrow
			v2 arrow_start = bar_start + V2(filter_name_width / 2 - ARROW_WIDTH / 2, 0);
			PushSizedQuad(pState, arrow_start, V2(ARROW_WIDTH, ARROW_HEIGHT), font_color, GetBitmap(g_assets, BITMAP_ArrowUp));
		}
		if (mode < FILTER_MODE_COUNT - 1)
		{
			//Down arrow
			v2 arrow_start = bar_start + V2(filter_name_width / 2 - ARROW_WIDTH / 2, size.Height - ARROW_HEIGHT);
			PushSizedQuad(pState, arrow_start, V2(ARROW_WIDTH, ARROW_HEIGHT), font_color, GetBitmap(g_assets, BITMAP_ArrowDown));
		}

		//Filter items
		for (char i = start; i <= end; i++)
		{
			v2 position = V2(filter_start.X + item_space * (i - start), bar_start.Y);
			if (selected_index + start == i && is_active)
			{
				PushSizedQuad(pState, position, V2(item_space, bar_end.Y), ACCENT_COLOR);
			}

			char text[2]; text[0] = i; text[1] = '\0';
			v4 color = GetFontColor(IsFocused() && exists[i - start]);

			PushText(pState, FONT_Normal, text, position + CenterText(FONT_Normal, text, V2(item_space, size.Height)), color);
		}
	}

	void Update(YaffeState* pState, float pDeltaTime) 
	{
		//Cycle the index until we get to one that exists, or the filter bar
		if (IsLeftPressed() && mode != FILTER_MODE_None)
		{
			if (selected_index < 0) selected_index = ArrayCount(exists) - 1;
			while (!exists[--selected_index] && selected_index != -1);
			FilterRoms();
		}
		else if (IsRightPressed() && mode != FILTER_MODE_None)
		{
			while (!exists[++selected_index] && selected_index != -1)
			{
				if (selected_index == ArrayCount(exists) - 1) selected_index = -2; //-2 because we will increment once before checking the loop
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

			TileFlexBox* box = GetWidget<TileFlexBox*>(UI_Roms);
			box->SetTileFlags(EXECUTABLE_FLAG_None);
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

		v2 position = GetRelativePosition();
		SetPosition(Lerp(position, pDeltaTime * ANIMATION_SPEED, V2(0, is_active ? 0 : -size.Height)));
	}

	void FilterRoms()
	{
		TileFlexBox* box = GetWidget<TileFlexBox*>(UI_Roms);
		//Don't do any filtering when moving to the filter bar
		if (selected_index < 0)
		{
			box->SetTileFlags(EXECUTABLE_FLAG_None);
			return;
		}

		//Hide roms that don't match our particular filter item
		bool index_set = false;
		char filter_char = selected_index + start;
		Platform* platform = GetSelectedPlatform();
		List<Executable> roms = platform->files;

		for (u32 i = 0; i < roms.count; i++)
		{
			Executable* rom = roms.GetItem(i);
			bool display = true;
			switch (mode)
			{
				case FILTER_MODE_None:
					break;
				case FILTER_MODE_Name:
					display = (toupper(rom->display_name[0]) == filter_char);
					break;
				case FILTER_MODE_Players:
					display = rom->players >= (u8)selected_index + 1;
					break;
			}

			u32 flags = EXECUTABLE_FLAG_None;
			if (!display)
			{
				flags |= EXECUTABLE_FLAG_Filtered;
			}
			else if(!index_set)
			{
				g_state.selected_rom = i;
				index_set = true;
			}
			box->SetTileFlags((EXECUTABLE_FLAGS)flags, i);
		}
	}

	void SetExists()
	{
		//Set if a particular filter item exists for our set of roms
		for (u32 i = 0; i < ArrayCount(exists); i++) exists[i] = false;

		List<Executable> roms = GetSelectedPlatform()->files;
		for (u32 i = 0; i < roms.count; i++)
		{
			Executable* rom = roms.GetItem(i);
			s32 index = 0;
			switch (mode)
			{
				case FILTER_MODE_None:
					break;
				case FILTER_MODE_Name:
					index = toupper(rom->display_name[0]) - 'A';
					break;
				case FILTER_MODE_Players:
					index = rom->players - 1;
					break;
				default:
					assert(false);
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