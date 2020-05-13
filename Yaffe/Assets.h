#pragma once
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb\stb_truetype.h"
#include "typedefs.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_GIF
#include "stb\stb_image.h"

enum ASSET_STATE
{
	ASSET_STATE_Unloaded,
	ASSET_STATE_Queued,
	ASSET_STATE_Loaded
};

enum ASSET_TYPES : u8
{
	ASSET_TYPE_Bitmap,
	ASSET_TYPE_Font,
};

enum BITMAPS : u8
{
	BITMAP_None,
	BITMAP_Background,
	BITMAP_Placeholder,
	BITMAP_Error,
	BITMAP_Question,
	BITMAP_ArrowUp,
	BITMAP_ArrowDown,
	BITMAP_ButtonA,
	BITMAP_ButtonB,
	BITMAP_ButtonX,
	BITMAP_ButtonY,
	BITMAP_App,
	BITMAP_Emulator,

	BITMAP_COUNT
};

enum FONTS : u8
{
	FONT_Subtext,
	FONT_Normal,
	FONT_Title,

	FONT_COUNT
};

struct FontInfo
{
	float size;
	u32 atlasWidth;
	u32 atlasHeight;
	stbtt_packedchar* charInfo;
	stbtt_fontinfo info;
	u8* data;
	u32 texture;
};

struct Bitmap
{
	s32 width;
	s32 height;
	s32 channels;
	u32 texture;
	u8* data;
};

struct AssetSlot
{
	u32 state;
	u64 last_requested;
	ASSET_TYPES type;
	union 
	{
		Bitmap* bitmap;
		FontInfo* font;
	};
	char load_path[MAX_PATH];
	float size;
};

struct Assets
{
	AssetSlot fonts[FONT_COUNT];
	AssetSlot bitmaps[BITMAP_COUNT];
	u32 blank_texture;

	MemoryPool* memory;
};

struct LoadAssetWork
{
	Assets* assets;
	TaskCallbackQueue* queue;
	AssetSlot* slot;
	const char* load_info;
};

void EvictOldestAsset(AssetSlot* pAssets, u32 pAssetCount);