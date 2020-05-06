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
		Bitmap* b = (Bitmap*)pData;

		GLenum format;
		if (b->channels == 1) format = GL_RED;
		else if (b->channels == 3) format = GL_RGB;
		else if (b->channels == 4) format = GL_RGBA;

		glGenTextures(1, &b->texture);

		glBindTexture(GL_TEXTURE_2D, b->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, format, b->width, b->height, 0, format, GL_UNSIGNED_BYTE, b->data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(b->data);
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
	}
}

static Bitmap* LoadBitmapAsset(Assets* pAssets, const char* pPath)
{
	Bitmap* bitmap = AllocStruct(pAssets->memory, Bitmap);

	bitmap->data = stbi_load(pPath, &bitmap->width, &bitmap->height, &bitmap->channels, 0);
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
	void* data = Alloc(pAssets->memory, (ATLAS_SIZE * ATLAS_SIZE) + (sizeof(stbtt_packedchar) * charCount) + sizeof(FontInfo));

	FontInfo* font = (FontInfo*)data;
	font->size = (pSize / 1000) * g_state.form->height;
	font->atlasWidth = ATLAS_SIZE;
	font->atlasHeight = ATLAS_SIZE;
	font->charInfo = (stbtt_packedchar*)((char*)data + sizeof(FontInfo));
	font->data = (u8*)((char*)font->charInfo + (sizeof(stbtt_packedchar) * charCount));

	std::ifstream file(pPath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		DisplayErrorMessage("Unable to read font file", ERROR_TYPE_Warning);
		return nullptr;
	}
	const auto size = file.tellg();
	file.seekg(0, std::ios::beg);
	u8* bytes = (u8*)Alloc(pAssets->memory, size);
	file.read(reinterpret_cast<char*>(&bytes[0]), size);
	file.close();
	stbtt_InitFont(&font->info, bytes, stbtt_GetFontOffsetForIndex(bytes, 0));

	stbtt_pack_context context;
	if (!stbtt_PackBegin(&context, font->data, font->atlasWidth, font->atlasHeight, 0, 1, nullptr))
	{
		DisplayErrorMessage("Unable to pack font", ERROR_TYPE_Warning);
		return nullptr;
	}

	stbtt_PackSetOversampling(&context, 2, 2);
	if (!stbtt_PackFontRange(&context, bytes, 0, font->size, firstChar, charCount, font->charInfo))
	{
		DisplayErrorMessage("Unable pack font range", ERROR_TYPE_Warning);
		return nullptr;
	}

	stbtt_PackEnd(&context);
	Free(bytes);

	return font;
}

static void ProcessTaskCallbacks(TaskCallbackQueue* pQueue)
{
	u32 array = (u32)(pQueue->i >> 32);
	u64 newArray = ((u64)array ^ 1) << 32;

	u64 event = InterlockedExchange(&pQueue->i, newArray);
	u32 count = (u32)event;

	for (u32 i = 0; i < count; i++)
	{
		TaskCallback* cb = &pQueue->callbacks[array][i];
		cb->callback(cb->data);
	}
}

static void AddTaskCallback(TaskCallbackQueue* pQueue, TaskCompleteCallback* pFunc, void* pData)
{
	u64 arrayIndex = InterlockedIncrement(&pQueue->i);
	arrayIndex--;

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
		case ASSET_TYPE_Font:
			work->slot->font = LoadFontAsset(work->assets, work->load_info, work->slot->size);
			AddTaskCallback(work->queue, SendFontToGraphicsCard, work->slot->font);
			break;
	}
	_InterlockedExchange(&work->slot->state, ASSET_STATE_Loaded);
	delete work;
}

static void LoadAsset(Assets* pAssets, AssetSlot* pSlot)
{
	if (_InterlockedCompareExchange(&pSlot->state, ASSET_STATE_Queued, ASSET_STATE_Unloaded) == ASSET_STATE_Unloaded)
	{
		LoadAssetWork* work = new LoadAssetWork();
		work->load_info = pSlot->load_path;
		work->slot = pSlot;
		work->assets = pAssets;
		work->queue = &g_state.callbacks;
		QueueUserWorkItem(g_state.queue, LoadAssetBackground, work);
	}
}

static Bitmap* GetBitmap(Assets* pAssets, AssetSlot* pSlot)
{
	pSlot->last_requested = __rdtsc();
	if (pSlot->state == ASSET_STATE_Unloaded && FileExists(pSlot->load_path))
	{
		LoadAsset(pAssets, pSlot);
	}
	return pSlot->bitmap;
}

static Bitmap* GetBitmap(Assets* pAssets, BITMAPS pAsset)
{
	return GetBitmap(pAssets, pAssets->bitmaps + pAsset);
}

static FontInfo* GetFont(Assets* pAssets, AssetSlot* pSlot)
{
	pSlot->last_requested = __rdtsc();
	if (pSlot->state == ASSET_STATE_Unloaded && FileExists(pSlot->load_path))
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
				size.Width = max(size.Width, x);
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

static void AddBitmapAsset(Assets* pAssets, BITMAPS pName, const char* pPath)
{
	AssetSlot* slot = pAssets->bitmaps + pName;
	slot->type = ASSET_TYPE_Bitmap;
	strcpy(slot->load_path, pPath);
}

static void AddFontAsset(Assets* pAssets, FONTS pName, const char* pPath, float pSize)
{
	AssetSlot* slot = pAssets->fonts + pName;
	slot->type = ASSET_TYPE_Font;
	strcpy(slot->load_path, pPath);
	slot->size = pSize;
}

static Assets* LoadAssets(void* pStack, u64 pSize)
{
	Assets* assets = (Assets*)pStack;
	void* AssetMemory = (char*)pStack + sizeof(Assets);
	ZeroMemory(assets, sizeof(Assets));
	assets->memory = CreateMemoryPool(AssetMemory, pSize);
	
	AddFontAsset(assets, FONT_Subtext, "C:/windows/fonts/arial.ttf", 20);
	AddFontAsset(assets, FONT_Normal, "C:/windows/fonts/arial.ttf", 25);
	AddFontAsset(assets, FONT_Title, "C:/windows/fonts/arialbd.ttf", 35);
	AddBitmapAsset(assets, BITMAP_Background, "./Assets/background.jpg");
	AddBitmapAsset(assets, BITMAP_Placeholder, "./Assets/placeholder.jpg");
	AddBitmapAsset(assets, BITMAP_Error, "./Assets/error.png");
	AddBitmapAsset(assets, BITMAP_Question, "./Assets/question.png");
	AddBitmapAsset(assets, BITMAP_ArrowUp, "./Assets/arrow_up.png");
	AddBitmapAsset(assets, BITMAP_ArrowDown, "./Assets/arrow_down.png");
	AddBitmapAsset(assets, BITMAP_ButtonA, "./Assets/button_a.png");
	AddBitmapAsset(assets, BITMAP_ButtonB, "./Assets/button_b.png");
	AddBitmapAsset(assets, BITMAP_ButtonX, "./Assets/button_x.png");
	AddBitmapAsset(assets, BITMAP_ButtonY, "./Assets/button_y.png");
	
	//Put a 1px white texture so we get fully lit instead of only ambient lighting
	u8 data[] = { 255, 255, 255, 255 };
	glGenTextures(1, &assets->blank_texture);
	glBindTexture(GL_TEXTURE_2D, assets->blank_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return assets;
}

void FreeAsset(AssetSlot* pSlot)
{
	if (pSlot->state == ASSET_STATE_Loaded)
	{
		switch (pSlot->type)
		{
		case ASSET_TYPE_Bitmap:
			//stbi_image_free(pSlot->bitmap->data);
			Free(pSlot->bitmap);
			glDeleteTextures(1, &pSlot->bitmap->texture);
			pSlot->bitmap = nullptr;
			break;

		case ASSET_TYPE_Font:
			Free(pSlot->font);
			glDeleteTextures(1, &pSlot->font->texture);
			pSlot->font = nullptr;
			break;
		}
		pSlot->last_requested = 0;
		_InterlockedExchange(&pSlot->state, ASSET_STATE_Unloaded);
	}
}

void FreeAllAssets(YaffeState* pState, Assets* pAssets)
{
	for (u32 i = 0; i < BITMAP_COUNT; i++)
		FreeAsset(pAssets->bitmaps + i);
	for (u32 i = 0; i < FONT_COUNT; i++)
		FreeAsset(pAssets->fonts + i);

	for (u32 i = 0; i < pState->emulators.count; i++)
	{
		Application* e = pState->emulators.GetItem(i);
		for (u32 j = 0; j < e->files.count; j++)
		{
			Executable* r = e->files.GetItem(j);
			FreeAsset(&r->banner);
			FreeAsset(&r->boxart);
		}
	}
}

void EvictOldestBitmap(List<AssetSlot> pAssets)
{
	AssetSlot* slot = nullptr;
	u64 request = __rdtsc();
	for (u32 i = 0; i < pAssets.count; i++)
	{
		AssetSlot* asset = pAssets.GetItem(i);
		if (asset->state == ASSET_STATE_Loaded && asset->last_requested < request)
		{
			request = asset->last_requested;
			slot = asset;
		}
	}
	FreeAsset(slot);
}

static void InitializeAssetFiles()
{
	char assets_path[MAX_PATH];
	GetFullPath(".\\Assets\\", assets_path);

	Verify(CreateDirectoryIfNotExists(assets_path), "Unable to create assets directory", ERROR_TYPE_Warning);
}
