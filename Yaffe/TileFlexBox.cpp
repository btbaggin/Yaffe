class TileFlexBox : public Widget
{
	const float PAGE_PADDING = 0.075F;
	Platform* current_platform;
	s32 first_visible;

	s32 tiles_x;
	s32 tiles_y;

public:
	TileFlexBox(Interface* pInterface) : Widget(UI_Roms, pInterface)
	{
		first_visible = 0;
	}

	void OnAdded()
	{
		SetPosition(RelativeToAbsolute(PAGE_PADDING, PAGE_PADDING));
		SetSize(1 - PAGE_PADDING * 2, 1 - PAGE_PADDING * 2);
	}

	inline s32 GetFirstVisibleTile() { return first_visible - tiles_x * 2; }
	inline s32 GetLastVisibleTile() { return first_visible + (tiles_x * tiles_y) + tiles_x * 2; }
	float GetDefaultItemSize(float pDimension, float pAmount) { return (pDimension - pDimension * PAGE_PADDING * 2) / pAmount; }

	inline void SetTileFlags(EXECUTABLE_FLAGS pFlags, u32 pIndex)
	{
		((ExecutableTile*)children[pIndex])->flags = pFlags;
	}

	void SetTileFlags(EXECUTABLE_FLAGS pFlags)
	{
		for (u32 i = 0; i < child_count; i++)
		{
			SetTileFlags(pFlags, i);
		}
	}

	void Render(RenderState* pState) { }
	void DoRender(RenderState* pState)
	{
		Platform* plat = GetSelectedPlatform();
		if (!plat) return;

		for (u32 i = 0; i < child_count; i++)
		{
			if (i != g_state.selected_rom && IsWidgetVisible(children[i])) //We are going to draw selected item last
			{
				children[i]->Render(pState);
			}
		}

		//Render selected rom last so it's on top of everything else
		Executable* rom = GetSelectedExecutable();
		if (rom)
		{
			children[g_state.selected_rom]->Render(pState);
		}
	}

	void IncrementIndex(s32* pIndex, s32 pAmount, bool pForward, Platform* pEmulator)
	{
		s32 old_index = *pIndex;
		s32 one = pForward ? 1 : -1;
		s32 i = 0;
		//Since certain roms could be filtered out,
		//we will loop until we have incremented the proper amount of times
		for (i = 0; i < pAmount; i++)
		{
			//Move until we have found an unfiltered rom
			s32 new_index = *pIndex + one;
			while (new_index >= 0 &&
				new_index < (s32)child_count &&
				!((ExecutableTile*)children[new_index])->IsVisible())
			{
				new_index += one;
			}

			if (new_index < 0 || new_index >= (s32)child_count) break;
			*pIndex = new_index;
		}

		//If we haven't moved the total amount we intended to
		//revert all changes. This will prevent it going to the last item when pressing down
		if (i != pAmount)
		{
			*pIndex = old_index;
			return;
		}

		//Adjust first_visible index until our index is inside it
		while (*pIndex < first_visible)  first_visible -= tiles_x;
		while (*pIndex > first_visible + (tiles_x * tiles_y) - 1) first_visible += tiles_x;
		assert(first_visible >= 0);
		assert(*pIndex >= 0);
	}

	void Update(YaffeState* pState, float pDeltaTime)
	{
		Platform* e = GetSelectedPlatform();
		if (!e) return;

		if (current_platform != e)
		{
			current_platform = e;
			ClearChildren();

			for (u32 i = 0; i < e->files.count; i++)
			{
				ExecutableTile* w = new ExecutableTile(e->files.GetItem(i), i, &g_ui);
				AddChild(w);
			}
		}

		if (IsFocused())
		{
			if (IsLeftPressed())
			{
				IncrementIndex(&pState->selected_rom, 1, false, e);
			}
			else if (IsRightPressed())
			{
				IncrementIndex(&pState->selected_rom, 1, true, e);
			}
			else if (IsUpPressed())
			{
				IncrementIndex(&pState->selected_rom, tiles_x, false, e);
			}
			else if (IsDownPressed())
			{
				IncrementIndex(&pState->selected_rom, tiles_x, true, e);
			}

			if (IsEnterPressed())
			{
				Executable* exe = GetSelectedExecutable();
				if (exe->platform > 0) UpdateGameLastRun(exe);

				char path[MAX_PATH], args[100], roms[MAX_PATH];
				GetPlatformInfo(exe->platform, path, args, roms);

				g_state.overlay.allow_input = (exe->platform < 0);

				char buffer[1000];
				BuildCommandLine(buffer, exe, path, roms, args);
				StartProgram(&g_state, buffer, path);

			}
			else if (IsInfoPressed())
			{
				FocusElement(UI_Info);
			}
			else if (IsFilterPressed())
			{
				FocusElement(UI_Search);
			}

			if (IsEscPressed())
			{
				RevertFocus();
			}
		}

		UpdateTiles(pState, pDeltaTime);
	}

	void UpdateTiles(YaffeState* pState, float pDeltaTime)
	{
		Platform* plat = GetSelectedPlatform();

		if (plat)
		{
			v2 ideal_tile_size = GetIdealTileSize(plat->type != PLATFORM_Emulator);

			v2 position = GetPosition();
			//Display index of the window (does not include roms which are not displayed)
			s32 effective_i = 0;
			for (s32 i = 0; i < (s32)child_count; i++)
			{
				ExecutableTile* rom = (ExecutableTile*)children[i];
				SizeIndividualTile(rom, ideal_tile_size);

				s32 x = (effective_i % tiles_x);
				s32 y = (effective_i / tiles_x) - (first_visible / tiles_x);

				v2 offset = (ideal_tile_size - rom->size) / 2.0F;

				//Calculate the ideal rom spot
				v2 rom_position = (ideal_tile_size * V2((float)x, (float)y)) //Get tile in position
					+ offset; //Account for different sized images


				//This is to prevent roms from sliding in from 0,0 when the application is first started
				v2 relative_position = rom->GetRelativePosition();
				if (IsZero(relative_position)) rom->SetPosition(rom_position);
				else rom->SetPosition(Lerp(relative_position, pDeltaTime * ANIMATION_SPEED, rom_position));

				if (rom->IsVisible())
				{
					effective_i++;
				}
			}
		}
	}

	v2 GetIdealTileSize(bool pFindMax)
	{
		tiles_x = 1;
		tiles_y = 1;
		if (child_count == 0) return V2(0);

		s32 max_width = 0;
		Bitmap* b = nullptr;
		for (u32 i = 0; i < child_count; i++)
		{
			//Try to find a bitmap actually loaded so we get proper sizes.
			//Should be the first one most times
			ExecutableTile* rom = (ExecutableTile*)children[i];
			Bitmap* bitmap = rom->GetTileBitmap();
			if (bitmap && bitmap->width > max_width)
			{
				b = bitmap;
				max_width = bitmap->width;
				if (!pFindMax) break;
			}
		}

		float width = 0;
		float height = 0;
		if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);
		if (b)
		{
			v2 menu_size = size;
			//Scale based on the larger dimension
			if (b->width > b->height)
			{
				float aspect = (float)b->height / (float)b->width;
				width = GetDefaultItemSize(menu_size.Width, 4);
				height = aspect * width;
			}
			else
			{
				float aspect = (float)b->width / (float)b->height;
				height = GetDefaultItemSize(menu_size.Height, 3);
				width = aspect * height;
			}
			tiles_x = (s32)(menu_size.Width / width);
			tiles_y = (s32)(menu_size.Height / height);
		}

		return V2(width, height);
	}

	void SizeIndividualTile(ExecutableTile* pWidget, v2 pIdealSize)
	{
		//Get bitmap or placeholder if it isn't loaded yet
		Bitmap* bitmap = pWidget->GetTileBitmap();
		if (!bitmap) bitmap = GetBitmap(g_assets, BITMAP_Placeholder);
		if (!bitmap) return;

		v2 tile_size = pIdealSize;
		//By default on the recents menu it chooses the widest game boxart (see pFindMax in GetTileSize)
		//We wouldn't want vertical boxart to stretch to the horizontal dimensions
		//This will scale boxart that is different aspect to fit within the tile_size.Height
		float real_aspect = (float)bitmap->width / (float)bitmap->height;
		float tile_aspect = tile_size.Width / tile_size.Height;

		//If an aspect is wider than it is tall, it is > 1
		//If the two aspect ratios are on other sides of one, it means we need to scale
		if (signbit(real_aspect - 1) != signbit(tile_aspect - 1))
		{
			tile_size.Width = tile_size.Height * real_aspect;
		}

		pWidget->SetSize(tile_size.Width / size.Width, tile_size.Height / size.Height);
	}
};