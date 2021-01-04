static MODAL_CLOSE(OnAddApplicationModalClose)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		PlatformDetailModal* add = (PlatformDetailModal*)pContent;
		if (add->GetIsApplication())
		{

			AddNewApplication(add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
			RefreshExecutables(pState, pState->platforms.GetItem(pState->platforms.count - 1));
		}
		else
		{
			InsertPlatform(add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
			//RefreshExecutables gets called once we have determined the real platform through YaffeService
		}
	}
}

static MODAL_CLOSE(OnUpdateApplicationModalClose)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		PlatformDetailModal* add = (PlatformDetailModal*)pContent;
		if (add->GetIsApplication())
		{
			UpdateApplication(add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
		}
		else
		{
			UpdatePlatform(add->id, add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
		}
		RefreshExecutables(pState, GetSelectedPlatform());
	}
}

class PlatformList : public Widget
{
public:
	PlatformList(Interface* pInterface, float pWidth) : Widget(UI_Emulators, pInterface)
	{
		SetPosition(V2(0));
		SetSize(pWidth, 1);
	}

	static void PushGroupHeader(RenderState* pState, float* pY, v2 pSize, PLATFORM_TYPE pType, float pIconSize)
	{
		Bitmap* b = nullptr;
		switch (pType)
		{
		case PLATFORM_Emulator:
			b = GetBitmap(g_assets, BITMAP_Emulator);
			break;
		case PLATFORM_App:
			b = GetBitmap(g_assets, BITMAP_App);
			break;
		case PLATFORM_Recents:
			b = GetBitmap(g_assets, BITMAP_Recent);
			break;
		}
		*pY += UI_MARGIN;
		float y = *pY + pIconSize - UI_MARGIN;
		PushSizedQuad(pState, V2(UI_MARGIN, *pY), V2(pIconSize), b);
		PushLine(pState, V2(pIconSize + UI_MARGIN * 2, y), V2(pSize.Width - UI_MARGIN, y), 1, V4(1));

		*pY = y + UI_MARGIN * 2;
	}

	void Render(RenderState* pState)
	{
		const float ICON_SIZE = 28.0F;

		v2 position = GetPosition();
		v4 foreground = IsFocused() ? ACCENT_COLOR : ACCENT_UNFOCUSED;
		v4 font = GetFontColor(IsFocused());
		float current_y = GetFontSize(FONT_Title) + UI_MARGIN * 3 + position.Y;

		//Title
		PushQuad(pState, position, size, MENU_BACKGROUND);
		PushText(pState, FONT_Title, "Yaffe", position + V2(UI_MARGIN), TEXT_FOCUSED);

		//List of platforms
		const float OFFSET = ICON_SIZE + UI_MARGIN * 2;
		PLATFORM_TYPE type = PLATFORM_Invalid;

		float item_size = GetFontSize(FONT_Normal) + UI_MARGIN * 2;
		for (u32 i = 0; i < g_state.platforms.count; i++)
		{
			Platform* app = g_state.platforms.GetItem(i);
			char* item = app->name;

			//Push header when type changes
			if (app->type != type)
			{
				PushGroupHeader(pState, &current_y, size, app->type, ICON_SIZE);
				type = app->type;
			}

			//Selected highlight
			v2 item_position = { position.X + OFFSET, current_y };
			if (i == g_state.selected_platform)
			{
				PushQuad(pState, item_position, V2(size.Width, item_position.Y + item_size), foreground);
			}
			PushText(pState, FONT_Normal, (char*)item, item_position, font);

			//Add count of number of files
			if (app->type != PLATFORM_Recents)
			{
				char count[5]; sprintf(count, "%d", app->files.count);
				float text_size = MeasureString(FONT_Subtext, count).Width;
				PushText(pState, FONT_Subtext, count, item_position + V2(size.Width - text_size - OFFSET - UI_MARGIN, 0), font);
			}

			current_y += item_size;
		}
	}

	void Update(YaffeState* pState, float pDeltaTime)
	{
		if (!IsFocused()) return;
		if (IsUpPressed() && pState->selected_platform > 0)
		{
			pState->selected_platform--;
			RefreshExecutables(pState, GetSelectedPlatform());
		}
		else if (IsDownPressed() && pState->selected_platform < pState->platforms.count - 1)
		{
			pState->selected_platform++;
			RefreshExecutables(pState, GetSelectedPlatform());
		}

		if (IsEnterPressed()) FocusElement(UI_Roms);
		else if (IsInfoPressed())
		{
			Platform* platform = GetSelectedPlatform();
			if (platform && platform->type != PLATFORM_Recents)
			{
				DisplayModalWindow(pState, "Platform Info", new PlatformDetailModal(GetSelectedPlatform(), false), BITMAP_None, OnUpdateApplicationModalClose, "Save");

			}
		}
	}
};