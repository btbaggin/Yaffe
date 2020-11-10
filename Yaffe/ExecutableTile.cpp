class ExecutableTile : public Widget
{
	const float SELECTED_SCALAR = 0.2F;
	const float ROM_OUTLINE_SIZE = 5;
	Executable* exe;
	const float ANIMATION_TIME = 0.3F;
	u32 index;
	bool focused;
	float time;

public:
	u8 flags;

	ExecutableTile(Executable* pExe, u32 pIndex, Interface* pInterface) : Widget(UI_None, pInterface)
	{
		exe = pExe;
		index = pIndex;
	}

	bool IsVisible()
	{
		return !HasFlag(flags, EXECUTABLE_FLAG_Filtered);
	}

	Bitmap* GetTileBitmap()
	{
		Executable* ed = GetSelectedPlatform()->files.GetItem(index);
		return GetBitmap(g_assets, ed->boxart);
	}

	void Render(RenderState* pState)
	{
		if (!IsVisible()) return;

		Bitmap* b = GetTileBitmap();
		if (!b) b = GetBitmap(g_assets, BITMAP_Placeholder);

		v2 position = GetPosition();
		v2 target_size = size;
		if (focused)
		{
			float animation_remainder = (ANIMATION_TIME - time) / ANIMATION_TIME;
			//Have alpha fade in as the item grows to full size
			float alpha = pow(animation_remainder, 2);

			float height = GetFontSize(FONT_Subtext);
			target_size *= 1 + animation_remainder * SELECTED_SCALAR;
			//Center the tile while it's growing
			position -= (target_size - size) / 2.0F;
			v2 menu_position = position + target_size;

			//We need to copy the name so we can text wrap and things
			char render_name[80];
			strcpy(render_name, exe->display_name);
			WrapText(render_name, strlen(render_name), FONT_Subtext, target_size.Width);

			v2 name_size = MeasureString(FONT_Subtext, render_name);
			height = max(height, name_size.Height);

			//Check if we need to push the buttons below the text due to overlap
			float text_width = name_size.Width + MeasureString(FONT_Subtext, "Info Run").Width + 40;
			if (text_width > target_size.Width)
			{
				menu_position.Y += height;
				height += GetFontSize(FONT_Subtext);
			}

			//Selected background
			PushSizedQuad(pState,
				position - V2(ROM_OUTLINE_SIZE),
				target_size + V2(ROM_OUTLINE_SIZE * 2, ROM_OUTLINE_SIZE * 2 + height),
				V4(MODAL_BACKGROUND, alpha * 0.94F));

			//Name
			PushText(pState, FONT_Subtext, render_name, position + V2(0, target_size.Height), V4(TEXT_FOCUSED, alpha));

			//Help
			PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonX, 20, FONT_Subtext, "Info", V4(1, 1, 1, alpha));
			PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonA, 20, FONT_Subtext, "Run", V4(1, 1, 1, alpha));
		}

		PushSizedQuad(pState, position, target_size, b);
	}

	void Update(YaffeState* pState, float pDeltaTime)
	{
		if (pState->selected_rom == index && GetFocusedElement() == UI_Roms)
		{
			if (!focused)
			{
				focused = true;
				time = ANIMATION_TIME;
			}
		}
		else if (focused)
		{
			focused = false;
		}

		time = max(0, time - pDeltaTime);
	}
};