static inline Emulator* GetSelectedEmulator()
{
	return g_state.emulators.GetItem(g_state.selected_emulator);
}

static inline Rom* GetSelectedRom()
{
	return GetSelectedEmulator()->roms.GetItem(g_state.selected_rom);
}

static bool GetNextLine(FILE* pFile, std::string* pLine)
{
	char l[256];
	if (fgets(l, 256, pFile))
	{
		do
		{
			std::string line(l);
			ltrim(&line);
			if (line[0] != '#' && line.length() > 0)
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
	char config_file[MAX_PATH];
	if (!GetFullPathNameA("./Emulators.txt", MAX_PATH, config_file, 0))
	{
		DisplayErrorMessage("Unable to find emulators configuration file", ERROR_TYPE_Error);
		return;
	}

	FILE* file = fopen(config_file, "r");
	if (!file)
	{
		DisplayErrorMessage("Unable to read emulators configuration file", ERROR_TYPE_Error);
		return;
	}

	pState->emulators.InitializeWithArray(new Emulator[32], 32);

	std::string line;
	while (GetNextLine(file, &line))
	{
		assert(pState->emulators.count < 32);
		Emulator* current = pState->emulators.GetItem(pState->emulators.count++);
		strcpy(current->display_name, line.c_str());

		GetFullPathNameA(".\\Assets", MAX_PATH, current->asset_path, 0);
		PathAppendA(current->asset_path, current->display_name);
		if (!PathIsDirectoryA(current->asset_path))
		{
			SECURITY_ATTRIBUTES sa = {};
			CreateDirectoryA(current->asset_path, &sa);
		}

		//Get path to executable
		if (!GetNextLine(file, &line))
		{
			DisplayErrorMessage("Incorrectly configured emulators file", ERROR_TYPE_Error);
			return;
		}
		strcpy(current->start_path, line.c_str());

		//Get path to roms folder
		if (!GetNextLine(file, &line))
		{
			DisplayErrorMessage("Incorrectly configured emulators file", ERROR_TYPE_Error);
			return;
		}
		strcpy(current->rom_path, line.c_str());
	}
	fclose(file);
}

WORK_QUEUE_CALLBACK(DownloadRomAssets)
{
	RomAssetWork* work = (RomAssetWork*)pData;

	GetGameImages(work->platform, work->name, work->banner_loadpath, work->boxart_loadpath);
	delete work;
}

static bool IsValidRomFile(char* pFile)
{
	char* extension = PathFindExtensionA(pFile);
	if (strcmp(extension, ".srm") == 0) return false;
	return true;
}

static u32 CountFilesInDirectory(char* pDirectory)
{
	WIN32_FIND_DATAA file;
	HANDLE h;
	u32 count = 0;
	if ((h = FindFirstFileA(pDirectory, &file)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(file.cFileName, ".") != 0 &&
				strcmp(file.cFileName, "..") != 0)
			{
				if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && IsValidRomFile(file.cFileName)) count++;
			}
		} while (FindNextFileA(h, &file));
	}
	FindClose(h);

	return count;
}

static void QueueAssetDownloads(Rom* pRom, const char* pAssetPath, s32 pPlatform)
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

bool RomsSort(Rom a, Rom b) { return strcmp(a.name, b.name) < 0; }
static void GetRoms(YaffeState* pState, Emulator* pEmulator, bool pForce)
{
	pState->selected_rom = 0;
	g_ui.root->OnEmulatorChanged(pEmulator);

	//Only retrieve rom information if we know the platform or can find it
	//If not, we need to wait for the user to select a platform before we continue
	if (pEmulator->platform == 0)
	{
		if (!GetEmulatorPlatform(pEmulator)) return;
	}

	char path[MAX_PATH];
	sprintf(path, "%s\\*.*", pEmulator->rom_path);

	u32 count = CountFilesInDirectory(path);
	if (pEmulator->roms.count == count && !pForce) return;
	
	if (pEmulator->roms.items) delete pEmulator->roms.items;
	pEmulator->roms.InitializeWithArray(new Rom[count], count);

	WIN32_FIND_DATAA file;
	HANDLE h;
	if ((h = FindFirstFileA(path, &file)) == INVALID_HANDLE_VALUE)
	{
		DisplayErrorMessage("Invalid roms directory", ERROR_TYPE_Warning);
		return;
	}

	do
	{
		//Find first file will always return "."
		//    and ".." as the first two directories. 
		if (strcmp(file.cFileName, ".") != 0 && 
			strcmp(file.cFileName, "..") != 0)
		{
			if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && IsValidRomFile(file.cFileName))
			{
				Rom* rom = pEmulator->roms.items + pEmulator->roms.count++;
				PathCombineA(rom->path, pEmulator->rom_path, file.cFileName);

				PathRemoveExtensionA(file.cFileName);
				strcpy(rom->name, file.cFileName);

				char rom_asset_path[MAX_PATH];
				PathCombineA(rom_asset_path, pEmulator->asset_path, rom->name);

				char boxart_path[MAX_PATH];
				PathCombineA(boxart_path, rom_asset_path, "boxart.jpg");
				strcpy(rom->boxart.load_path, boxart_path);
				rom->boxart.type = ASSET_TYPE_Bitmap;

				char banner_path[MAX_PATH];
				PathCombineA(banner_path, rom_asset_path, "banner.jpg");
				strcpy(rom->banner.load_path, banner_path);
				rom->banner.type = ASSET_TYPE_Bitmap;

				rom->players = GetGamePlayers(pEmulator->platform, rom->name);

				QueueAssetDownloads(rom, rom_asset_path, pEmulator->platform);
			}
		}
	} while (FindNextFileA(h, &file)); //Find the next file. 
	FindClose(h);

	std::sort(pEmulator->roms.items, pEmulator->roms.items + pEmulator->roms.count, RomsSort);
	g_ui.root->OnEmulatorChanged(pEmulator);
}

static void InitializeAssetFiles()
{
	char assets_path[MAX_PATH];
	GetFullPathNameA("./Assets/", MAX_PATH, assets_path, 0);

	if (!PathIsDirectoryA(assets_path))
	{
		SECURITY_ATTRIBUTES sa = {};
		if (!CreateDirectoryA(assets_path, &sa))
		{
			DisplayErrorMessage("Unable to create assets directory", ERROR_TYPE_Warning);
		}
	}
}

