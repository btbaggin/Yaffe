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
	char file[100]; //TODO can I get rid of this
	char display_name[80];
	std::string overview;
	AssetSlot* boxart;
	AssetSlot* banner;
	u8 players;
	s32 platform; //Duplicated from Platform so we always know it, even if launching from recents

	u8 flags;
	v2 position;
	char command_line[600];
};

//struct ExecutableDisplay
//{
//	v2 position;
//	u8 flags;
//};

struct Platform
{
	char name[35];
	PLATFORM_TYPE type;
	s32 id;

	List<Executable> files;
	//List<ExecutableDisplay> file_display;
};


static inline Platform* GetSelectedPlatform();
static void RefreshExecutables(YaffeState* pState, Platform* pEmulator);
static void BuildCommandLine(Executable* pExe, const char* pPath, const char* pArgs);
static void BuildCommandLine(Executable* pExe, const char* pEmuPath, const char* pPath, const char* pArgs);
bool RomsSort(Executable a, Executable b) { return strcmp(a.display_name, b.display_name) < 0; }
