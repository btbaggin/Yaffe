class RomMenu : public UiControl
{
public:
	s32 first_visible;
	v2 selected_size;
	YaffeState* state;

	v2 tile_size;
	s32 tiles_x;
	s32 tiles_y;

	static void Render(RenderState* pState, UiRegion pRegion, RomMenu* pMenu)
	{
		Platform* plat = GetSelectedApplication();
		List<Executable> roms = plat->files;

		v2 roms_display_start = pRegion.position + (pRegion.size * ROM_PAGE_PADDING);

		pMenu->GetTileSize(&roms, pRegion, plat->type == PLATFORM_Recents);

		s32 effective_i = 0;
		v2 selected_size = pMenu->selected_size;
		for (s32 i = 0; i < pMenu->tiles_x * pMenu->tiles_y; i++)
		{
			u32 absolute_index = pMenu->first_visible + i;
			if (absolute_index >= roms.count) break;

			Executable* rom = roms.GetItem(absolute_index);
			if (!HasFlag(rom->flags, EXECUTABLE_FLAG_Filtered))
			{
				Bitmap* b = GetBitmap(g_assets, rom->boxart);
				if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);

				u32 x = effective_i % pMenu->tiles_x;
				u32 y = effective_i / pMenu->tiles_x;

				v2 offset = {};
				v2 tile_size = pMenu->tile_size;
				if (b)
				{
					//By default on the recents menu it chooses the widest game boxart (see pFindMax in GetTileSize)
					//We wouldn't want vertical boxart to stretch to the horizontal dimensions
					//This will scale boxart that is different aspect to fit within the tile_size.Height
					float real_aspect = (float)b->width / (float)b->height;
					float tile_aspect = tile_size.Width / tile_size.Height;

					//If an aspect is wider than it is tall, it is > 1
					//If the two aspect ratios are on other sides of one, it means we need to scale
					if (signbit(real_aspect - 1) != signbit(tile_aspect - 1))
					{
						tile_size.Width = tile_size.Height * real_aspect;
						offset.X += tile_size.Height * real_aspect / 2.0F;
						if(absolute_index == g_state.selected_rom) selected_size.Width = selected_size.Height * real_aspect;
					}
				}

				//Calculate the ideal rom spot
				v2 rom_position = roms_display_start + (pMenu->tile_size * V2((float)x, (float)y)) + offset;
				if (absolute_index == g_state.selected_rom && pMenu->IsFocused())
				{
					//Selected rom needs to move a little bit to account for the size increase
					rom_position -= tile_size * (SELECTED_ROM_SIZE / 2.0F);
				}

				//This is to prevent roms from sliding in from 0,0 when the application is first started
				if (rom->position.X == 0 && rom->position.Y == 0) rom->position = rom_position;
				rom->position = Lerp(rom->position, g_state.time.delta_time * 5, rom_position); 

				if (absolute_index != g_state.selected_rom) //We are going to draw selected item last
				{
					PushSizedQuad(pState, rom->position, tile_size, b);
				}
				effective_i++;
			}
		}

		//Render selected rom last so it's on top of everything else
		Executable* rom = GetSelectedExecutable();
		if (rom && !HasFlag(rom->flags, EXECUTABLE_FLAG_Filtered))
		{
			Bitmap* b = GetBitmap(g_assets, rom->boxart);
			if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);

			//If we aren't focused we want to render everything the same size
			if(!pMenu->IsFocused()) pMenu->selected_size = pMenu->tile_size;
			else
			{
				//Have alpha fade in as the item grows to full size
				//It is important we use Y here instead of X since X can scale in the recents platform if image sizes differ
				float alpha = (1 - ((pMenu->tile_size.Y * SELECTED_ROM_SIZE) - (selected_size.Y - pMenu->tile_size.Y)));

				float height = GetFontSize(FONT_Subtext) + UI_MARGIN;
				v2 menu_position = rom->position + selected_size;// +V2(ROM_OUTLINE_SIZE, height);

				//Selected background
				PushSizedQuad(pState,
					rom->position - V2(ROM_OUTLINE_SIZE),
					selected_size + V2(ROM_OUTLINE_SIZE * 2, ROM_OUTLINE_SIZE * 2 + height),
					V4(MODAL_BACKGROUND.R, MODAL_BACKGROUND.G, MODAL_BACKGROUND.B, alpha * 0.94F));
				//Name
				PushText(pState, FONT_Subtext, rom->display_name, rom->position + V2(0, selected_size.Height), V4(TEXT_FOCUSED.R, TEXT_FOCUSED.G, TEXT_FOCUSED.B, alpha));

				//Help
				PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonX, 20, FONT_Subtext, "Info", V4(1, 1, 1, alpha));
				PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonA, 20, FONT_Subtext, "Run", V4(1, 1, 1, alpha));
			}

			PushSizedQuad(pState, rom->position, selected_size, b);
		}
	}

	void IncrementIndex(s32* pIndex, s32 pAmount, bool pForward, Platform* pEmulator)
	{
		s32 old_index = *pIndex;
		s32 one = pForward ? 1 : -1;
		List<Executable> files = pEmulator->files;
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

		selected_size = tile_size;
	}

	void Update(float pDeltaTime)
	{
		Platform* e = GetSelectedApplication();
		if (IsFocused())
		{
			if (IsLeftPressed())
			{
				IncrementIndex(&state->selected_rom, 1, false, e);
			}
			else if (IsRightPressed())
			{
				IncrementIndex(&state->selected_rom, 1, true, e);
			}
			else if (IsUpPressed())
			{
				IncrementIndex(&state->selected_rom, tiles_x, false, e);
			}
			else if (IsDownPressed())
			{
				IncrementIndex(&state->selected_rom, tiles_x, true, e);
			}

			if (IsEnterPressed())
			{
				Executable* r = GetSelectedExecutable();
				if (e->type == PLATFORM_Emulator)
				{
					UpdateGameLastRun(r, e->id);
				}
				StartProgram(&g_state, r);
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

			selected_size = Lerp(selected_size, pDeltaTime * ANIMATION_SPEED, tile_size * (1.0F + SELECTED_ROM_SIZE));
		}
	}

	float GetDefaultItemSize(float pDimension, float pAmount)
	{
		return (pDimension - pDimension * ROM_PAGE_PADDING * 2) / pAmount;
	}

	void GetTileSize(List<Executable>* pRoms, UiRegion pRegion, bool pFindMax)
	{
		float width = 0;
		float height = 0;
		tiles_x = 1;
		tiles_y = 1;
		if (pRoms->count > 0)
		{
			float max_width = 0;
			Bitmap* bitmap = nullptr;
			for (u32 i = 0; i < pRoms->count; i++)
			{
				//Try to find a bitmap actually loaded so we get proper sizes.
				//Should be the first one most times
				Executable* rom = pRoms->GetItem(i);
				Bitmap* b = GetBitmap(g_assets, rom->boxart);
				if (b && b->width > max_width)
				{
					bitmap = b;
					if(!pFindMax) break;
				}
			}

			if (!bitmap) bitmap = GetBitmap(g_assets, BITMAP_Placeholder);
			if (!bitmap) return;

			v2 menu_size = pRegion.size;
			//Scale based on the larger dimension
			if (bitmap->width > bitmap->height)
			{
				float aspect = (float)bitmap->height / (float)bitmap->width;
				width = GetDefaultItemSize(menu_size.Width, 4);
				height = aspect * width;
			}
			else
			{
				float aspect = (float)bitmap->width / (float)bitmap->height;
				height = GetDefaultItemSize(menu_size.Height, 3);
				width = aspect * height;
			}

			tile_size = V2(width, height);
			tiles_x = (s32)(menu_size.Width / width);
			tiles_y = (s32)(menu_size.Height / height);
		}
	}

	RomMenu(YaffeState* pState) : UiControl(UI_Roms)
	{
		state = pState;
		selected_size = tile_size;
		first_visible = 0;
	}
};