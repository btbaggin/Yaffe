static MODAL_CLOSE(OnAddApplicationModalClose)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		AddApplicationModal* add = (AddApplicationModal*)pContent;
		if (add->application.checked)
		{
			AddNewApplication(add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
		}
		else
		{
			AddNewEmulator(add->GetName(), add->GetExecutable(), add->GetArgs(), add->GetFolder());
		}		
		GetConfiguredEmulators(&g_state);
	}
}

class EmulatorList : public UiControl
{
public:
	List<Application>* applications;

	static void Render(RenderState* pRender, UiRegion pRegion, EmulatorList* pList)
	{
		v4 foreground = pList->IsFocused() ? ACCENT_COLOR : ACCENT_UNFOCUSED;
		v4 font = GetFontColor(pList->IsFocused());

		PushQuad(pRender, pRegion.position, pRegion.size, MENU_BACKGROUND);

		PushText(pRender, FONT_Title, "Yaffe", pRegion.position + V2(UI_MARGIN), TEXT_FOCUSED);
		float start_y = GetFontSize(FONT_Title) + UI_MARGIN * 2;

		const float OFFSET = 30.0F;

		float text_size = GetFontSize(FONT_Normal);
		for (u32 i = 0; i < pList->applications->count; i++)
		{
			Application* app = pList->applications->GetItem(i);
			char* item = app->display_name;
			v2 item_size = V2(pRegion.size.Width - OFFSET, text_size + UI_MARGIN * 2);
			v2 item_position = { pRegion.position.X + OFFSET, pRegion.position.Y + start_y + (i * item_size.Height) };
			if (i == g_state.selected_emulator)
			{
				PushSizedQuad(pRender, item_position, item_size, foreground);
			}

			PushText(pRender, FONT_Normal, (char*)item, item_position, font);
			char count[5]; sprintf(count, "%d", app->files.count);
			float size = MeasureString(FONT_Subtext, count).Width;
			PushText(pRender, FONT_Subtext, count, item_position + V2(pRegion.size.Width - size - OFFSET - UI_MARGIN, 0), font);
		}
	}

	void Update(float pDeltaTime)
	{
		if (IsFocused())
		{
			if (IsUpPressed())
			{
				--g_state.selected_emulator;
				if (g_state.selected_emulator < 0) g_state.selected_emulator = applications->count - 1;
				GetExecutables(&g_state, applications->GetItem(g_state.selected_emulator));
			}
			else if (IsDownPressed())
			{
				g_state.selected_emulator = (g_state.selected_emulator + 1) % applications->count;
				GetExecutables(&g_state, applications->GetItem(g_state.selected_emulator));
			}

			if (IsInfoPressed())
			{
				DisplayModalWindow(&g_state, "Add Emulator", new AddApplicationModal(), BITMAP_None, OnAddApplicationModalClose);
			}

			if (IsEnterPressed())
			{
				FocusElement(UI_Roms);
			}
		}
	}

	EmulatorList(YaffeState* pState) : UiControl(UI_Emulators)
	{
		applications = &pState->emulators;
	}
};