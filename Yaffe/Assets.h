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
	ASSET_TYPE_TexturePack,
	ASSET_TYPE_Font,
};

enum BITMAPS : u8
{
	//Unpacked textures go here
	BITMAP_None,
	BITMAP_Background,
	BITMAP_Placeholder,
	BITMAP_TexturePack,

	//Packed textures go here
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
	BITMAP_Recent,
	BITMAP_Speaker,
	BITMAP_Settings,

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
	v2 uv_min;
	v2 uv_max;
	u8* data;
};

struct AssetSlot
{
	u32 state;
	u64 last_requested;
	union 
	{
		Bitmap* bitmap;
		FontInfo* font;
	};
	float size;

	ASSET_TYPES type;
	std::string load_path;
};

struct Assets
{
	AssetSlot fonts[FONT_COUNT];
	AssetSlot bitmaps[BITMAP_COUNT];
	u32 blank_texture;

	std::map<std::string, AssetSlot> display_images;

	MemoryPool* memory;
};

struct LoadAssetWork
{
	Assets* assets;
	TaskCallbackQueue* queue;
	AssetSlot* slot;
	const char* load_info;
	void* data;
};

struct PackedTextureEntry
{
	BITMAPS bitmap;
	const char* file;
};

struct PackedTexture
{
	PackedTextureEntry textures[BITMAP_COUNT - BITMAP_TexturePack - 1];

	const char* image;
	const char* config;
};

struct TextureAtlasWork
{
	Bitmap* bitmap;
	PackedTexture* atlas;
};


void EvictOldestAsset(Assets* pAssets);