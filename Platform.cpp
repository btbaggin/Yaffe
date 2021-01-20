static inline Platform* GetSelectedPlatform()
{
	if(g_state.selected_platform >= 0) return g_state.platforms.GetItem(g_state.selected_platform);
	return nullptr;
}

static inline Executable* GetSelectedExecutable()
{
	Platform* app = GetSelectedPlatform();

	if (app) return app->files.GetItem(g_state.selected_rom);
	return nullptr;
}

static void CleanFileName(char* pName, char* pDest)
{
	//Strip any bracket/parenthesis that are commonly appended to names
	bool in_bracket = false;
	u32 j = 0;
	for (u32 i = 0; pName[i] != 0; i++)
	{
		if (pName[i] == '(' || pName[i] == '[')
			in_bracket = true;

		if (!in_bracket)
		{
			pDest[j++] = pName[i];
		}

		if (pName[i] == ')' || pName[i] == ']')
			in_bracket = false;
	}
	pDest[j] = '\0';

	//Trim right whitespace
	while (j > 0 && isspace(pDest[--j]))
	{
		pDest[j] = '\0';
	}
}

void RemovePathExtension(char* pPath)
{
    if (pPath == NULL) return;

    char* lastExt = strrchr(pPath, '.');
    if (lastExt != NULL) *lastExt = '\0';
}

static bool IsValidRomFile(char* pFile)
{
	const char *dot = strrchr(pFile, '.');
    if(!dot || dot == pFile) return false;

    if (strcmp(dot + 1, "srm") == 0) return false;
    return true;
}


static void RefreshExecutables(YaffeState* pState, Platform* pApp)
{
	pState->selected_rom = 0;

	char platform_names[RECENT_COUNT][35];
	switch (pApp->type)
	{
		case PLATFORM_Emulator:
		{
			std::vector<std::string> files = GetFilesInDirectory(pApp->path);
			if (pApp->files.count == files.size()) return;

			YaffeLogInfo("Found %d items for platform %s", (u32)files.size(), pApp->name);

			pApp->files.Initialize((u32)files.size());

			for (u32 j = 0; j < files.size(); j++)
			{
				Executable* exe = pApp->files.AddItem();

				char file_name[MAX_PATH];
				strcpy(file_name, files[j].c_str());
				strcpy(exe->file, file_name);
				exe->platform = pApp->id;

				char clean_file_name[MAX_PATH];
				RemovePathExtension(file_name);
				CleanFileName(file_name, clean_file_name);
				strcpy(exe->display_name, clean_file_name);

				YaffeLogInfo("Processing file %s, name %s", file_name, clean_file_name);

				GetGameInfo(pApp, exe, clean_file_name);
			}
		}
		break;

	case PLATFORM_App:
		GetAllApplications(pState, pApp);
		
		break;

	case PLATFORM_Recents:
		GetRecentGames(pApp, platform_names);
		break;
	}

	if(pApp->type != PLATFORM_Recents) std::sort(pApp->files.items, pApp->files.items + pApp->files.count, RomsSort);

	for (u32 i = 0; i < pApp->files.count; i++)
	{
		Executable* exe = pApp->files.GetItem(i);

		if (!exe->invalid)
		{
			const char* name = pApp->type == PLATFORM_Recents ? platform_names[i] : pApp->name;
			SetAssetPaths(name, exe, &exe->banner, &exe->boxart);
		}
	}
}
