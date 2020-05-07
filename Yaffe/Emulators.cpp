static inline Application* GetSelectedApplication()
{
	return g_state.emulators.GetItem(g_state.selected_emulator);
}

static inline Executable* GetSelectedExecutable()
{
	Application* app = GetSelectedApplication();
	return app->files.GetItem(g_state.selected_rom);
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

static void AddNewEmulator(std::string pName, std::string pPath, std::string pArgs, std::string pRoms)
{
	char config_file[MAX_PATH];
	GetFullPath("./Emulators.txt", config_file);

	FILE* fin = fopen(config_file, "a");
	Verify(fin, "Unable to add emulator", ERROR_TYPE_Warning);

	fputs("\n\n", fin);
	std::string name = pName.append("\n"); fputs(name.c_str(), fin);
	std::string path = pPath.append("\n"); fputs(path.c_str(), fin);
	std::string args = pArgs.append("\n"); fputs(args.c_str(), fin);
	std::string roms = pRoms.append("\n"); fputs(roms.c_str(), fin);

	fclose(fin);
}

static void AddNewApplication(std::string pName, std::string pPath, std::string pArgs, std::string pImage)
{
	char config_file[MAX_PATH];
	GetFullPath(".\\Applications\\", config_file);
	Verify(CreateDirectoryIfNotExists(config_file), "Unable to create applications folder", ERROR_TYPE_Warning);

	char assets[MAX_PATH];
	GetFullPath(".\\Applications\\Assets\\", assets);
	Verify(CreateDirectoryIfNotExists(assets), "Unable to create applications asset folder", ERROR_TYPE_Warning);

	CombinePath(assets, assets, pName.c_str());
	Verify(CreateDirectoryIfNotExists(assets), "Unable to create applications asset folder", ERROR_TYPE_Warning);

	CombinePath(config_file, config_file, pName.c_str());
	Verify(CreateShortcut(pPath.c_str(), pArgs.c_str(), config_file), "Unable to create application shortcut", ERROR_TYPE_Warning);

	CombinePath(assets, assets, "boxart.jpg");
	Verify(CopyFileTo(pImage.c_str(), assets), "Unable to copy application image", ERROR_TYPE_Warning);
}

static void GetConfiguredEmulators(YaffeState* pState)
{
	char config_file[MAX_PATH];
	GetFullPath(".\\Emulators.txt", config_file);

	FILE* fin = fopen(config_file, "r");
	Verify(fin, "Unable to read emulators configuration file", ERROR_TYPE_Error);

	pState->emulators.InitializeWithArray(new Application[32], 32);

	//
	// EMULATORS
	//
	std::string line;
	while (GetNextLine(fin, &line))
	{
		assert(pState->emulators.count < 32);
		Application* current = pState->emulators.GetItem(pState->emulators.count++);
		current->type = APPLICATION_Emulator;
		strcpy(current->display_name, line.c_str());

		GetFullPath(".\\Assets", current->asset_path);
		CombinePath(current->asset_path, current->asset_path, current->display_name);
		CreateDirectoryIfNotExists(current->asset_path);

		//Get path to executable
		Verify(GetNextLine(fin, &line, false), "Incorrectly configured emulators file", ERROR_TYPE_Error);
		strcpy(current->start_path, line.c_str());

		//Get executable args
		Verify(GetNextLine(fin, &line, false), "Incorrectly configured emulators file", ERROR_TYPE_Error);
		strcpy(current->start_args, line.c_str());

		//Get path to roms folder
		Verify(GetNextLine(fin, &line, false), "Incorrectly configured emulators file", ERROR_TYPE_Error);
		strcpy(current->rom_path, line.c_str());

		GetExecutables(pState, current, true);
	}
	fclose(fin);

	//
	// APPLICATIONS
	//
	char applications[MAX_PATH];
	GetFullPath(".\\Applications\\", applications);

	if (PathIsDirectoryA(applications))
	{
		Application* current = pState->emulators.GetItem(pState->emulators.count++);
		current->type = APPLICATION_App;
		strcpy(current->display_name, "Applications");

		strcpy(current->rom_path, applications);

		CombinePath(applications, applications, "Assets\\");
		strcpy(current->asset_path, applications);
		CreateDirectoryIfNotExists(current->asset_path);

		GetExecutables(pState, current, true);
	}
}

WORK_QUEUE_CALLBACK(DownloadRomAssets)
{
	RomAssetWork* work = (RomAssetWork*)pData;

	GetGameImages(work->platform, work->name, work->banner_loadpath, work->boxart_loadpath);
	delete work;
}

static void QueueAssetDownloads(Executable* pRom, const char* pAssetPath, s32 pPlatform)
{
	if (!FileExists(pRom->banner.load_path) || !FileExists(pRom->boxart.load_path))
	{
		SECURITY_ATTRIBUTES sa = {};
		CreateDirectoryA(pAssetPath, &sa);

		RomAssetWork* work = new RomAssetWork();
		work->name = std::string(pRom->name);
		work->banner_loadpath = std::string(pRom->banner.load_path);
		work->boxart_loadpath = std::string(pRom->boxart.load_path);
		work->platform = pPlatform;
		QueueUserWorkItem(g_state.queue, DownloadRomAssets, work);
	}
}

static char* CleanFileName(char* pName)
{
	return pName;
}

bool RomsSort(Executable a, Executable b) { return strcmp(a.name, b.name) < 0; }
static void GetExecutables(YaffeState* pState, Application* pEmulator, bool pForce)
{
	pState->selected_rom = 0;

	//Only retrieve rom information if we know the platform or can find it
	//If not, we need to wait for the user to select a platform before we continue
	u32 platform = 0;
	if (pEmulator->type == APPLICATION_Emulator)
	{
		platform = GetEmulatorPlatform(pEmulator);
	}

	char path[MAX_PATH];
	sprintf(path, "%s\\*.*", pEmulator->rom_path);

	std::vector<std::string> files = GetFilesInDirectory(path);
	if (pEmulator->files.count == files.size() && !pForce) return;
	
	if (pEmulator->files.items) delete pEmulator->files.items;
	pEmulator->files.InitializeWithArray(new Executable[files.size()], (u32)files.size());

	for (u32 i = 0; i < files.size(); i++)
	{
		char* file_name = (char*)files[i].c_str();
		Executable* rom = pEmulator->files.items + pEmulator->files.count++;
		rom->platform = platform;
		CombinePath(rom->path, pEmulator->rom_path, file_name);

		PathRemoveExtensionA(file_name);
		strcpy(rom->name, CleanFileName(file_name));

		char rom_asset_path[MAX_PATH];
		CombinePath(rom_asset_path, pEmulator->asset_path, rom->name);

		char boxart_path[MAX_PATH];
		CombinePath(boxart_path, rom_asset_path, "boxart.jpg");
		strcpy(rom->boxart.load_path, boxart_path);
		rom->boxart.type = ASSET_TYPE_Bitmap;

		char banner_path[MAX_PATH];
		CombinePath(banner_path, rom_asset_path, "banner.jpg");
		strcpy(rom->banner.load_path, banner_path);
		rom->banner.type = ASSET_TYPE_Bitmap;

		rom->players = GetGamePlayers(platform, rom->name);

		QueueAssetDownloads(rom, rom_asset_path, platform);
	}

	std::sort(pEmulator->files.items, pEmulator->files.items + pEmulator->files.count, RomsSort);
}
