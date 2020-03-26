#pragma once

enum ROM_DISPLAY_FLAGS : u8
{
	ROM_DISPLAY_None,
	ROM_DISPLAY_Filtered,
};

struct Rom
{
	char name[100];
	char path[MAX_PATH];
	AssetSlot boxart;
	AssetSlot banner;
	u8 players;

	u8 flags;
	v2 position;
};

struct Emulator
{
	char display_name[35];
	char start_path[MAX_PATH];
	char rom_path[MAX_PATH];
	char asset_path[MAX_PATH];
	s32 platform;

	List<Rom> roms;
};

struct RomAssetWork
{
	std::string name;
	std::string banner_loadpath;
	std::string boxart_loadpath;
	s32 platform;
};

static inline Emulator* GetSelectedEmulator();
static void GetRoms(YaffeState* pState, Emulator* pEmulator, bool pForce = false);
static void QueueAssetDownloads(Rom* pRom, const char* pAssetPath, s32 pPlatform);
