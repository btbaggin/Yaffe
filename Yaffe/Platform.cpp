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

static void BuildCommandLine(Executable* pExe, const char* pEmuPath, const char* pPath, const char* pArgs)
{
	char exe_path[MAX_PATH];
	CombinePath(exe_path, pPath, pExe->file);

	sprintf(pExe->command_line, "\"%s\" %s \"%s\"", pEmuPath, pArgs, exe_path);

}

static void BuildCommandLine(Executable* pExe, const char* pPath, const char* pArgs)
{
	sprintf(pExe->command_line, "\"%s\" %s", pPath, pArgs);
}

static bool GetNextLine(FILE* pFile, std::string* pLine, bool pSkipBlankLines = true)
{
	char l[256];
	if (fgets(l, 256, pFile))
	{
		do
		{
			std::string line(l);
			//Trim beginning spaces
			line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int ch) {
				return !std::isspace(ch);
			}));

			if (line[0] != '#' && (line.length() > 0 || !pSkipBlankLines))
			{
				std::replace(line.begin(), line.end(), '\n', '\0');
				*pLine = line;
				return true;
			}
		} while (fgets(l, 256, pFile));
	}

	return false;
}

static void CleanFileName(char* pName, char* pDest)
{
	//Stip any bracket/parenthesis that are commonly appended to names
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

	//Trim right whitespace
	while (j > 0 && isspace(pDest[--j]))
	{
		pDest[j] = '\0';
	}
}

static void RefreshExecutables(YaffeState* pState, Platform* pApp)
{
	pState->selected_rom = 0;

	if (pApp->type == PLATFORM_Emulator)
	{
		char path[MAX_PATH];
		char args[100];
		char roms[MAX_PATH];
		GetPlatformInfo(pApp, path, args, roms);

		std::vector<std::string> files = GetFilesInDirectory(roms);
		if (pApp->files.count == files.size()) return;

		pApp->files.Initialize((u32)files.size());

		for (u32 j = 0; j < files.size(); j++)
		{
			Executable* exe = pApp->files.AddItem();

			char file_name[MAX_PATH];
			strcpy(file_name, files[j].c_str());
			strcpy(exe->file, file_name);

			PathRemoveExtensionA(file_name);
			CleanFileName(file_name, file_name);
			strcpy(exe->display_name, file_name);

			BuildCommandLine(exe, path, roms, args);

			GetGameInfo(pApp, exe, file_name);
		}
	}
	else if (pApp->type == PLATFORM_App)
	{
		GetAllApplications(pState, pApp);
	}
	else if (pApp->type == PLATFORM_Recents)
	{
		GetRecentGames(pApp);
	}

	if (pApp->type != PLATFORM_Recents)
	{
		std::sort(pApp->files.items, pApp->files.items + pApp->files.count, RomsSort);
	}
}
