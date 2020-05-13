#define ExecuteUpdate(pStatement) (sqlite3_step(pStatement.stmt) == SQLITE_DONE)
#define ExecuteReader(pStatement) (sqlite3_step(pStatement.stmt) == SQLITE_ROW)
#define GetTextColumn(pStatement, pColumn) ((const char*)sqlite3_column_text(pStatement.stmt, pColumn))
#define GetIntColumn(pStatement, pColumn) sqlite3_column_int(pStatement.stmt, pColumn);
#define BindIntParm(pStatement, pValue) sqlite3_bind_int(pStatement.stmt, pStatement.parm_index++, pValue)
#define BindTextParm(pStatement, pValue) sqlite3_bind_text(pStatement.stmt, pStatement.parm_index++, pValue, -1, nullptr)

#pragma comment(lib, "urlmon.lib")
#include <urlmon.h>
static void DownloadImage(const char* pUrl, std::string pSlot)
{
	IStream* stream;
	//Also works with https URL's - unsure about the extent of SSL support though.
	HRESULT result = URLOpenBlockingStreamA(0, pUrl, &stream, 0, 0);
	Verify(result == 0, "Unable to retrieve image", ERROR_TYPE_Warning);

	static const u32 READ_AMOUNT = 100;
	HANDLE file = CreateFileA(pSlot.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	char buffer[READ_AMOUNT];
	unsigned long bytesRead;
	stream->Read(buffer, READ_AMOUNT, &bytesRead);
	while (bytesRead > 0U)
	{
		WriteFile(file, buffer, READ_AMOUNT, NULL, NULL);
		stream->Read(buffer, READ_AMOUNT, &bytesRead);
	}
	stream->Release();
	CloseHandle(file);
}


static void RefreshPlatformList(void* pData) { GetConfiguredEmulators(&g_state); }
static void WritePlatformToDB(PlatformInfo* pInfo, std::string pOld)
{
	std::lock_guard<std::mutex> guard(db_mutex);
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_AddPlatform);
	BindIntParm(stmt, pInfo->id);
	BindTextParm(stmt, pInfo->display_name.c_str());
	BindTextParm(stmt, pInfo->path.c_str());
	BindTextParm(stmt, pInfo->args.c_str());
	BindTextParm(stmt, pInfo->folder.c_str());
	Verify(ExecuteUpdate(stmt), "Unable to add new platform", ERROR_TYPE_Error);
	AddTaskCallback(&g_state.callbacks, RefreshPlatformList, nullptr);
}
MODAL_CLOSE(WritePlatformToDB)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		ListModal<PlatformInfo>* content = (ListModal<PlatformInfo>*)pContent;
		PlatformInfo alias = content->GetSelected();

		WritePlatformToDB(&alias, content->old_value);
	}
}
WORK_QUEUE_CALLBACK(RetrievePossiblePlatforms)
{
	PlatformInfoWork* work = (PlatformInfoWork*)pData;

	json j = CreateServiceMessage(MESSAGE_TYPE_Platform, work->name.c_str(), 0);
	json response;
	Verify(SendServiceMessage(g_state.service, j, &response), "Error communicating with YaffeService", ERROR_TYPE_Error);

	u32 count = response["count"].get<u32>();

	json games = response["platforms"];
	std::vector<PlatformInfo> items;
	for (u32 i = 0; i < count; i++)
	{
		json game = games.at(i);
		PlatformInfo pi;
		pi.name = game["name"].get<std::string>();
		pi.args = work->args;
		pi.folder = work->folder;
		pi.path = work->path;
		pi.id = game["id"].get<s32>();
		pi.display_name = work->name;
		items.push_back(pi);
	}

	if (count > 1)
	{
		char title[200];
		sprintf(title, "Found %d results for platform '%s'", count, work->name.c_str());
		DisplayModalWindow(&g_state, "Select Emulator", new ListModal<PlatformInfo>(items, work->name.c_str(), title), BITMAP_None, WritePlatformToDB);
	}
	else if(count == 1)
	{
		WritePlatformToDB(&items[0], work->name.c_str());
	}
	
}
static void GetAllPlatforms(YaffeState* pState)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetAllPlatforms);
	
	pState->emulators.InitializeWithArray(new Platform[32], 32);
	while (ExecuteReader(stmt))
	{
		Platform* current = pState->emulators.GetItem(pState->emulators.count++);
		current->type = APPLICATION_Emulator;
		current->platform = GetIntColumn(stmt, 0);
		strcpy(current->name, GetTextColumn(stmt, 1));

		RefreshExecutables(pState, current);
	}
}
static void GetPlatform(Platform* Application, char* pPath, char* pArgs, char* pRoms)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetPlatform);
	BindIntParm(stmt, Application->platform);
	Verify(ExecuteReader(stmt), "Unable to locate platform", ERROR_TYPE_Error);

	strcpy(pPath, GetTextColumn(stmt, 0));
	strcpy(pArgs, GetTextColumn(stmt, 1));
	strcpy(pRoms, GetTextColumn(stmt, 2));
}
static void InsertPlatform(std::string pName, std::string pPath, std::string pArgs, std::string pRom)
{
	PlatformInfoWork* work = new PlatformInfoWork();
	work->name = pName;
	work->path = pPath;
	work->args = pArgs;
	work->folder = pRom;
	QueueUserWorkItem(g_state.queue, RetrievePossiblePlatforms, work);
}




static void AddNewApplication(std::string pName, std::string pPath, std::string pArgs, std::string pImage)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_AddApplication);
	BindTextParm(stmt, pName.c_str());
	BindTextParm(stmt, pPath.c_str());
	BindTextParm(stmt, pArgs.c_str());
	Verify(ExecuteUpdate(stmt), "Unable to add new application", ERROR_TYPE_Error);

	char assets[MAX_PATH];
	GetFullPath(".\\Assets\\Applications", assets);
	Verify(CreateDirectoryIfNotExists(assets), "Unable to create applications asset folder", ERROR_TYPE_Warning);

	CombinePath(assets, assets, pName.c_str());
	Verify(CreateDirectoryIfNotExists(assets), "Unable to create applications asset folder", ERROR_TYPE_Warning);

	CombinePath(assets, assets, "boxart.jpg");
	Verify(CopyFileTo(pImage.c_str(), assets), "Unable to copy application image", ERROR_TYPE_Warning);
}

static void GetAllApplications(YaffeState* pState)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetAllApplications);

	Platform* current = pState->emulators.GetItem(pState->emulators.count++);
	current->type = APPLICATION_App;
	current->platform = -1;
	strcpy(current->name, "Applications");

	//TODO don't hardcode
	//TODO i could add an application per one, but one display one?
	current->files.InitializeWithArray(new Executable[128], 128);

	while (ExecuteReader(stmt))
	{
		//Executable* exe = current->files.GetItem(current->files.count++);

		//strcpy(exe->name, GetTextColumn(stmt, 0));
		////TODO start args???

		//char rom_asset_path[MAX_PATH];
		//GetAssetPath(rom_asset_path, current, exe);

		//CombinePath(exe->boxart.load_path, rom_asset_path, "boxart.jpg");
		//exe->boxart.type = ASSET_TYPE_Bitmap;
	}
	std::sort(current->files.items, current->files.items + current->files.count, RomsSort);
}







static void WriteGameToDB(GameInfo* pInfo, std::string pOld)
{
	std::lock_guard<std::mutex> guard(db_mutex);
	DatabaseConnection con;
	{
		SqlStatement stmt(&con, qs_AddGame);
		BindIntParm(stmt, pInfo->id);
		BindIntParm(stmt, pInfo->platform);
		BindTextParm(stmt, pInfo->name.c_str());
		BindTextParm(stmt, pInfo->overview.c_str());
		BindIntParm(stmt, pInfo->players);
		BindTextParm(stmt, pOld.c_str());
		Verify(ExecuteUpdate(stmt), "Unable to update additional game information", ERROR_TYPE_Warning);
	}
	
	std::string url_base = "https://cdn.thegamesdb.net/images/medium/";
	if(!pInfo->boxart.empty())
	{
		DownloadImage((url_base + pInfo->boxart).c_str(), pInfo->boxart_load);
	}

	if(!pInfo->banner.empty())
	{
		DownloadImage((url_base + pInfo->banner).c_str(), pInfo->banner_load);
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

	json request = CreateServiceMessage(MESSAGE_TYPE_Game, work->name.c_str(), work->platform);
	json response;
	Verify(SendServiceMessage(g_state.service, request, &response), "Unable to communicate to YaffeService", ERROR_TYPE_Error);

	u32 count = response["count"].get<u32>();
	json games = response["games"];
	std::vector<GameInfo> items;
	for (u32 i = 0; i < count; i++)
	{
		json game = games.at(i);
		GameInfo pi;
		pi.name = game["name"].get<std::string>();
		pi.id = game["id"].get<s32>();
		pi.overview = game["overview"].get<std::string>();
		pi.players = game["players"].get<s32>();
		pi.platform = work->platform;
		pi.banner = game["banner"].get<std::string>();
		pi.boxart = game["boxart"].get<std::string>();
		pi.banner_load = work->banner;
		pi.boxart_load = work->boxart;
		items.push_back(pi);
	}

	if (count > 1)
	{
		char title[200];
		sprintf(title, "Found %d results for game '%s'", count, work->name.c_str());
		DisplayModalWindow(&g_state, "Select Game", new ListModal<GameInfo>(items, work->name.c_str(), title), BITMAP_None, WriteGameToDB);
	}
	else if(count == 1)
	{
		WriteGameToDB(&items[0], (char*)work->name.c_str());
	}
	
	delete work;
}
static void GetGameInfo(Platform* pApp, Executable* pExe)
{
	DatabaseConnection con;
	SqlStatement stmt(&con, qs_GetGame);
	BindIntParm(stmt, pApp->platform);
	BindTextParm(stmt, pExe->name);
	if(ExecuteReader(stmt))
	{
		pExe->overview = GetTextColumn(stmt, 2);
		pExe->players = GetIntColumn(stmt, 3);
	}
	else
	{
		GameInfoWork* work = new GameInfoWork();
		work->name = pExe->name;
		work->banner = pExe->banner.load_path;
		work->boxart = pExe->boxart.load_path;
		work->platform = pApp->platform;
		QueueUserWorkItem(g_state.queue, RetrievePossibleGames, work);
	}
}