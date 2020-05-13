#pragma once

enum EXECUTABLE_FLAGS : u8
{
	EXECUTABLE_FLAG_None = 0,
	EXECUTABLE_FLAG_Filtered = 1,
};

enum APPLICATION_TYPE : u8
{
	APPLICATION_Emulator,
	APPLICATION_App,
};

struct Executable
{
	char name[100];
	char command_line[1000];
	std::string overview;
	AssetSlot boxart;
	AssetSlot banner;
	s32 platform;
	u8 players;

	u8 flags;
	v2 position;
};

struct Platform
{
	char name[35];
	APPLICATION_TYPE type;
	s32 platform;

	List<Executable> files;
};


static inline Platform* GetSelectedApplication();
static void RefreshExecutables(YaffeState* pState, Platform* pEmulator);
static void GetConfiguredEmulators(YaffeState* pState);
bool RomsSort(Executable a, Executable b) { return strcmp(a.name, b.name) < 0; }

static void GetAssetPath(char* pPath, Platform* pApp, Executable* pExe)
{
	GetFullPath(".\\Assets", pPath);
	CombinePath(pPath, pPath, pApp->name);
	CreateDirectoryIfNotExists(pPath);

	CombinePath(pPath, pPath, pExe->name);
	CreateDirectoryIfNotExists(pPath);
}
