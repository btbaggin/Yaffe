class RomMenu : public UiControl
{
public:
	s32 first_visible;

	v2 ideal_tile_size = {};
	s32 tiles_x;
	s32 tiles_y;

	float GetDefaultItemSize(float pDimension, float pAmount)
	{
		return (pDimension - pDimension * ROM_PAGE_PADDING * 2) / pAmount;
	}

	inline s32 GetFirstVisibleTile() { return first_visible - tiles_x; }
	inline s32 GetLastVisibleTile() { return first_visible + (tiles_x * tiles_y) + tiles_x; }

	void Render(RenderState* pState, UiRegion pRegion)
	{
		Platform* plat = GetSelectedPlatform();

		if (plat)
		{
			List<ExecutableDisplay> roms = plat->file_display;
			for (s32 i = max(0, GetFirstVisibleTile()); i < GetLastVisibleTile(); i++)
			{
				if (i >= (s32)roms.count) break;

				ExecutableDisplay* rom = roms.GetItem(i);
				if (!HasFlag(rom->flags, EXECUTABLE_FLAG_Filtered))
				{
					Bitmap* b = GetBitmap(g_assets, rom->boxart);
					if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);

					if (i != g_state.selected_rom) //We are going to draw selected item last
					{
						PushSizedQuad(pState, rom->position, rom->size, b);
					}
				}
			}

			//Render selected rom last so it's on top of everything else
			ExecutableDisplay* rom = g_state.platforms.GetItem(g_state.selected_platform)->file_display.GetItem(g_state.selected_rom);
			Executable* rom2 = GetSelectedExecutable();
			if (rom && !HasFlag(rom->flags, EXECUTABLE_FLAG_Filtered))
			{
				Bitmap* b = GetBitmap(g_assets, rom->boxart);
				if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);

				if (IsFocused())
				{
					//Have alpha fade in as the item grows to full size
					//It is important we use Y here instead of X since X can scale in the recents platform if image sizes differ
					float alpha = (1 - (rom->target_size.Y - rom->size.Y) / 2.0F);

					float height = GetFontSize(FONT_Subtext) + UI_MARGIN;
					v2 menu_position = rom->position + rom->size;

					//Check if we need to push the buttons below the text due to overlap
					float text_width = MeasureString(FONT_Subtext, rom2->display_name).Width + MeasureString(FONT_Subtext, "InfoRun").Width + 40;
					if (text_width > rom->target_size.Width)
					{
						menu_position.Y += height;
						height *= 2;
					}

					//Selected background
					PushSizedQuad(pState,
						rom->position - V2(ROM_OUTLINE_SIZE),
						rom->size + V2(ROM_OUTLINE_SIZE * 2, ROM_OUTLINE_SIZE * 2 + height),
						V4(MODAL_BACKGROUND, alpha * 0.94F));
					//Name
					PushText(pState, FONT_Subtext, rom2->display_name, rom->position + V2(0, rom->size.Height), V4(TEXT_FOCUSED, alpha));

					//Help
					PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonX, 20, FONT_Subtext, "Info", V4(1, 1, 1, alpha));
					PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonA, 20, FONT_Subtext, "Run", V4(1, 1, 1, alpha));
				}

				PushSizedQuad(pState, rom->position, rom->size, b);
			}
		}
	}

	void IncrementIndex(s32* pIndex, s32 pAmount, bool pForward, Platform* pEmulator)
	{
		s32 old_index = *pIndex;
		s32 one = pForward ? 1 : -1;
		List<ExecutableDisplay> files = pEmulator->file_display;
		s32 i = 0;
		//Since certain roms could be filtered out,
		//we will loop until we have incremented the proper amount of times
		for (i = 0; i < pAmount; i++)
		{
			//Move until we have found an unfiltered rom
 			s32 new_index = *pIndex + one;
			while (new_index >= 0 &&
				   new_index < (s32)files.count &&
				   HasFlag(files.GetItem(new_index)->flags, EXECUTABLE_FLAG_Filtered))
			{
				new_index += one;
			}

			if (new_index < 0 || new_index >= (s32)files.count) break;
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

			char buffer[1000];
			BuildCommandLine(buffer, exe, path, roms, args);
			StartProgram(&g_state, buffer);
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

		UpdateTiles(pState, pDeltaTime);
	}

	void UnfocusedUpdate(YaffeState* pState, float pDeltaTime)
	{
		UpdateTiles(pState, pDeltaTime);
	}

	void UpdateTiles(YaffeState* pState, float pDeltaTime)
	{
		Platform* plat = GetSelectedPlatform();

		if (plat)
		{
			UiRegion region;
			region.position = V2(pState->form->width * EMU_MENU_PERCENT, 0);
			region.size = V2(pState->form->width, pState->form->height) - region.position;

			List<ExecutableDisplay> roms = plat->file_display;
			GetTileSize(&roms, region, plat->type != PLATFORM_Emulator);
			s32 effective_i = 0;
			for (s32 i = 0; i < (s32)roms.count; i++)
			{
				ExecutableDisplay* rom = roms.GetItem(i);
				v2 rom_display_start = region.position + (region.size * ROM_PAGE_PADDING);

				s32 x = (effective_i % tiles_x);
				s32 y = (effective_i / tiles_x) - (first_visible / tiles_x);

				v2 offset = (ideal_tile_size - rom->size) / 2.0F;

				//Calculate the ideal rom spot
				v2 rom_position = rom_display_start
					+ (ideal_tile_size * V2((float)x, (float)y)) //Get tile in position
					+ offset //Account for different sized images
					- ((rom->target_size - rom->size) / 2.0F); //Account for growing/shrinking

				rom->size = Lerp(rom->size, pDeltaTime * ANIMATION_SPEED, rom->target_size);

				//This is to prevent roms from sliding in from 0,0 when the application is first started
				if (rom->position.X == 0 && rom->position.Y == 0) rom->position = rom_position;
				rom->position = Lerp(rom->position, pDeltaTime * ANIMATION_SPEED, rom_position);

				if (i >= GetFirstVisibleTile() && i <= GetLastVisibleTile() &&
					!HasFlag(rom->flags, EXECUTABLE_FLAG_Filtered))
				{
					effective_i++;
				}
			}
		}
	}

	void GetTileSize(List<ExecutableDisplay>* pRoms, UiRegion pRegion, bool pFindMax)
	{
		float width = 0;
		float height = 0;
		tiles_x = 1;
		tiles_y = 1;
		if (pRoms->count > 0)
		{
			s32 max_width = 0;
			Bitmap* b = nullptr;
			for (u32 i = 0; i < pRoms->count; i++)
			{
				//Try to find a bitmap actually loaded so we get proper sizes.
				//Should be the first one most times
				ExecutableDisplay* rom = pRoms->GetItem(i);
				Bitmap* bitmap = GetBitmap(g_assets, rom->boxart);
				if (bitmap && bitmap->width > max_width)
				{
					b = bitmap;
					max_width = bitmap->width;
					if (!pFindMax) break;
				}
			}

			if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);
			if (!b) return;

			v2 menu_size = pRegion.size;
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
			ideal_tile_size = V2(width, height);
			tiles_x = (s32)(menu_size.Width / width);
			tiles_y = (s32)(menu_size.Height / height);

			for (s32 i = max(0, GetFirstVisibleTile()); i < GetLastVisibleTile(); i++)
			{
				if (i >= (s32)pRoms->count) return;

				//Try to find a bitmap actually loaded so we get proper sizes.
				//Should be the first one most times
				ExecutableDisplay* rom = pRoms->GetItem(i);
				Bitmap* bitmap = GetBitmap(g_assets, rom->boxart);
				if (!bitmap) bitmap = GetBitmap(g_assets, BITMAP_Placeholder);
				if (!bitmap) continue;

				rom->target_size = ideal_tile_size;
				//By default on the recents menu it chooses the widest game boxart (see pFindMax in GetTileSize)
				//We wouldn't want vertical boxart to stretch to the horizontal dimensions
				//This will scale boxart that is different aspect to fit within the tile_size.Height
				float real_aspect = (float)bitmap->width / (float)bitmap->height;
				float tile_aspect = ideal_tile_size.Width / ideal_tile_size.Height;

				//If an aspect is wider than it is tall, it is > 1
				//If the two aspect ratios are on other sides of one, it means we need to scale
				if (signbit(real_aspect - 1) != signbit(tile_aspect - 1))
				{
					rom->target_size.Width = rom->target_size.Height * real_aspect;
				}

				if (i == g_state.selected_rom && IsFocused())
				{
					rom->target_size = rom->target_size * (1 + SELECTED_ROM_SIZE);
				}
				if (rom->size.X == 0 && rom->size.Y == 0) rom->size = rom->target_size;
			}
		}
	}

	RomMenu() : UiControl(UI_Roms)
	{
		first_visible = 0;
	}
};