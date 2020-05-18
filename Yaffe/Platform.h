#pragma once

enum EXECUTABLE_FLAGS : u8
{
	EXECUTABLE_FLAG_None = 0,
	EXECUTABLE_FLAG_Filtered = 1,
};

enum PLATFORM_TYPE : u8
{
	PLATFORM_Invalid,

	PLATFORM_Recents,
	PLATFORM_Emulator,
	PLATFORM_App,
};

struct Executable
{
	char file[100];
	char display_name[100];
	char command_line[600];
	std::string overview;
	AssetSlot* boxart;
	AssetSlot* banner;
	u8 players;

	u8 flags;
	v2 position;
};

struct Platform
{
	char name[35];
	PLATFORM_TYPE type;
	s32 id;

	List<Executable> files;
};


static inline Platform* GetSelectedApplication();
static void RefreshExecutables(YaffeState* pState, Platform* pEmulator);
static void BuildCommandLine(Executable* pExe, const char* pPath, const char* pArgs);
static void BuildCommandLine(Executable* pExe, const char* pEmuPath, const char* pPath, const char* pArgs);
bool RomsSort(Executable a, Executable b) { return strcmp(a.file, b.file) < 0; }
