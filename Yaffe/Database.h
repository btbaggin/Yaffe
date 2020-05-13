#pragma once

#include "Queries.h"

std::mutex db_mutex;//TODO make better
struct GameInfo
{
	std::string name;
	s32 id;
	s32 players;
	std::string overview;
	std::string banner;
	std::string boxart;
	std::string banner_load;
	std::string boxart_load;
	s32 platform;
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
	std::string asset_path;
	std::string banner;
	std::string boxart;
	s32 platform;
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