#include <fstream>
#include <sstream>
#include "Assets.h"

inline static bool FileExists(const char* pPath)
{
	FILE* f = fopen(pPath, "r");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
}

static void SendTextureToGraphicsCard(void* pData)
{
	if (pData)
	{
		Bitmap* bitmap = (Bitmap*)pData;

		glGenTextures(1, &bitmap->texture);
		glBindTexture(GL_TEXTURE_2D, bitmap->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(bitmap->data);
		bitmap->data = nullptr;
	}
}

static void SendFontToGraphicsCard(void* pData)
{
	if (pData)
	{
		FontInfo* font = (FontInfo*)pData;

		glGenTextures(1, &font->texture);
		glBindTexture(GL_TEXTURE_2D, font->texture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, font->atlasWidth, font->atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, font->data);
		glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
		glGenerateMipmap(GL_TEXTURE_2D);

		delete font->data;
		font->data = nullptr;
	}
}

static void AddBitmapAsset(Assets* pAssets, Bitmap* pTexture, TextureAtlas* pAtlas, BITMAPS pName, const char* pPath)
{
	AssetSlot* slot = pAssets->bitmaps + pName;
	slot->type = ASSET_TYPE_Bitmap;
	slot->state = ASSET_STATE_Loaded;

	slot->bitmap = AllocStruct(pAssets, Bitmap);
	memcpy(slot->bitmap, pTexture, sizeof(Bitmap));
	for (u32 i = 0; i < pAtlas->count; i++)
	{
		TextureAtlasEntry e = pAtlas->entries[i];
		if (strcmp(e.file, pPath) == 0)
		{
			slot->bitmap->uv_min = V2(e.u_min, e.v_min);
			slot->bitmap->uv_max = V2(e.u_max, e.v_max);
			return;
		}
	}
}
static void LoadTexturePack(void* pData)
{
	TextureAtlasWork* work = (TextureAtlasWork*)pData;
	if (work->bitmap)
	{
		SendTextureToGraphicsCard(work->bitmap);

		TextureAtlas atlas = ta_ReadTextureAtlas(work->atlas->config);
		for (u32 i = 0; i < atlas.count; i++)
		{
			PackedTextureEntry t = work->atlas->textures[i];
			AddBitmapAsset(g_assets, work->bitmap, &atlas, t.bitmap, t.file);
		}

		ta_DisposeTextureAtlas(&atlas);
	}
	delete work;
	work = nullptr;
}

static Bitmap* LoadBitmapAsset(Assets* pAssets, const char* pPath)
{
	Bitmap* bitmap = AllocStruct(pAssets, Bitmap);
	bitmap->uv_min = V2(0);
	bitmap->uv_max = V2(1);

	bitmap->data = stbi_load(pPath, &bitmap->width, &bitmap->height, &bitmap->channels, STBI_rgb_alpha);
	if (!bitmap->data) return nullptr;

	return bitmap;
}

static FontInfo* LoadFontAsset(Assets* pAssets, const char* pPath, float pSize)
{
	//https://github.com/0xc0dec/demos/blob/master/src/stb-truetype/StbTrueType.cpp
	const u32 firstChar = ' ';
	const u32 charCount = '~' - ' ';
	const u32 ATLAS_SIZE = 1024;

	//Alloc in one big block so we can free in one big block
	void* data = Alloc(pAssets, (sizeof(stbtt_packedchar) * charCount) + sizeof(FontInfo));

	FontInfo* font = (FontInfo*)data;
	font->size = (pSize / 1000) * g_state.form->height;
	font->atlasWidth = ATLAS_SIZE;
	font->atlasHeight = ATLAS_SIZE;
	font->charInfo = (stbtt_packedchar*)((char*)data + sizeof(FontInfo));
	font->data = new u8[ATLAS_SIZE * ATLAS_SIZE];

	std::ifstream file(pPath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		DisplayErrorMessage("Unable to read font file", ERROR_TYPE_Warning);
		return nullptr;
	}
	const auto size = file.tellg();
	file.seekg(0, std::ios::beg);
	u8* bytes = (u8*)Alloc(pAssets, size);
	file.read(reinterpret_cast<char*>(&bytes[0]), size);
	file.close();
	stbtt_InitFont(&font->info, bytes, stbtt_GetFontOffsetForIndex(bytes, 0));

	stbtt_pack_context context;
	if (!stbtt_PackBegin(&context, font->data, font->atlasWidth, font->atlasHeight, 0, 1, nullptr))
	{
		DisplayErrorMessage("Unable to pack font", ERROR_TYPE_Warning);
		return nullptr;
	}

	stbtt_PackSetOversampling(&context, 3, 3);
	if (!stbtt_PackFontRange(&context, bytes, 0, font->size, firstChar, charCount, font->charInfo))
	{
		DisplayErrorMessage("Unable pack font range", ERROR_TYPE_Warning);
		return nullptr;
	}

	stbtt_PackEnd(&context);
	Free(bytes);
	file.close();

	return font;
}

static void ProcessTaskCallbacks(TaskCallbackQueue* pQueue)
{
	u32 array = (u32)(pQueue->i >> 32);
	u64 newArray = ((u64)array ^ 1) << 32;

	u64 event = AtomicExchange(&pQueue->i, newArray);
	u32 count = (u32)event;

	for (u32 i = 0; i < count; i++)
	{
		TaskCallback* cb = &pQueue->callbacks[array][i];
		cb->callback(cb->data);
	}
}

static void AddTaskCallback(TaskCallbackQueue* pQueue, TaskCompleteCallback* pFunc, void* pData)
{
	u64 arrayIndex = AtomicAdd(&pQueue->i, 1);

	u32 array = (u32)(arrayIndex >> 32);
	u32 index = (u32)arrayIndex;
	assert(index < 32);

	TaskCallback cb;
	cb.callback = pFunc;
	cb.data = pData;
	pQueue->callbacks[array][index] = cb;
}

WORK_QUEUE_CALLBACK(LoadAssetBackground)
{
	LoadAssetWork* work = (LoadAssetWork*)pData;

	switch (work->slot->type)
	{
		case ASSET_TYPE_Bitmap:
			work->slot->bitmap = LoadBitmapAsset(work->assets, work->load_info);
			AddTaskCallback(work->queue, SendTextureToGraphicsCard, work->slot->bitmap);
			break;

		case ASSET_TYPE_TexturePack:
		{
			work->slot->bitmap = LoadBitmapAsset(work->assets, work->load_info);

			TextureAtlasWork* ta_work = new TextureAtlasWork();
			ta_work->bitmap = work->slot->bitmap;
			ta_work->atlas = (PackedTexture*)work->data;
			AddTaskCallback(work->queue, LoadTexturePack, ta_work);
		}
		break;
		
		case ASSET_TYPE_Font:
			work->slot->font = LoadFontAsset(work->assets, work->load_info, work->slot->size);
			AddTaskCallback(work->queue, SendFontToGraphicsCard, work->slot->font);
			break;
	}
	AtomicExchange(&work->slot->state, ASSET_STATE_Loaded);
	delete work;
	work = nullptr;
}

static void LoadAsset(Assets* pAssets, AssetSlot* pSlot, void* pData = nullptr)
{
	if (AtomicCompareExchange(&pSlot->state, ASSET_STATE_Queued, ASSET_STATE_Unloaded) == ASSET_STATE_Unloaded)
	{
		YaffeLogInfo("Loading asset %s", pSlot->load_path.c_str());

		LoadAssetWork* work = new LoadAssetWork();
		work->load_info = pSlot->load_path.c_str();
		work->slot = pSlot;
		work->assets = pAssets;
		work->queue = &g_state.callbacks;
		work->data = pData;
		QueueUserWorkItem(g_state.work_queue, LoadAssetBackground, work);
	}
}

static Bitmap* GetBitmap(Assets* pAssets, AssetSlot* pSlot)
{
	if (pSlot)
	{
		pSlot->last_requested = __rdtsc();
		if (pSlot->state == ASSET_STATE_Unloaded && FileExists(pSlot->load_path.c_str()))
		{
			LoadAsset(pAssets, pSlot);
		}
		return pSlot->bitmap;
	}
	return nullptr;
}

static Bitmap* GetBitmap(Assets* pAssets, BITMAPS pAsset)
{
	return GetBitmap(pAssets, pAssets->bitmaps + pAsset);
}

static FontInfo* GetFont(Assets* pAssets, AssetSlot* pSlot)
{
	pSlot->last_requested = __rdtsc();
	if (pSlot->state == ASSET_STATE_Unloaded && FileExists(pSlot->load_path.c_str()))
	{
		LoadAsset(pAssets, pSlot);
	}
	return pSlot->font;
}

static FontInfo* GetFont(Assets* pAssets, FONTS pFont)
{
	return GetFont(pAssets, pAssets->fonts + pFont);
}

static v2 MeasureString(FONTS pFont, const char* pText)
{
	v2 size = {};
	float x = 0;
	FontInfo* font = GetFont(g_assets, pFont);
	if (font)
	{
		size.Height = font->size;
		float y = 0.0F;
		stbtt_aligned_quad quad;
		for (u32 i = 0; pText[i] != 0; i++)
		{
			const unsigned char c = pText[i];
			if (c == '\n')
			{
				size.Height += font->size;
				x = 0;
			}
			else
			{
				assert(c - ' ' >= 0);
				stbtt_GetPackedQuad(font->charInfo, font->atlasWidth, font->atlasHeight, c - ' ', &x, &y, &quad, 1);
				size.Width = std::max(size.Width, x);
			}
		}
	}
	return size;
}

static float CharWidth(FONTS pFont, char pChar)
{
	FontInfo* font = GetFont(g_assets, pFont);
	if (font)
	{
		if (pChar == '\n' || pChar - ' ' < 0)
		{
			return 0;
		}
		else
		{
			stbtt_aligned_quad quad;
			float y = 0.0F;
			float x = 0.0F;

			stbtt_GetPackedQuad(font->charInfo, font->atlasWidth, font->atlasHeight, pChar - ' ', &x, &y, &quad, 1);
			return x;
		}
	}
	return 0;
}

static float GetFontSize(FONTS pFont)
{
	FontInfo* font = GetFont(g_assets, pFont);
	if (font) return font->size;
	return 0;
}

static void WrapText(char* pText, u32 pLength, FONTS pFont, float pWidth)
{ 
	u32 last_index = 0;
	float total_size = 0;
	float word_size = 0;
	for (u32 i = 0; i <= pLength; i++)
	{
		if (pText[i] == '\n')
		{
			total_size = 0;
			last_index = i;
		}
		else if (i == pLength || (pText[i] > 0 && isspace(pText[i]))) 
		{
			word_size = 0;
			for (u32 j = last_index; j < i; j++)
			{
				word_size += CharWidth(pFont, pText[j]);
			}

			total_size += word_size;
			if (total_size > pWidth)
			{
				pText[last_index] = '\n';
				total_size = word_size;
			}
			last_index = i;
		}
	}
}

static void AddBitmapAsset(Assets* pAssets, BITMAPS pName, const char* pPath)
{
	AssetSlot* slot = pAssets->bitmaps + pName;
	slot->type = ASSET_TYPE_Bitmap;
	slot->load_path = pPath;
}

static void AddFontAsset(Assets* pAssets, FONTS pName, const char* pPath, float pSize)
{
	AssetSlot* slot = pAssets->fonts + pName;
	slot->type = ASSET_TYPE_Font;
	slot->load_path = pPath;
	slot->size = pSize;
}

static void LoadPackedTexture(Assets* pAssets, BITMAPS pName, PackedTexture* pAtlas)
{
	AssetSlot* slot = pAssets->bitmaps + pName;
	slot->type = ASSET_TYPE_TexturePack;
	slot->load_path = pAtlas->image;

	LoadAsset(pAssets, slot, pAtlas);
}

static bool LoadAssets(void* pStack, u64 pSize, Assets** pAssets)
{
	char assets_path[MAX_PATH];
	GetFullPath("./Assets/", assets_path);
	if (!CreateDirectoryIfNotExists(assets_path))
	{
		YaffeLogError("Unable to create assets directory");
		return false;
	}
	YaffeLogInfo("Asset path %s", assets_path);

	Assets* assets = new Assets();
	*pAssets = assets;
	assets->memory = CreateMemoryPool(pStack, pSize);
	
	AddFontAsset(assets, FONT_Subtext, "./Assets/Roboto-Regular.ttf", 20);
	AddFontAsset(assets, FONT_Normal, "./Assets/Roboto-Regular.ttf", 25);
	AddFontAsset(assets, FONT_Title, "./Assets/Roboto-Black.ttf", 36);
	AddBitmapAsset(assets, BITMAP_Background, "./Assets/background.jpg");
	AddBitmapAsset(assets, BITMAP_Placeholder, "./Assets/placeholder.png");
	AddBitmapAsset(assets, BITMAP_PlaceholderBanner, "./Assets/banner.png");

	PackedTexture* text = new PackedTexture();
	text->textures[0] = { BITMAP_Error, "error.png" };
	text->textures[1] = { BITMAP_Question, "question.png" };
	text->textures[2] = { BITMAP_ArrowUp, "arrow_up.png" };
	text->textures[3] = { BITMAP_ArrowDown, "arrow_down.png" };
	text->textures[4] = { BITMAP_ButtonA, "button_a.png" };
	text->textures[5] = { BITMAP_ButtonB, "button_b.png" };
	text->textures[6] = { BITMAP_ButtonX, "button_x.png" };
	text->textures[7] = { BITMAP_ButtonY, "button_y.png" };
	text->textures[8] = { BITMAP_App, "apps.png" };
	text->textures[9] = { BITMAP_Emulator, "emulator.png" };
	text->textures[10] = { BITMAP_Recent, "recents.png" };
	text->textures[11] = { BITMAP_Speaker, "speaker.png" };
	text->textures[12] = { BITMAP_Settings, "settings.png" };
	text->textures[13] = { BITMAP_ERSB_Everyone, "everyone.png" };
	text->textures[14] = { BITMAP_ERSB_Everyone10, "everyone10.png" };
	text->textures[15] = { BITMAP_ERSB_Teen, "teen.png" };
	text->textures[16] = { BITMAP_ERSB_Mature, "mature.png" };
	text->textures[17] = { BITMAP_ERSB_AdultOnly, "adults.png" };
	text->config = "./Assets/atlas.tex";
	text->image = "./Assets/packed.png";
	LoadPackedTexture(assets, BITMAP_IconPack, text);

	//Put a 1px white texture so we get fully lit instead of only ambient lighting
	u8 data[] = { 255, 255, 255, 255 };
	glGenTextures(1, &assets->blank_texture);
	glBindTexture(GL_TEXTURE_2D, assets->blank_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	return true;
}

void FreeAsset(AssetSlot* pSlot)
{
	if (pSlot->state == ASSET_STATE_Loaded)
	{
		Free(pSlot->data);
		switch (pSlot->type)
		{
			case ASSET_TYPE_Bitmap:
			case ASSET_TYPE_TexturePack:
				glDeleteTextures(1, &pSlot->bitmap->texture);
				break;

			case ASSET_TYPE_Font:
				glDeleteTextures(1, &pSlot->font->texture);
				break;

			default:
				assert(false);
		}
		pSlot->last_requested = 0;
		AtomicExchange(&pSlot->state, ASSET_STATE_Unloaded);
	}
}

void FreeAllAssets(Assets* pAssets)
{
	for (u32 i = 0; i < BITMAP_COUNT; i++)
		FreeAsset(pAssets->bitmaps + i);
	for (u32 i = 0; i < FONT_COUNT; i++)
		FreeAsset(pAssets->fonts + i);

	for (auto it = pAssets->display_images.begin(); it != pAssets->display_images.end(); it++)
	{
		FreeAsset(&it->second);
	}
}

void EvictOldestAsset(Assets* pAssets)
{
	AssetSlot* slot = nullptr;
	u64 request = __rdtsc();

	//Don't bother looking through pAssets->bitmaps
	//Those are very small and UI elements so they are frequency requested

	for (auto it = pAssets->display_images.begin(); it != pAssets->display_images.end(); it++)
	{
		AssetSlot* asset = &it->second;
		if (asset->state == ASSET_STATE_Loaded && asset->last_requested < request)
		{
			request = asset->last_requested;
			slot = asset;
		}
	}

	pAssets->display_images.erase(std::string(slot->load_path));
	FreeAsset(slot);
}


static void SetAssetPaths(const char* pPlatName, Executable* pExe, AssetSlot** pBanner, AssetSlot** pBoxart)
{
	char rom_asset_path[MAX_PATH];
	GetFullPath(".\\Assets", rom_asset_path);
	CombinePath(rom_asset_path, rom_asset_path, pPlatName);
	CreateDirectoryIfNotExists(rom_asset_path);

	CombinePath(rom_asset_path, rom_asset_path, pExe->display_name);
	CreateDirectoryIfNotExists(rom_asset_path);

	//Check to see if the image has already been loaded and use that.
	//This allows us to call SetAssetPaths without having to worry 
	//if we have already called it for that image
	char* boxart = new char[MAX_PATH];
	CombinePath(boxart, rom_asset_path, "boxart.jpg");
	std::string boxart_string = std::string(boxart);
	auto find = g_assets->display_images.find(boxart_string);
	if (find == g_assets->display_images.end())
	{
		AssetSlot slot = {};
		slot.type = ASSET_TYPE_Bitmap;
		slot.load_path = boxart_string;
		g_assets->display_images.emplace(std::make_pair(boxart_string, slot));
		find = g_assets->display_images.find(boxart_string);
	}
	assert(find != g_assets->display_images.end());
	*pBoxart = &find->second;

	//Same thing for banners
	char* banner = new char[MAX_PATH];
	CombinePath(banner, rom_asset_path, "banner.jpg");
	std::string banner_string = std::string(banner);
	find = g_assets->display_images.find(banner_string);
	if (find == g_assets->display_images.end())
	{
		AssetSlot slot = {};
		slot.type = ASSET_TYPE_Bitmap;
		slot.load_path = banner_string;
		g_assets->display_images.emplace(std::make_pair(banner_string, slot));
		find = g_assets->display_images.find(banner_string);
	}
	assert(find != g_assets->display_images.end());
	*pBanner = &find->second;

	YaffeLogInfo("Asset paths for %s: Banner - %s, Boxart - %s", pExe->display_name, banner_string.c_str(), boxart_string.c_str());
}
