#define ExecuteUpdate(pStatement) (sqlite3_step(pStatement.stmt) == SQLITE_DONE)
#define ExecuteReader(pStatement) (sqlite3_step(pStatement.stmt) == SQLITE_ROW)
#define GetTextColumn(pStatement, pColumn) ((const char*)sqlite3_column_text(pStatement.stmt, pColumn))
#define GetIntColumn(pStatement, pColumn) sqlite3_column_int(pStatement.stmt, pColumn)
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

static u8 GetRatingFromName(std::string pRating)
{
	if (pRating == "E - Everyone") return PLATFORM_RATING_Everyone;
	else if (pRating == "E10+ - Everyone 10+") return PLATFORM_RATING_Everyone10;
	else if (pRating == "T - Teen") return PLATFORM_RATING_Teen;
	else if (pRating == "M - Mature 17+") return PLATFORM_RATING_Mature;
	else if (pRating == "AO - Adult Only 18+") return PLATFORM_RATING_AdultOnly;
	return PLATFORM_RATING_Pending;
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
		strcpy(current->path, GetTextColumn(stmt, 2));
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
	BindTextParm(stmt, pInfo->display_name);
	BindTextParm(stmt, pInfo->path);
	BindTextParm(stmt, pInfo->args != nullptr ? pInfo->args : "");
	BindTextParm(stmt, pInfo->folder);

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
	args.name = work->name;
	if (SendServiceMessage(g_state.service, &args, &response))
	{
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
			WritePlatformToDB(&items[0], work->name);
		}
		else if (count > 0)
		{
			char title[200];
			sprintf(title, "Found %d results for platform '%s'", count, work->name);
			DisplayModalWindow(&g_state, "Select Emulator", new ListModal<PlatformInfo>(items, work->name, title), BITMAP_None, WritePlatformToDB);
		}
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
	QueueUserWorkItem(g_state.work_queue, RetrievePossiblePlatforms, work);
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
static void GetPlatformInfo(s32 pPlatform, char* pPath, char* pArgs, char* pRoms)
{
	DatabaseConnection con;

	if (pPlatform < 0)
	{
		SqlStatement stmt(&con, qs_GetApplication);
		BindIntParm(stmt, pPlatform);
		Verify(ExecuteReader(stmt), "Unable to locate platform", ERROR_TYPE_Error);

		if (pPath) strcpy(pPath, GetTextColumn(stmt, 1)); else pPath[0] = 0;
		if (pArgs) strcpy(pArgs, GetTextColumn(stmt, 2)); else pArgs[0] = 0;
		pRoms[0] = 0;
	}
	else
	{
		SqlStatement stmt(&con, qs_GetPlatform);
		BindIntParm(stmt, pPlatform);
		Verify(ExecuteReader(stmt), "Unable to locate platform", ERROR_TYPE_Error);

		if (pPath) strcpy(pPath, GetTextColumn(stmt, 1)); else pPath[0] = 0;
		if (pArgs) strcpy(pArgs, GetTextColumn(stmt, 2)); else pArgs[0] = 0;
		if (pRoms) strcpy(pRoms, GetTextColumn(stmt, 3)); else pRoms[0] = 0;
	}
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

	s32 id = -1;
	{
		SqlStatement stmt(&con, qs_GetApplicationID);
		Verify(ExecuteReader(stmt), "Unable to get application ID", ERROR_TYPE_Error);
		id = GetIntColumn(stmt, 0);
	}

	SqlStatement stmt(&con, qs_AddApplication);
	BindIntParm(stmt, id);
	BindTextParm(stmt, pName);
	BindTextParm(stmt, pPath != nullptr ? pPath : "");
	BindTextParm(stmt, pArgs != nullptr ? pArgs : "");

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

		exe->platform = GetIntColumn(stmt, 0);
		strcpy(exe->file, GetTextColumn(stmt, 1));
		strcpy(exe->display_name, exe->file);
	}
}




//
// GAME QUERIES
//
static void GetRecentGames(Platform* pPlat, char pNames[RECENT_COUNT][35])
{
	pPlat->files.Initialize(RECENT_COUNT);

	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetRecentGames);
	u32 i = 0;
	while (ExecuteReader(stmt))
	{
		Executable* exe = pPlat->files.AddItem();

		strcpy(exe->display_name, GetTextColumn(stmt, 0));
		exe->overview = GetTextColumn(stmt, 1);
		exe->players = max(1, GetIntColumn(stmt, 2)); //Some games might not report players so it's null (0), assume it's 1
		exe->rating = (PLATFORM_RATINGS)GetIntColumn(stmt, 3);
		strcpy(exe->file, GetTextColumn(stmt, 4));
		exe->platform = GetIntColumn(stmt, 5);
		strcpy(pNames[i++], GetTextColumn(stmt, 6));
	}
}
static void UpdateGameLastRun(Executable* pGame)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_UpdateGameLastRun);
	BindIntParm(stmt, pGame->platform);
	BindTextParm(stmt, pGame->file);

	std::lock_guard<std::mutex> guard(g_state.db_mutex);
	ExecuteUpdate(stmt);
}
static void WriteGameToDB(GameInfo* pInfo, std::string pOld)
{
	//We now know exactly what game we wanted, write the values we didn't know
	strcpy(pInfo->exe->display_name, pInfo->name.c_str());
	SetAssetPaths(pInfo->platform_name, pInfo->exe, &pInfo->exe->banner, &pInfo->exe->boxart);

	std::string url_base = "https://cdn.thegamesdb.net/images/medium/";
	if (!pInfo->boxart_url.empty()) DownloadImage((url_base + pInfo->boxart_url).c_str(), pInfo->exe->boxart);
	if (!pInfo->banner_url.empty()) DownloadImage((url_base + pInfo->banner_url).c_str(), pInfo->exe->banner);

	{
		DatabaseConnection con;
		SqlStatement stmt(&con, qs_AddGame);
		BindIntParm(stmt, pInfo->id);
		BindIntParm(stmt, pInfo->platform);
		BindTextParm(stmt, pInfo->exe->display_name);
		BindTextParm(stmt, pInfo->overview.c_str());
		BindIntParm(stmt, pInfo->players);
		BindIntParm(stmt, pInfo->rating);
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
	if (SendServiceMessage(g_state.service, &args, &response))
	{
		u32 count = (u32)response.get("count").get<double>();
		bool exact = response.get("exact").get<bool>();
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
			pi.banner_url = game.get("banner").get<std::string>();
			pi.boxart_url = game.get("boxart").get<std::string>();
			pi.exe = work->exe;
			pi.rating = GetRatingFromName(game.get("rating").get<std::string>());
			pi.platform = work->platform;
			strcpy(pi.platform_name, work->platform_name);
			items.push_back(pi);
		}

		if (exact)
		{
			WriteGameToDB(&items[0], work->exe->file);
		}
		else if (count > 0)
		{
			char title[200];
			sprintf(title, "Found %d results for game '%s'", count, work->name.c_str());
			DisplayModalWindow(&g_state, "Select Game", new ListModal<GameInfo>(items, work->exe->file, title), BITMAP_None, WriteGameToDB);
		}
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
		pExe->players = max(1, GetIntColumn(stmt, 3)); //Some games might not report players so it's null (0), assume it's 1
		pExe->rating = (PLATFORM_RATINGS)GetIntColumn(stmt, 4);
	}
	else
	{
		pExe->invalid = true;

		//Taking a pointer to the exe here could cause an invalid reference when we try to write to it in WriteGameToDB
		//However, it seems unlikely due to the circumstances that would need to occur
		GameInfoWork* work = new GameInfoWork();
		work->exe = pExe;
		work->name = pName;
		work->platform = pApp->id;
		strcpy(work->platform_name, pApp->name);
		QueueUserWorkItem(g_state.work_queue, RetrievePossibleGames, work);
	}
}