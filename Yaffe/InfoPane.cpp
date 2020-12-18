class InfoPane : public Widget
{
	const float INFO_PANE_WIDTH = 0.33F;
public:
	std::string overview;

	InfoPane(Interface* pInterface) : Widget(UI_Info, pInterface) { }

private:
	void OnAdded()
	{
		SetPosition(V2(parent->size.Width, 0));
		SetSize(INFO_PANE_WIDTH, 1);
	}

	void Render(RenderState* pState)
	{
		const float PADDING = 20.0F;
		Executable* rom = GetSelectedExecutable();
		if (rom)
		{
			v2 position = GetPosition();
			//Background
			PushSizedQuad(pState, position, size, MODAL_BACKGROUND);

			//Calculate correct height based on bitmap size
			float height = 0;
			Bitmap* b = GetBitmap(g_assets, rom->banner);
			if (!b) b = GetBitmap(g_assets, BITMAP_PlaceholderBanner);
			if (b)
			{
				//Banner
				height = (size.Width / b->width) * b->height;
				PushQuad(pState, position, V2(position.X + size.Width, height), b);
			}
			//Accent bar
			PushSizedQuad(pState, position + V2(0, height), V2(UI_MARGIN, size.Height - height), ACCENT_COLOR);

			//Game rating
			BITMAPS rating = BITMAP_None;
			switch (rom->rating)
			{
				case PLATFORM_RATING_Everyone:
					rating = BITMAP_ERSB_Everyone;
					break;
				case PLATFORM_RATING_Everyone10:
					rating = BITMAP_ERSB_Everyone10;
					break;
				case PLATFORM_RATING_Teen:
					rating = BITMAP_ERSB_Teen;
					break;
				case PLATFORM_RATING_Mature:
					rating = BITMAP_ERSB_Mature;
					break;
				case PLATFORM_RATING_AdultOnly:
					rating = BITMAP_ERSB_AdultOnly;
					break;
			}
			if (rating != BITMAP_None)
			{
				PushSizedQuad(pState, position + V2(size.Width - 64 - PADDING, height - 96), V2(64, 96), GetBitmap(g_assets, rating));
			}

			//Description
			if (!overview.empty())
			{
				PushText(pState, FONT_Normal, overview.c_str(), position + V2(PADDING, height + PADDING), TEXT_FOCUSED);
			}
		}
	}

	void Update(YaffeState* pState, float pDeltaTime) 
	{ 
		float destination = IsFocused() ? -size.Width : 0;
		v2 position = GetRelativePosition();
		SetPosition(Lerp(position, pDeltaTime * ANIMATION_SPEED, V2(destination + parent->size.Width, 0)));
		if (IsEscPressed())
		{
			RevertFocus();
		}
	}

	void OnFocusGained()
	{
		Executable* rom = GetSelectedExecutable();
		Platform* emu = GetSelectedPlatform();
		overview = rom->overview;
		if (overview.empty()) overview = "No information available";

		WrapText((char*)overview.c_str(), (u32)overview.size(), FONT_Normal, size.Width - 20 * 2);
	}
};