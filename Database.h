#pragma once

struct GameInfo
{
	std::string name;
	s32 id;
	s32 players;
	std::string overview;
	std::string banner_url;
	std::string boxart_url;
	Executable* exe;
	s32 platform;
	u8 rating;
	char platform_name[100];
};

struct PlatformInfo
{
	std::string name;
	char* path;
	char* args;
	char* folder;
	char* display_name;
	s32 id;
};

struct GameInfoWork
{
	std::string name;
	Executable* exe;
	s32 platform;
	char platform_name[100];
};

struct PlatformInfoWork
{
	char* name;
	char* path;
	char* args;
	char* folder;
};

struct DatabaseConnection
{
	sqlite3* con;
	DatabaseConnection()
	{
		if (sqlite3_open("Yaffe.db", &con) != SQLITE_OK)
		{
			YaffeLogError("Unable to connect to database: %s", sqlite3_errmsg(con));
			DisplayErrorMessage("Connection to database failed. See logs for details", ERROR_TYPE_Error);
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
	u32 parm_index;
	SqlStatement(DatabaseConnection* pCon, const char* pQuery)
	{
		int prep = sqlite3_prepare_v2(pCon->con, pQuery, -1, &stmt, 0);
		if (prep != SQLITE_OK)
		{
			YaffeLogError("Unable to prepare statement: %d %s", prep, sqlite3_errmsg(pCon->con));
			DisplayErrorMessage("Connection to database failed. See logs for details", ERROR_TYPE_Error);
		}
		parm_index = 1;
	}
	~SqlStatement()
	{
		sqlite3_finalize(stmt);
	}
};

static void GetPlatformInfo(s32 pApplication, char* pPath, char* pArgs, char* pRoms);

const char* qs_GetPlatform = "SELECT Platform, Path, Args, Roms FROM Platforms WHERE ID = @ID";
const char* qs_GetAllPlatforms = "SELECT ID, Platform, Roms FROM Platforms";
const char* qs_AddPlatform = "INSERT INTO Platforms ( ID, Platform, Path, Args, Roms ) VALUES ( @PlatformId, @Platform, @Path, @Args, @Roms )";
const char* qs_UpdatePlatform = "UPDATE Platforms SET Path = @Path, Args = @Args, Roms = @Roms WHERE ID = @ID";

const char* qs_GetGame = "SELECT ID, Name, Overview, Players, Rating, FileName FROM Games WHERE Platform = @Platform AND FileName = @Game";
const char* qs_GetRecentGames = "SELECT g.Name, g.Overview, g.Players, g.Rating, g.FileName, p.ID, p.Platform FROM Games g, Platforms p WHERE g.Platform = p.ID AND LastRun IS NOT NULL ORDER BY LastRun DESC LIMIT " RECENT_COUNT_STRING;
const char* qs_AddGame = "INSERT INTO Games (ID, Platform, Name, Overview, Players, Rating, FileName) VALUES ( @GameId, @Platform, @Name, @Overview, @Players, @Rating, @FileName )";
const char* qs_UpdateGameLastRun = "UPDATE Games SET LastRun = strftime('%s', 'now', 'localtime') WHERE Platform = @Platform AND FileName = @Game";

const char* qs_GetAllApplications = "SELECT ID, Name FROM Applications";
const char* qs_GetApplication = "SELECT Name, Path, Args FROM Applications WHERE ID = @ID";
const char* qs_AddApplication = "INSERT INTO Applications ( ID, Name, Path, Args ) VALUES ( @ID, @Name, @Path, @Args )";
const char* qs_UpdateApplication = "UPDATE Applications Set Path = @Path, Args = @Args WHERE Name = @Name";
const char* qs_GetApplicationID = "SELECT COALESCE(MIN(ID), 0) - 1 from Applications";

const char* qs_CreateApplicationTable = "CREATE TABLE \"Applications\" ( \"ID\" INTEGER, \"Name\" TEXT, \"Path\" TEXT, \"Args\"	TEXT )";
const char* qs_CreateGamesTable = "CREATE TABLE \"Games\" ( \"ID\" INTEGER, \"Platform\" INTEGER, \"Name\" TEXT, \"Overview\" TEXT, \"Players\" INTEGER, \"Rating\" INTEGER, \"FileName\" TEXT, \"LastRun\" INTEGER )";
const char* qs_CreatePlatformsTable = "CREATE TABLE \"Platforms\" ( \"ID\" INTEGER, \"Platform\" TEXT, \"Path\" TEXT, \"Args\" TEXT, \"Roms\" TEXT )";