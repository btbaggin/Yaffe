static void StartProgram(YaffeState* pState, Application* pEmulator, Executable* pRom);
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
		Application* emu = GetSelectedApplication();
		List<Executable> roms = emu->files;

		v2 roms_display_start = pRegion.position + (pRegion.size * ROM_PAGE_PADDING);

		pMenu->GetTileSize(&roms, pRegion);

		s32 effective_i = 0;
		for (s32 i = 0; i < pMenu->tiles_x * pMenu->tiles_y; i++)
		{
			u32 absolute_index = pMenu->first_visible + i;
			if (absolute_index >= roms.count)  break;

			Executable* rom = roms.GetItem(absolute_index);
			if (!HasFlag(rom->flags, EXECUTABLE_FLAG_Filtered))
			{
				Bitmap* b = GetBitmap(g_assets, &rom->boxart);
				if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);

				u32 x = effective_i % pMenu->tiles_x;
				u32 y = effective_i / pMenu->tiles_x;

				//Calculate the ideal rom spot
				v2 rom_position = roms_display_start + (pMenu->tile_size * V2((float)x, (float)y));
				if (absolute_index == g_state.selected_rom && pMenu->IsFocused())
				{
					//Selected rom needs to move a little bit to account for the size increase
					rom_position -= pMenu->tile_size * (SELECTED_ROM_SIZE / 2.0F);
				}
				//This is to prevent roms from sliding in from 0,0 when the application is first started
				if (rom->position.X == 0 && rom->position.Y == 0) rom->position = rom_position;

				rom->position = Lerp(rom->position, g_state.time.delta_time * 5, rom_position); 
				if (absolute_index != g_state.selected_rom) //We are going to draw selected item last
				{
					PushSizedQuad(pState, rom->position, pMenu->tile_size, b);
				}
				effective_i++;
			}
		}

		//Render selected rom last so it's on top of everything else
		Executable* rom = GetSelectedExecutable();
		if (rom && !HasFlag(rom->flags, EXECUTABLE_FLAG_Filtered))
		{
			Bitmap* b = GetBitmap(g_assets, &rom->boxart);
			if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);

			//If we aren't focused we want to render everything the same size
			if(!pMenu->IsFocused()) pMenu->selected_size = pMenu->tile_size;
			PushSizedQuad(pState, rom->position, pMenu->selected_size, b);

			if (pMenu->IsFocused())
			{
				float bar_height = GetFontSize(FONT_Title) + UI_MARGIN * 2;
				PushText(pState, FONT_Title, rom->name, V2(pRegion.position.X, pRegion.size.Height - bar_height), V4(0, 0, 0, 1));
			}
		}
	}

	void IncrementIndex(s32* pIndex, s32 pAmount, bool pForward, Application* pEmulator)
	{
		s32 old_index = *pIndex;
		s32 one = pForward ? 1 : -1;
		List<Executable> roms = pEmulator->files;
		s32 i = 0;
		//Since certain roms could be filtered out,
		//we will loop until we have incremented the proper amount of times
		for (i = 0; i < pAmount; i++)
		{
			//Move until we have found an unfiltered rom
 			s32 new_index = *pIndex + one;
			while (new_index >= 0 &&
				   new_index < (s32)roms.count &&
				   HasFlag(roms.GetItem(new_index)->flags, EXECUTABLE_FLAG_Filtered))
			{
				new_index += one;
			}

			if (new_index < 0 || new_index >= (s32)roms.count) break;
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
		Application* e = GetSelectedApplication();


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
				Executable* r = e->files.GetItem(state->selected_rom);
				if (r)
				{
					StartProgram(&g_state, e, r);
				}
			}
			else if (e->type != APPLICATION_App && IsInfoPressed())
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

			selected_size = Lerp(selected_size, pDeltaTime * 5, tile_size * (1.0F + SELECTED_ROM_SIZE));
		}
	}

	float GetDefaultItemSize(float pDimension, float pAmount)
	{
		return (pDimension - pDimension * ROM_PAGE_PADDING * 2) / pAmount;
	}

	void GetTileSize(List<Executable>* pRoms, UiRegion pRegion)
	{
		float width = 0;
		float height = 0;
		tiles_x = 1;
		tiles_y = 1;
		if (pRoms->count > 0)
		{
			Bitmap* bitmap = nullptr;
			for (u32 i = 0; i < pRoms->count; i++)
			{
				//Try to find a bitmap actually loaded so we get proper sizes.
				//Should be the first one most times
				Executable* rom = pRoms->GetItem(i);
				Bitmap* b = GetBitmap(g_assets, &rom->boxart);
				if (b)
				{
					bitmap = b;
					break;
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