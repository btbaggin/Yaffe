static MODAL_CLOSE(OnAddApplicationModalClose)
{
	//TODO
	//Create an application folder to put all application
	//Loading won't really need to change and we just need to put a "Application" emulator if the folder exists
	if (pResult == MODAL_RESULT_Ok)
	{
		AddApplicationModal* add = (AddApplicationModal*)pContent;
		char config_file[MAX_PATH];

		if (add->application.checked)
		{
			GetFullPathNameA("./Applications.txt", MAX_PATH, config_file, 0);
		}
		else
		{
			GetFullPathNameA("./Emulators.txt", MAX_PATH, config_file, 0);
		}


		FILE* fin = fopen(config_file, "a");
		if (!fin)
		{
			DisplayErrorMessage("Unable to add emulator", ERROR_TYPE_Warning);
			return;
		}

		fputs("\n\n", fin);

		
		for (u32 i = 0; i < ArrayCount(add->fields); i++)
		{
			char* line = (char*)malloc(add->fields[i].stringlen + 2);
			strcpy(line, add->fields[i].string);
			line[add->fields[i].stringlen] = '\n'; line[add->fields[i].stringlen + 1] = '\0';
			fputs(line, fin);
			free(line);
		}

		fclose(fin);
	}
}

class EmulatorList : public UiControl
{
public:
	List<Application>* emulators;

	static void Render(RenderState* pRender, UiRegion pRegion, EmulatorList* pList)
	{
		v4 foreground = pList->IsFocused() ? ACCENT_COLOR : ACCENT_UNFOCUSED;
		v4 font = GetFontColor(pList->IsFocused());

		PushQuad(pRender, pRegion.position, pRegion.size, MENU_BACKGROUND);

		float text_size = GetFontSize(FONT_Normal);
		for (u32 i = 0; i < pList->emulators->count; i++)
		{
			char* item = pList->emulators->GetItem(i)->display_name;
			v2 item_size = V2(pRegion.size.Width, text_size + UI_MARGIN * 2);
			v2 item_position = { pRegion.position.X, pRegion.position.Y + (i * item_size.Height) };
			if (i == g_state.selected_emulator)
			{
				PushQuad(pRender, item_position, item_position + item_size, foreground);
			}

			PushText(pRender, FONT_Normal, (char*)item, item_position + CenterText(FONT_Normal, item, item_size), font);
		}
	}

	void Update(float pDeltaTime)
	{
		if (IsFocused())
		{
			if (IsUpPressed())
			{
				--g_state.selected_emulator;
				if (g_state.selected_emulator < 0) g_state.selected_emulator = emulators->count - 1;
				GetRoms(&g_state, emulators->GetItem(g_state.selected_emulator));
			}
			else if (IsDownPressed())
			{
				g_state.selected_emulator = (g_state.selected_emulator + 1) % emulators->count;
				GetRoms(&g_state, emulators->GetItem(g_state.selected_emulator));
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
		emulators = &pState->emulators;
	}
};