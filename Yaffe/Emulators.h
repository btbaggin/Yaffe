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
	char path[MAX_PATH];
	AssetSlot boxart;
	AssetSlot banner;
	s32 platform;
	u8 players;

	u8 flags;
	v2 position;
};

struct Application
{
	char display_name[35];
	char start_path[MAX_PATH];
	char start_args[100];
	char rom_path[MAX_PATH];
	char asset_path[MAX_PATH];
	APPLICATION_TYPE type;

	List<Executable> files;
};

struct RomAssetWork
{
	std::string name;
	std::string banner_loadpath;
	std::string boxart_loadpath;
	s32 platform;
};

static inline Application* GetSelectedApplication();
static void GetExecutables(YaffeState* pState, Application* pEmulator, bool pForce = false);
static void QueueAssetDownloads(Executable* pRom, const char* pAssetPath, s32 pPlatform);
