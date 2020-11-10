#pragma once

const u32 RECENT_COUNT = 10;
#define RECENT_COUNT_STRING "10"

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

//Information about the executable
struct Executable
{
	char file[100];
	char display_name[80];
	std::string overview;
	s32 platform; //Duplicated from Platform so we always know it, even if launching from recents
	u8 players;
	bool invalid;
	AssetSlot* boxart;
	AssetSlot* banner;
};

struct Platform
{
	char name[35];
	char path[MAX_PATH];
	PLATFORM_TYPE type;
	s32 id;

	List<Executable> files;
};


static inline Platform* GetSelectedPlatform();
static void RefreshExecutables(YaffeState* pState, Platform* pEmulator);
static void BuildCommandLine(Executable* pExe, const char* pEmuPath, const char* pPath, const char* pArgs);
bool RomsSort(Executable a, Executable b) { return strcmp(a.display_name, b.display_name) < 0; }
