static inline Platform* GetSelectedApplication()
{
	return g_state.emulators.GetItem(g_state.selected_emulator);
}

static inline Executable* GetSelectedExecutable()
{
	Platform* app = GetSelectedApplication();
	return app->files.GetItem(g_state.selected_rom);
}


static void BuildCommandLine(Executable* pExe, char* pEmuPath, char* pPath, char* pFile, char* pArgs)
{
	char exe_path[MAX_PATH];
	CombinePath(exe_path, pPath, pFile);

	sprintf(pExe->command_line, "\"%s\" %s \"%s\"", pEmuPath, pArgs, exe_path);

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

static void GetConfiguredEmulators(YaffeState* pState)
{
	GetAllPlatforms(pState);

	GetAllApplications(pState);
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
	char path[MAX_PATH];
	char args[100];
	char roms[MAX_PATH];
	GetPlatform(pApp, path, args, roms);

	pState->selected_rom = 0;

	std::vector<std::string> files = GetFilesInDirectory(roms);
	if (pApp->files.count == files.size()) return;

	if (pApp->files.items) delete pApp->files.items;
	pApp->files.InitializeWithArray(new Executable[files.size()], files.size());

	for (u32 j = 0; j < files.size(); j++)
	{
		char* file_name = (char*)files[j].c_str();
		Executable* exe = pApp->files.AddItem();

		BuildCommandLine(exe, path, roms, file_name, args);

		PathRemoveExtensionA(file_name);
		CleanFileName(file_name, exe->name);

		char rom_asset_path[MAX_PATH];
		GetAssetPath(rom_asset_path, pApp, exe);

		CombinePath(exe->boxart.load_path, rom_asset_path, "boxart.jpg");
		exe->boxart.type = ASSET_TYPE_Bitmap;

		CombinePath(exe->banner.load_path, rom_asset_path, "banner.jpg");
		exe->banner.type = ASSET_TYPE_Bitmap;

		GetGameInfo(pApp, exe);
	}

	std::sort(pApp->files.items, pApp->files.items + pApp->files.count, RomsSort);
}
