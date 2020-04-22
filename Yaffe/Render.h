#pragma once
#include "shaders.h"
enum RENDER_GROUP_ENTRY_TYPE : u8
{
	RENDER_GROUP_ENTRY_TYPE_Line,
	RENDER_GROUP_ENTRY_TYPE_Text,
	RENDER_GROUP_ENTRY_TYPE_Quad,
};

struct Renderable_Text
{
	u32 first_index;
	u32 first_vertex;
	u32 index_count;
	u32 texture;
};

struct Renderable_Quad
{
	u32 first_index;
	u32 first_vertex;
	u32 texture;
};

struct Renderable_Line
{
	u32 first_vertex;
	float size;
};

struct RenderEntry
{
	RENDER_GROUP_ENTRY_TYPE type;
	u8 size;
};

struct Vertex
{
	v2 position;
	v4 color;
	v2 uv;
};

struct GlyphQuad
{
	Vertex vertices[4];
};

struct GLProgram
{
	u32 id;
	u32 texture;
	u32 mvp;
	u32 font;
};

struct RenderState
{
	MemoryStack* arena;
	TemporaryMemoryHandle memory;
	u32 VAO;
	u32 VBO;
	u32 EBO;

	GLProgram program;

	MemoryStack* vertices;
	u32 vertex_count;

	MemoryStack* indices;
	u32 index_count;

	u32 entry_count;
};