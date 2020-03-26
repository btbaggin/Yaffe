
#include "Modal.cpp"
#include "SelectorModal.cpp"

enum SQLITE_TYPES
{
	SQLITE_TYPE_Int,
	SQLITE_TYPE_Text,
};

struct SqliteParameter
{
	SQLITE_TYPES type;
	void* data;
};

struct DatabaseConnection
{
	sqlite3* con;
	DatabaseConnection()
	{
		if (sqlite3_open("Yaffe.db", &con) != SQLITE_OK)
		{
			DisplayErrorMessage("Unable to connect to games database", ERROR_TYPE_Error);
		}
	}
	~DatabaseConnection()
	{
		int rc = sqlite3_close(con);
		assert(rc == SQLITE_OK);
	}
};

struct SqlStatement
{
	sqlite3_stmt* stmt;
	SqlStatement(DatabaseConnection* pCon, const char* pQuery)
	{
		if (sqlite3_prepare_v2(pCon->con, pQuery, -1, &stmt, 0) != SQLITE_OK)
		{
			DisplayErrorMessage("Unable to prepare statement", ERROR_TYPE_Error);
		}
	}
	~SqlStatement()
	{
		sqlite3_finalize(stmt);
	}
};

inline static bool ExecuteReader(SqlStatement* pStmt)
{
	return sqlite3_step(pStmt->stmt) == SQLITE_ROW;
}

inline static bool ExecuteUpdate(SqlStatement* pStmt)
{
	return sqlite3_step(pStmt->stmt) == SQLITE_DONE;
}

inline static const char* GetTextColumn(SqlStatement* pStmt, int pColumn)
{
	return (const char*)sqlite3_column_text(pStmt->stmt, pColumn);
}

inline static int GetIntColumn(SqlStatement* pStmt, int pColumn)
{
	return sqlite3_column_int(pStmt->stmt, pColumn);
}

static void _BindParameters(SqlStatement* pStatement, SqliteParameter* pParameters, u32 pParameterCount)
{
	for (u32 i = 0; i < pParameterCount; i++)
	{
		int index = i + 1;
		SqliteParameter* parm = pParameters + i;
		switch (parm->type)
		{
		case SQLITE_TYPE_Int:
		{
			int* value = (int*)parm->data;
			sqlite3_bind_int(pStatement->stmt, index, *value);
		}
		break;

		case SQLITE_TYPE_Text:
		{
			char* value = (char*)parm->data;
			sqlite3_bind_text(pStatement->stmt, index, value, -1, nullptr);
		}
		break;
		}
	}
}
#define BindParameters(pStatement, pParameters) _BindParameters(pStatement, pParameters, ArrayCount(pParameters))

#pragma comment(lib, "urlmon.lib")
#include <urlmon.h>
static void DownloadImage(const char* pUrl, std::string pSlot)
{
	IStream* stream;
	//Also works with https URL's - unsure about the extent of SSL support though.
	HRESULT result = URLOpenBlockingStreamA(0, pUrl, &stream, 0, 0);
	if (result != 0)
	{
		DisplayErrorMessage("Unable to retrieve image", ERROR_TYPE_Warning);
		return;
	}

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

MODAL_CLOSE(WritePlatformAliasToDB)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		SelectorModal* content = (SelectorModal*)pContent;
		std::string alias = content->GetSelected();

		SqliteParameter parm[2] = {
			{SQLITE_TYPE_Text, (char*)content->data},
			{SQLITE_TYPE_Text, (char*)alias.c_str()},
		};

		{
			DatabaseConnection con;
			SqlStatement stmt(&con, "UPDATE Platforms SET Alias = @Alias WHERE Platform = @Platform");
			BindParameters(&stmt, parm);

			if (!ExecuteUpdate(&stmt))
			{
				DisplayErrorMessage("Unable to update platform alias", ERROR_TYPE_Warning);
			}
		}

		GetRoms(&g_state, GetSelectedEmulator(), true);
	}
}

static void GetListOfPossibleEmulators(DatabaseConnection* pCon, char* pName)
{
	std::string name(pName);
	name = "%" + name + "%";
	SqliteParameter parm[1] = {
		{SQLITE_TYPE_Text, (char*)name.c_str()},
	};

	std::vector<std::string> items;
	SqlStatement stmt(pCon, "SELECT ID, Platform  FROM Platforms where Platform LIKE @Platform");
	BindParameters(&stmt, parm);
	while (ExecuteReader(&stmt))
	{
		items.push_back(std::string(GetTextColumn(&stmt, 1)));
	}

	if (items.size() > 0)
	{
		DisplayModalWindow(&g_state, new SelectorModal(items, pName), BITMAP_None, WritePlatformAliasToDB);
	}
}

static bool GetEmulatorPlatform(Emulator* pEmulator)
{
	DatabaseConnection con;
	SqliteParameter parm[1] = {
		{SQLITE_TYPE_Text, pEmulator->display_name},
	};

	SqlStatement stmt(&con, "SELECT ID FROM Platforms where Platform = @Platform OR Alias = @Platform");
	BindParameters(&stmt, parm);
	if (!ExecuteReader(&stmt))
	{
		GetListOfPossibleEmulators(&con, pEmulator->display_name);
		return false;
	}
	pEmulator->platform = GetIntColumn(&stmt, 0);
	return true;
}

struct GameInfo
{
	std::string name;
	s32 platform;
};
MODAL_CLOSE(WriteGameAliasToDB)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		SelectorModal* content = (SelectorModal*)pContent;
		std::string alias = content->GetSelected();

		GameInfo* gi = (GameInfo*)content->data;
		SqliteParameter parm[3] = {
			{SQLITE_TYPE_Text, (char*)gi->name.c_str()},
			{SQLITE_TYPE_Text, (char*)alias.c_str()},
			{SQLITE_TYPE_Int, &gi->platform},
		};

		{
			DatabaseConnection con;
			SqlStatement stmt(&con, "UPDATE Games SET Alias = @Alias WHERE Game = @Game AND Platform = @Platform");
			BindParameters(&stmt, parm);

			if (!ExecuteUpdate(&stmt))
			{
				DisplayErrorMessage("Unable to update platform alias", ERROR_TYPE_Warning);
			}
		}

		Emulator* e = GetSelectedEmulator();
		for (u32 i = 0; i < e->roms.count; i++)
		{
			Rom* r = e->roms.GetItem(i);
			if (strcmp(r->name, gi->name.c_str()) == 0)
			{
				char rom_asset_path[MAX_PATH];
				sprintf(rom_asset_path, "%s\\%s", e->asset_path, r->name);

				QueueAssetDownloads(r, rom_asset_path, gi->platform);
				break;
			}
		}

		delete gi;
	}
}

static void GetListOfPossibleGames(DatabaseConnection* pCon, s32 pPlatform, std::string pGame)
{
	std::string game = "%" + pGame + "%";
	SqliteParameter parm[2] = {
		{SQLITE_TYPE_Text, (char*)game.c_str()},
		{SQLITE_TYPE_Int, &pPlatform},
	};

	std::vector<std::string> items;
	SqlStatement stmt(pCon, "SELECT Game FROM Games where Game LIKE @Game AND Platform = @Platform");
	BindParameters(&stmt, parm);
	while (ExecuteReader(&stmt))
	{
		items.push_back(std::string(GetTextColumn(&stmt, 0)));
	}

	if (items.size() > 0)
	{
		GameInfo* gi = new GameInfo();
		gi->name = pGame;
		gi->platform = pPlatform;
		DisplayModalWindow(&g_state, new SelectorModal(items, gi), BITMAP_None, WriteGameAliasToDB);
	}
}

static void GetGameImages(s32 pPlatform, std::string pGame, std::string pBanner, std::string pBoxart)
{
	DatabaseConnection con;
	std::string url_base;
	{
		SqlStatement stmt(&con, "SELECT URL FROM ImageUrls WHERE Size = 'medium'");
		if (!ExecuteReader(&stmt))
		{
			DisplayErrorMessage("Unable to get base image URL", ERROR_TYPE_Warning);
			return;
		}
		url_base = std::string(GetTextColumn(&stmt, 0));
	}
	

	SqliteParameter parm[2] = {
		{SQLITE_TYPE_Text, (char*)pGame.c_str()},
		{SQLITE_TYPE_Int, &pPlatform},
	};
	SqlStatement stmt(&con, "SELECT i.Type, i.Side, i.File FROM Games g, Images i WHERE i.GameID = g.ID AND (g.Game = @Game OR g.Alias = @Game) AND g.Platform = @Platform");
	BindParameters(&stmt, parm);
	if (ExecuteReader(&stmt))
	{
		do
		{
			const char* type = GetTextColumn(&stmt, 0);
			const char* side = GetTextColumn(&stmt, 1);
			std::string file = std::string(GetTextColumn(&stmt, 2));

			if (strcmp(type, "banner") == 0)
			{
				DownloadImage((url_base + file).c_str(), pBanner);
			}
			else if (strcmp(type, "boxart") == 0 &&
				strcmp(side, "front") == 0)
			{
				DownloadImage((url_base + file).c_str(), pBoxart);
			}
		} while (ExecuteReader(&stmt));
	}
	else
	{
		GetListOfPossibleGames(&con, pPlatform, pGame);
	}
}

static std::string GetGameInformation(s32 pPlatform, char* pGame)
{
	DatabaseConnection con;
	SqliteParameter parm[2] = {
		{SQLITE_TYPE_Text, pGame},
		{SQLITE_TYPE_Int, &pPlatform},
	};

	SqlStatement stmt(&con, "SELECT Overview FROM Games WHERE Game = @Game AND Platform = @Platform");
	BindParameters(&stmt, parm);
	if (ExecuteReader(&stmt))
	{
		return std::string(GetTextColumn(&stmt, 0));
	}

	return nullptr;
}

static u32 GetGamePlayers(s32 pPlatform, char* pGame)
{
	DatabaseConnection con;
	SqliteParameter parm[2] = {
		{SQLITE_TYPE_Text, pGame},
		{SQLITE_TYPE_Int, &pPlatform},
	};

	SqlStatement stmt(&con, "SELECT Players FROM Games WHERE Game = @Game AND Platform = @Platform");
	BindParameters(&stmt, parm);
	if (ExecuteReader(&stmt))
	{
		return GetIntColumn(&stmt, 0);
	}

	return 1;
}