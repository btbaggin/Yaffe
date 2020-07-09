#pragma once

struct ImageInfo
{
	int width;
	int height;
	int channels;
	stbi_uc* data;
	char file[MAX_PATH];
};

struct TextureAtlasEntry
{
	char file[MAX_PATH];
	float u_min;
	float u_max;
	float v_min;
	float v_max;
};

struct TextureAtlas
{
	TextureAtlasEntry* entries;
	unsigned int count;
};

static void ta_WriteAtlasHeader(int pWidth, int pHeight, int pCount, FILE* pFile)
{
	fwrite(&pWidth, sizeof(int), 1, pFile);
	fwrite(&pHeight, sizeof(int), 1, pFile);
	fwrite(&pCount, sizeof(int), 1, pFile);
}

static void ta_WriteAtlasImage(ImageInfo* pInfo, int pX, int pY, FILE* pFile)
{
	fputs(pInfo->file, pFile);
	fputc('\0', pFile);
	fwrite(&pInfo->width, sizeof(int), 1, pFile);
	fwrite(&pInfo->height, sizeof(int), 1, pFile);
	fwrite(&pX, sizeof(int), 1, pFile);
	fwrite(&pY, sizeof(int), 1, pFile);
}

static TextureAtlas ta_ReadTextureAtlas(const char* pPath)
{
	TextureAtlas atlas = {};

	char path[MAX_PATH];
	GetFullPathNameA(pPath, MAX_PATH, path, 0);

	FILE* file = fopen(path, "r");
	if (!file) return atlas;

	int atlas_width, atlas_height;
	fread(&atlas_width, sizeof(int), 1, file);
	fread(&atlas_height, sizeof(int), 1, file);
	fread(&atlas.count, sizeof(int), 1, file);

	atlas.entries = new TextureAtlasEntry[atlas.count];
	for(unsigned int i = 0; i < atlas.count; i++)
	{
		TextureAtlasEntry* e = atlas.entries + i;;
		int width, height, x, y;
		char ch;
		int j = 0;
		while ((ch = fgetc(file)) != '\0' && ch != EOF)
			e->file[j++] = ch;
		e->file[j] = '\0';

		fread(&width, sizeof(int), 1, file);
		fread(&height, sizeof(int), 1, file);
		fread(&x, sizeof(int), 1, file);
		fread(&y, sizeof(int), 1, file);

		e->u_min = x / (float)atlas_width;
		e->u_max = (x + width) / (float)atlas_width;
		e->v_min = y / (float)atlas_height;
		e->v_max = (y + height) / (float)atlas_height;
	}

	return atlas;
}

static void ta_DisposeTextureAtlas(TextureAtlas* pAtlas)
{
	delete[] pAtlas->entries;
}