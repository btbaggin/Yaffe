static MODAL_CLOSE(OnAddApplicationModalClose)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		AddPlatformModal* add = (AddPlatformModal*)pContent;
		if (add->application.checked)
		{
			AddNewApplication(add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
			RefreshExecutables(&g_state, g_state.platforms.GetItem(g_state.platforms.count - 1));
		}
		else
		{
			InsertPlatform(add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
		}		
	}
}

class PlatformList : public UiControl
{
	static const u32 ICON_SIZE = 28.0F;
public:
	List<Platform>* applications;

	static void PushGroupHeader(RenderState* pState, float* pY, v2 pSize, PLATFORM_TYPE pType)
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
		}
		*pY += UI_MARGIN;
		float y = *pY + ICON_SIZE - UI_MARGIN;
		PushSizedQuad(pState, V2(UI_MARGIN, *pY), V2((float)ICON_SIZE), b);
		PushLine(pState, V2(ICON_SIZE + UI_MARGIN * 2, y), V2(pSize.Width - UI_MARGIN, y), 1, V4(1));

		*pY = y + UI_MARGIN * 2;
	}

	static void Render(RenderState* pRender, UiRegion pRegion, PlatformList* pList)
	{
		v4 foreground = pList->IsFocused() ? ACCENT_COLOR : ACCENT_UNFOCUSED;
		v4 font = GetFontColor(pList->IsFocused());
		float current_y = GetFontSize(FONT_Title) + UI_MARGIN * 3 + pRegion.position.Y;

		PushQuad(pRender, pRegion.position, pRegion.size, MENU_BACKGROUND);
		PushText(pRender, FONT_Title, "Yaffe", pRegion.position + V2(UI_MARGIN), TEXT_FOCUSED);
		
		const float OFFSET = ICON_SIZE + UI_MARGIN * 2;
		PLATFORM_TYPE type = PLATFORM_Emulator;
		PushGroupHeader(pRender, &current_y, pRegion.size, type);

		float item_size = GetFontSize(FONT_Normal) + UI_MARGIN * 2;
		for (u32 i = 0; i < pList->applications->count; i++)
		{
			Platform* app = pList->applications->GetItem(i);
			char* item = app->name;

			if (app->type != type)
			{
				PushGroupHeader(pRender, &current_y, pRegion.size, app->type);
				type = app->type;
			}

			v2 item_position = { pRegion.position.X + OFFSET, current_y};
			if (i == g_state.selected_platform)
			{
				PushQuad(pRender, item_position, V2(pRegion.size.Width, item_position.Y + item_size), foreground);
			}

			PushText(pRender, FONT_Normal, (char*)item, item_position, font);
			char count[5]; sprintf(count, "%d", app->files.count);
			float size = MeasureString(FONT_Subtext, count).Width;
			PushText(pRender, FONT_Subtext, count, item_position + V2(pRegion.size.Width - size - OFFSET - UI_MARGIN, 0), font);

			current_y += item_size;
		}
	}

	void Update(float pDeltaTime)
	{
		if (IsFocused())
		{
			if (IsUpPressed())
			{
				--g_state.selected_platform;
				if (g_state.selected_platform < 0) g_state.selected_platform = applications->count - 1;
				RefreshExecutables(&g_state, applications->GetItem(g_state.selected_platform));
			}
			else if (IsDownPressed())
			{
				g_state.selected_platform = (g_state.selected_platform + 1) % applications->count;
				RefreshExecutables(&g_state, applications->GetItem(g_state.selected_platform));
			}

			if (IsInfoPressed())
			{
				DisplayModalWindow(&g_state, "Add Platform", new AddPlatformModal(), BITMAP_None, OnAddApplicationModalClose);
			}

			if (IsEnterPressed())
			{
				FocusElement(UI_Roms);
			}
		}
	}

	PlatformList(YaffeState* pState) : UiControl(UI_Emulators)
	{
		applications = &pState->platforms;
	}
};