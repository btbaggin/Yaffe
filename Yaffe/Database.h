#pragma once

struct GameInfo
{
	std::string name;
	s32 id;
	s32 players;
	std::string overview;
	std::string banner;
	std::string boxart;
	Executable* exe;
	s32 platform;
	char platform_name[100];
};

struct PlatformInfo
{
	std::string name;
	std::string path;
	std::string args;
	std::string folder;
	std::string display_name;
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
	std::string name;
	std::string path;
	std::string args;
	std::string folder;
};

struct DatabaseConnection
{
	sqlite3* con;
	DatabaseConnection()
	{
		Verify(sqlite3_open("Yaffe.db", &con) == SQLITE_OK, "Unable to connect to games database", ERROR_TYPE_Error);
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
		Verify(prep == SQLITE_OK, "Unable to prepare statement", ERROR_TYPE_Error);
		parm_index = 1;
	}
	~SqlStatement()
	{
		sqlite3_finalize(stmt);
	}
};

const char* qs_GetPlatform = "SELECT Path, Args, Roms FROM Platforms WHERE ID = @ID";
const char* qs_GetAllPlatforms = "SELECT ID, Platform FROM Platforms";
const char* qs_AddPlatform = "INSERT INTO Platforms ( ID, Platform, Path, Args, Roms ) VALUES ( @PlatformId, @Platform, @Path, @Args, @Roms )";
const char* qs_UpdatePlatform = "UPDATE Platforms SET Path = @Path, Args = @Args, Roms = @Roms WHERE ID = @ID";

const char* qs_GetGame = "SELECT ID, Name, Overview, Players, FileName FROM Games WHERE Platform = @Platform AND FileName = @Game";
const char* qs_GetRecentGames = "SELECT g.Name, g.Overview, g.Players, g.FileName, p.Path, p.Args, p.Roms, p.Platform FROM Games g, Platforms p WHERE g.Platform = p.ID AND LastRun IS NOT NULL ORDER BY LastRun DESC LIMIT 10";
const char* qs_AddGame = "INSERT INTO Games (ID, Platform, Name, Overview, Players, FileName) VALUES ( @GameId, @Platform, @Name, @Overview, @Players, @FileName )";
const char* qs_UpdateGameLastRun = "UPDATE Games SET LastRun = strftime('%s', 'now', 'localtime') WHERE Platform = @Platform AND FileName = @Game";

const char* qs_GetAllApplications = "SELECT Name, Path, Args FROM Applications";
const char* qs_AddApplication = "INSERT INTO Applications ( Name, Path, Args ) VALUES ( @Name, @Path, @Args )";
const char* qs_UpdateApplication = "UPDATE Applications Set Path = @Path, Args = @Args WHERE Name = @Name";

const char* qs_CreateApplicationTable = "CREATE TABLE \"Applications\" ( \"Name\" TEXT, \"Path\" TEXT, \"Args\"	TEXT )";
const char* qs_CreateGamesTable = "CREATE TABLE \"Games\" ( \"ID\" INTEGER, \"Platform\" INTEGER, \"Name\" TEXT, \"Overview\" TEXT, \"Players\" INTEGER, \"FileName\" TEXT, \"LastRun\" INTEGER )";
const char* qs_CreatePlatformsTable = "CREATE TABLE \"Platforms\" ( \"ID\" INTEGER, \"Platform\" TEXT, \"Path\" TEXT, \"Args\" TEXT, \"Roms\" TEXT )";