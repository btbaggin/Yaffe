#define ExecuteUpdate(pStatement) (sqlite3_step(pStatement.stmt) == SQLITE_DONE)
#define ExecuteReader(pStatement) (sqlite3_step(pStatement.stmt) == SQLITE_ROW)
#define GetTextColumn(pStatement, pColumn) ((const char*)sqlite3_column_text(pStatement.stmt, pColumn))
#define GetIntColumn(pStatement, pColumn) sqlite3_column_int(pStatement.stmt, pColumn);
#define BindIntParm(pStatement, pValue) sqlite3_bind_int(pStatement.stmt, pStatement.parm_index++, pValue)
#define BindTextParm(pStatement, pValue) sqlite3_bind_text(pStatement.stmt, pStatement.parm_index++, pValue, -1, nullptr)


//
// GENERAL DATABASE
//
static void InitailizeDatbase(YaffeState* pState)
{
	char path[MAX_PATH];
	GetFullPath(".\\Yaffe.db", path);
	if (!FileExists(path))
	{
		DatabaseConnection con;
		{
			SqlStatement stmt(&con, qs_CreateApplicationTable);
			std::lock_guard<std::mutex> guard(g_state.db_mutex);
			ExecuteUpdate(stmt);
		}
		{
			SqlStatement stmt(&con, qs_CreateGamesTable);
			std::lock_guard<std::mutex> guard(g_state.db_mutex);
			ExecuteUpdate(stmt);
		}
		{
			SqlStatement stmt(&con, qs_CreatePlatformsTable);
			std::lock_guard<std::mutex> guard(g_state.db_mutex);
			ExecuteUpdate(stmt);
		}
	}
}



//
// PLATFORM QUERIES
//
static void GetAllPlatforms(YaffeState* pState)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetAllPlatforms);

	pState->platforms.Initialize(32);

	Platform* recents = pState->platforms.AddItem();
	recents->type = PLATFORM_Recents;
	recents->id = -1;
	strcpy(recents->name, "Recent");
	RefreshExecutables(pState, recents);

	while (ExecuteReader(stmt))
	{
		Platform* current = pState->platforms.AddItem();
		current->type = PLATFORM_Emulator;
		current->id = GetIntColumn(stmt, 0);
		strcpy(current->name, GetTextColumn(stmt, 1));
		RefreshExecutables(pState, current);
	}

	Platform* applications = pState->platforms.AddItem();
	applications->type = PLATFORM_App;
	applications->id = -1;
	strcpy(applications->name, "Applications");
	RefreshExecutables(pState, applications);
}
static void RefreshPlatformList(void* pData) { GetAllPlatforms(&g_state); }
static void WritePlatformToDB(PlatformInfo* pInfo, std::string pOld)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_AddPlatform);
	BindIntParm(stmt, pInfo->id);
	BindTextParm(stmt, pInfo->display_name.c_str());
	BindTextParm(stmt, pInfo->path.c_str());
	BindTextParm(stmt, pInfo->args.c_str());
	BindTextParm(stmt, pInfo->folder.c_str());

	std::lock_guard<std::mutex> guard(g_state.db_mutex);
	Verify(ExecuteUpdate(stmt), "Unable to add new platform", ERROR_TYPE_Error);
	AddTaskCallback(&g_state.callbacks, RefreshPlatformList, nullptr);
}
static MODAL_CLOSE(WritePlatformToDB)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		ListModal<PlatformInfo>* content = (ListModal<PlatformInfo>*)pContent;
		PlatformInfo alias = content->GetSelected();

		WritePlatformToDB(&alias, content->old_value);
	}
}
static WORK_QUEUE_CALLBACK(RetrievePossiblePlatforms)
{
	PlatformInfoWork* work = (PlatformInfoWork*)pData;

	json response;
	YaffeMessage args;
	args.type = MESSAGE_TYPE_Platform;
	args.name = work->name.c_str();
	Verify(SendServiceMessage(g_state.service, &args, &response), "Error communicating with YaffeService", ERROR_TYPE_Error);

	u32 count = (u32)response.get("count").get<double>();
	bool exact = response.get("exact").get<bool>();
	picojson::array games = response.get("platforms").get<picojson::array>();
	std::vector<PlatformInfo> items;
	for (u32 i = 0; i < count; i++)
	{
		json game = games.at(i);
		PlatformInfo pi;
		pi.name = game.get("name").get<std::string>();
		pi.id = (s32)game.get("id").get<double>();
		pi.args = work->args;
		pi.folder = work->folder;
		pi.path = work->path;
		pi.display_name = work->name;
		items.push_back(pi);
	}

	if (exact)
	{
		WritePlatformToDB(&items[0], work->name.c_str());
	}
	else if(count > 0)
	{
		char title[200];
		sprintf(title, "Found %d results for platform '%s'", count, work->name.c_str());
		DisplayModalWindow(&g_state, "Select Emulator", new ListModal<PlatformInfo>(items, work->name.c_str(), title), BITMAP_None, WritePlatformToDB);
	}
	delete work;
}
static void InsertPlatform(char* pName, char* pPath, char* pArgs, char* pRom)
{
	PlatformInfoWork* work = new PlatformInfoWork();
	work->name = pName;
	work->path = pPath;
	work->args = pArgs;
	work->folder = pRom;
	QueueUserWorkItem(g_state.queue, RetrievePossiblePlatforms, work);
}
static void UpdatePlatform(s32 pPlatform, char* pName, char* pPath, char* pArgs, char* pRom)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_UpdatePlatform);
	BindTextParm(stmt, pPath);
	BindTextParm(stmt, pArgs);
	BindTextParm(stmt, pRom);
	BindIntParm(stmt, pPlatform);

	std::lock_guard<std::mutex> guard(g_state.db_mutex);
	Verify(ExecuteUpdate(stmt), "Unable to update platform information", ERROR_TYPE_Warning);
}
static void GetPlatformInfo(Platform* Application, char* pPath, char* pArgs, char* pRoms)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetPlatform);
	BindIntParm(stmt, Application->id);
	Verify(ExecuteReader(stmt), "Unable to locate platform", ERROR_TYPE_Error);

	strcpy(pPath, GetTextColumn(stmt, 0));
	strcpy(pArgs, GetTextColumn(stmt, 1));
	strcpy(pRoms, GetTextColumn(stmt, 2));
}




//
// APPLICATION QUERIES
//
static void AddNewApplication(char* pName, char* pPath, char* pArgs, char* pImage)
{
	char assets[MAX_PATH];
	GetFullPath(".\\Assets\\Applications", assets);
	Verify(CreateDirectoryIfNotExists(assets), "Unable to create applications asset folder", ERROR_TYPE_Warning);

	CombinePath(assets, assets, pName);
	Verify(CreateDirectoryIfNotExists(assets), "Unable to create applications asset folder", ERROR_TYPE_Warning);

	CombinePath(assets, assets, "boxart.jpg");
	Verify(CopyFileTo(pImage, assets), "Unable to copy application image", ERROR_TYPE_Warning);

	DatabaseConnection con;
	SqlStatement stmt(&con, qs_AddApplication);
	BindTextParm(stmt, pName);
	BindTextParm(stmt, pPath);
	BindTextParm(stmt, pArgs);

	std::lock_guard<std::mutex> guard(g_state.db_mutex);
	Verify(ExecuteUpdate(stmt), "Unable to add new application", ERROR_TYPE_Error);
}
static void UpdateApplication(char* pName, char* pPath, char* pArgs, char* pImage)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_UpdateApplication);
	BindTextParm(stmt, pPath);
	BindTextParm(stmt, pArgs);
	BindTextParm(stmt, pName); 

	std::lock_guard<std::mutex> guard(g_state.db_mutex);
	Verify(ExecuteUpdate(stmt), "Unable to update application", ERROR_TYPE_Error);
}
static void GetAllApplications(YaffeState* pState, Platform* pPlat)
{
	pPlat->files.Initialize(16);

	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetAllApplications);
	while (ExecuteReader(stmt))
	{
		Executable* exe = pPlat->files.AddItem();

		strcpy(exe->file, GetTextColumn(stmt, 0));
		strcpy(exe->display_name, exe->file);
		exe->platform = pPlat->id;

		SetAssetPaths(pPlat->name, exe);

		const char* path = GetTextColumn(stmt, 1);
		const char* args = GetTextColumn(stmt, 2);
		BuildCommandLine(exe, path, args);
	}
}




//
// GAME QUERIES
//
static void GetRecentGames(Platform* pPlat)
{
	pPlat->files.Initialize(10);

	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetRecentGames);
	while (ExecuteReader(stmt))
	{
		Executable* exe = pPlat->files.AddItem();

		strcpy(exe->display_name, GetTextColumn(stmt, 0));
		exe->overview = GetTextColumn(stmt, 1);
		exe->players = GetIntColumn(stmt, 2);
		strcpy(exe->file, GetTextColumn(stmt, 3));
		exe->platform = GetIntColumn(stmt, 8);

		BuildCommandLine(exe, GetTextColumn(stmt, 4), GetTextColumn(stmt, 6), GetTextColumn(stmt, 5));

		SetAssetPaths(GetTextColumn(stmt, 7), exe);
	}
}
static void UpdateGameLastRun(Executable* pGame, s32 pPlatform)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_UpdateGameLastRun);
	BindIntParm(stmt, pPlatform);
	BindTextParm(stmt, pGame->file);

	std::lock_guard<std::mutex> guard(g_state.db_mutex);
	ExecuteUpdate(stmt);
}
static void WriteGameToDB(GameInfo* pInfo, std::string pOld)
{
	//We now know exactly what game we wanted, write the values we didn't know
	strcpy(pInfo->exe->display_name, pInfo->name.c_str());
	SetAssetPaths(pInfo->platform_name, pInfo->exe);

	std::string url_base = "https://cdn.thegamesdb.net/images/medium/";
	if (!pInfo->boxart.empty()) DownloadImage((url_base + pInfo->boxart).c_str(), pInfo->exe->boxart->load_path);
	if (!pInfo->banner.empty()) DownloadImage((url_base + pInfo->banner).c_str(), pInfo->exe->banner->load_path);

	{
		DatabaseConnection con;
		SqlStatement stmt(&con, qs_AddGame);
		BindIntParm(stmt, pInfo->id);
		BindIntParm(stmt, pInfo->platform);
		BindTextParm(stmt, pInfo->exe->display_name);
		BindTextParm(stmt, pInfo->overview.c_str());
		BindIntParm(stmt, pInfo->players);
		BindTextParm(stmt, pOld.c_str());

		std::lock_guard<std::mutex> guard(g_state.db_mutex);
		Verify(ExecuteUpdate(stmt), "Unable to update additional game information", ERROR_TYPE_Error);
	}
}
MODAL_CLOSE(WriteGameToDB)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		ListModal<GameInfo>* content = (ListModal<GameInfo>*)pContent;
		GameInfo pi = content->GetSelected();

		WriteGameToDB(&pi, content->old_value);
	}
}
WORK_QUEUE_CALLBACK(RetrievePossibleGames)
{
	GameInfoWork* work = (GameInfoWork*)pData;

	json response;
	YaffeMessage args;
	args.type = MESSAGE_TYPE_Game;
	args.name = work->name.c_str();
	args.platform = work->platform;
	Verify(SendServiceMessage(g_state.service, &args, &response), "Unable to communicate to YaffeService", ERROR_TYPE_Error);

	u32 count = (u32)response.get("count").get<double>();
	u32 exact = response.get("exact").get<bool>();
	picojson::array games = response.get("games").get<picojson::array>();
	std::vector<GameInfo> items;
	for (u32 i = 0; i < count; i++)
	{
		json game = games.at(i);
		GameInfo pi;
		pi.name = game.get("name").get<std::string>();
		pi.id = (s32)game.get("id").get<double>();
		pi.overview = game.get("overview").get<std::string>();
		pi.players = (s32)game.get("players").get<double>();
		pi.banner = game.get("banner").get<std::string>();
		pi.boxart = game.get("boxart").get<std::string>();
		pi.exe = work->exe;
		pi.platform = work->platform;
		strcpy(pi.platform_name, work->platform_name);
		items.push_back(pi);
	}

	if (exact)
	{
		WriteGameToDB(&items[0], work->exe->file);
	}
	else if(count > 0)
	{
		char title[200];
		sprintf(title, "Found %d results for game '%s'", count, work->name.c_str());
		DisplayModalWindow(&g_state, "Select Game", new ListModal<GameInfo>(items, work->exe->file, title), BITMAP_None, WriteGameToDB);
	}
	
	delete work;
}
static void GetGameInfo(Platform* pApp, Executable* pExe, const char* pName)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetGame);
	BindIntParm(stmt, pApp->id);
	BindTextParm(stmt, pExe->file);
	if(ExecuteReader(stmt))
	{
		strcpy(pExe->display_name, GetTextColumn(stmt, 1));
		pExe->overview = GetTextColumn(stmt, 2);
		pExe->players = GetIntColumn(stmt, 3);

		SetAssetPaths(pApp->name, pExe);
	}
	else
	{
		//Taking a pointer to the exe here could cause an invalid reference when we try to write to it in WriteGameToDB
		//However, it seems unlikely due to the circumstances that would need to occur
		GameInfoWork* work = new GameInfoWork();
		work->exe = pExe;
		work->name = pName;
		work->platform = pApp->id;
		strcpy(work->platform_name, pApp->name);
		QueueUserWorkItem(g_state.queue, RetrievePossibleGames, work);
	}
}