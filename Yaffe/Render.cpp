static GLuint LoadShader(const char* pShader, GLenum pType)
{
	GLuint shader_id = glCreateShader(pType);

	glShaderSource(shader_id, 1, &pShader, 0);
	glCompileShader(shader_id);

	GLint result = GL_FALSE;
	int info_log_length;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char* error = new char[info_log_length + 1];
		glGetShaderInfoLog(shader_id, info_log_length, 0, error);
		DisplayErrorMessage(error, ERROR_TYPE_Warning);
	}

	return shader_id;
}

static GLProgram CreateProgram(GLuint pVertex, GLuint pFragment)
{
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, pVertex);
	glAttachShader(program_id, pFragment);
	glLinkProgram(program_id);

	GLint result = GL_FALSE;
	int info_log_length;
	glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0)
	{
		char* error = new char[info_log_length + 1];
		glGetProgramInfoLog(program_id, info_log_length, 0, error);
		DisplayErrorMessage(error, ERROR_TYPE_Warning);
	}

	glDetachShader(program_id, pVertex);
	glDetachShader(program_id, pFragment);

	glDeleteShader(pVertex);
	glDeleteShader(pFragment);

	GLProgram prog;
	prog.id = program_id;
	prog.texture = glGetUniformLocation(program_id, "mainTex");
	prog.mvp = glGetUniformLocation(program_id, "MVP");
	prog.font = glGetUniformLocation(program_id, "font");
	return prog;
}

#define Megabytes(size) size * 1024 * 1024
static void InitializeRenderer(RenderState* pState)
{
	void* render = malloc(Megabytes(6));
	pState->arena = CreateMemoryStack(render, Megabytes(6));
	pState->index_count = 0;
	pState->vertex_count = 0;
	pState->vertices = CreateSubStack(pState->arena, Megabytes(4));
	pState->indices = CreateSubStack(pState->arena, Megabytes(1) - sizeof(MemoryStack));

	glGenVertexArrays(1, &pState->VAO);
	glBindVertexArray(pState->VAO);

	glGenBuffers(1, &pState->VBO);
	glGenBuffers(1, &pState->EBO);
	glBindBuffer(GL_ARRAY_BUFFER, pState->VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pState->EBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

	GLuint v_shader = LoadShader(VERTEX_SHADER, GL_VERTEX_SHADER);
	GLuint f_shader = LoadShader(FRAGMENT_SHADER, GL_FRAGMENT_SHADER);
	pState->program = CreateProgram(v_shader, f_shader);
	glUseProgram(pState->program.id);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void BeginRenderPass(v2 pFormSize, RenderState* pState)
{
	pState->vertex_count = 0;
	pState->index_count = 0;
	pState->entry_count = 0;

	pState->vertices->count = 0;
	pState->indices->count = 0;

	mat4 m = HMM_Orthographic(0.0F, pFormSize.Width, 0.0F, pFormSize.Height, -1.0F, 1.0F);
	glUniformMatrix4fv(pState->program.mvp, 1, GL_FALSE, &m.Elements[0][0]);
	pState->memory = BeginTemporaryMemory(pState->arena);
}


static void EndRenderPass(v2 pFormSize, RenderState* pState)
{
	glBindVertexArray(pState->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, pState->VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pState->EBO);
	glBufferData(GL_ARRAY_BUFFER, pState->vertex_count * sizeof(Vertex), MemoryAddress(pState->vertices), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, pState->index_count * sizeof(u16), MemoryAddress(pState->indices), GL_STATIC_DRAW);

	glViewport(0, 0, (int)pFormSize.Width, (int)pFormSize.Height);

	char* address = (char*)MemoryAddress(pState->arena, pState->memory.count);
	for (u32 i = 0; i < pState->entry_count; i++)
	{
		RenderEntry* header = (RenderEntry*)address;
		address += sizeof(RenderEntry);

		switch (header->type)
		{
		case RENDER_GROUP_ENTRY_TYPE_Text:
		{
			glUniform1i(pState->program.font, 1);

			Renderable_Text* text = (Renderable_Text*)address;

			glBindTexture(GL_TEXTURE_2D, text->texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(pState->program.texture, 0);

			if (HasFlag(text->flags, RENDER_TEXT_FLAG_Clip))
			{
				glEnable(GL_SCISSOR_TEST);
				glScissor((u32)text->clip_min.X, (u32)(g_state.form->height - (text->clip_min.Y + text->clip_max.Y)), (int)text->clip_max.X, (int)text->clip_max.Y);
			}

			glDrawElementsBaseVertex(GL_TRIANGLES, text->index_count, GL_UNSIGNED_SHORT, (void*)(text->first_index * sizeof(u16)), text->first_vertex);
			glDisable(GL_SCISSOR_TEST);

			glUniform1i(pState->program.font, 0);
		}
		break;

		case RENDER_GROUP_ENTRY_TYPE_Quad:
		{
			Renderable_Quad* vertices = (Renderable_Quad*)address;

			glActiveTexture(GL_TEXTURE0);
			glUniform1i(pState->program.texture, 0);
			glBindTexture(GL_TEXTURE_2D, vertices->texture);

			glDrawElementsBaseVertex(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)(vertices->first_index * sizeof(u16)), vertices->first_vertex);
		}
		break;

		case RENDER_GROUP_ENTRY_TYPE_Line:
		{
			Renderable_Line* line = (Renderable_Line*)address;

			glActiveTexture(GL_TEXTURE0);
			glUniform1i(pState->program.texture, 0);
			glBindTexture(GL_TEXTURE_2D, g_assets->blank_texture);

			glLineWidth(line->size);
			glDrawArrays(GL_LINES, line->first_vertex, 2);
		}
		break;
		}

		address += header->size;
	}

	glBindVertexArray(0);
	EndTemporaryMemory(pState->memory);
}

static inline
void* _PushRenderGroupEntry(RenderState* pState, u8 pSize, RENDER_GROUP_ENTRY_TYPE pType)
{
	RenderEntry* header = PushStruct(pState->arena, RenderEntry);
	header->type = pType;
	header->size = pSize;

	void* result = PushSize(pState->arena, pSize);
	pState->entry_count++;

	return result;
}
#define PushRenderGroupEntry(pState, pStruct, pType) (pStruct*)_PushRenderGroupEntry(pState, (u8)sizeof(pStruct), pType)

GlyphQuad GetGlyph(FontInfo* pFont, u32 pChar, v4 pColor, float* pX, float* pY)
{
	stbtt_aligned_quad quad;

	int index = pChar - ' ';
	if (index < 0 || index > '~' - ' ') index = 0;

	stbtt_GetPackedQuad(pFont->charInfo, pFont->atlasWidth, pFont->atlasHeight, index, pX, pY, &quad, 1);

	float x_min = quad.x0;
	float x_max = quad.x1;
	float y_min = -quad.y1;
	float y_max = -quad.y0;

	GlyphQuad info = {};
	info.vertices[0] = { { x_min, y_min }, pColor, { quad.s0, quad.t1 } };
	info.vertices[1] = { { x_min, y_max }, pColor, { quad.s0, quad.t0 } };
	info.vertices[2] = { { x_max, y_max }, pColor, { quad.s1, quad.t0 } };
	info.vertices[3] = { { x_max, y_min }, pColor, { quad.s1, quad.t1 } };

	return info;
}

static void _PushText(RenderState* pState, FONTS pFont, const char* pText, v2 pPosition, v4 pColor, v2 pClipMin, v2 pClipMax)
{
	float x = pPosition.X;
	float y = pPosition.Y;

	FontInfo* font = GetFont(g_assets, pFont);
	if (font)
	{
		y -= (g_state.form->height - font->size);
		Renderable_Text* m2 = PushRenderGroupEntry(pState, Renderable_Text, RENDER_GROUP_ENTRY_TYPE_Text);
		m2->first_vertex = pState->vertex_count;
		m2->first_index = pState->index_count;
		m2->index_count = 0;
		m2->texture = font->texture;
		m2->flags = 0;

		if (pClipMin.X > 0) m2->flags |= RENDER_TEXT_FLAG_Clip;
		m2->clip_min = pClipMin;
		m2->clip_max = pClipMax;

		u32 base_v = 0;
		for (u32 i = 0; pText[i] != 0; i++)
		{
			const unsigned char c = pText[i];
			if (c == '\n')
			{
				y += font->size;
				x = pPosition.X;
			}
			GlyphQuad info = GetGlyph(font, c, pColor, &x, &y);

			Vertex* v = PushArray(pState->vertices, Vertex, 4);
			for (u32 i = 0; i < 4; i++)
			{
				v[i] = info.vertices[i];
			}

			u16* indices = PushArray(pState->indices, u16, 6);
			indices[0] = base_v + 0; indices[1] = base_v + 2; indices[2] = base_v + 1;
			indices[3] = base_v + 0; indices[4] = base_v + 3; indices[5] = base_v + 2;

			m2->index_count += 6;
			pState->index_count += 6;
			pState->vertex_count += 4;
			base_v += 4;
		}
	}
}

static void PushText(RenderState* pState, FONTS pFont, const char* pText, v2 pPosition, v4 pColor)
{
	_PushText(pState, pFont, pText, pPosition, pColor, V2(0), V2(0));
}

static void PushClippedText(RenderState* pState, FONTS pFont, const char* pText, v2 pPosition, v4 pColor, v2 pClipMin, v2 pClipMax)
{
	_PushText(pState, pFont, pText, pPosition, pColor, pClipMin, pClipMax);
}

static void PushQuad(RenderState* pState, v2 pMin, v2 pMax, v4 pColor, Bitmap* pTexture)
{
	pMin.Y = g_state.form->height - pMin.Y;
	pMax.Y = g_state.form->height - pMax.Y;
	Vertex* vertices = PushArray(pState->vertices, Vertex, 4);
	vertices[0] = { { pMin.X, pMin.Y }, pColor, {0, 0} };
	vertices[1] = { { pMin.X, pMax.Y }, pColor, {0, 1} };
	vertices[2] = { { pMax.X, pMax.Y }, pColor, {1, 1} };
	vertices[3] = { { pMax.X, pMin.Y }, pColor, {1, 0} };

	u16* indices = PushArray(pState->indices, u16, 6);
	indices[0] = 0; indices[1] = 1; indices[2] = 2;
	indices[3] = 2; indices[4] = 3; indices[5] = 0;

	Renderable_Quad* m = PushRenderGroupEntry(pState, Renderable_Quad, RENDER_GROUP_ENTRY_TYPE_Quad);
	m->first_vertex = pState->vertex_count;
	m->first_index = pState->index_count;
	m->texture = pTexture ? pTexture->texture : g_assets->blank_texture;

	pState->vertex_count += 4;
	pState->index_count += 6;
}
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, v4 pColor, Bitmap* pTexture)
{
	PushQuad(pState, pMin, pMin + pSize, pColor, pTexture);
}

static void PushQuad(RenderState* pState, v2 pMin, v2 pMax, Bitmap* pTexture)
{
	PushQuad(pState, pMin, pMax, V4(1, 1, 1, 1), pTexture);
}
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, Bitmap* pTexture)
{
	PushQuad(pState, pMin, pMin + pSize, V4(1, 1, 1, 1), pTexture);
}

static void PushQuad(RenderState* pState, v2 pMin, v2 pMax, v4 pColor)
{
	PushQuad(pState, pMin, pMax, pColor, nullptr);
}
static void PushSizedQuad(RenderState* pState, v2 pMin, v2 pSize, v4 pColor)
{
	PushQuad(pState, pMin, pMin + pSize, pColor, nullptr);
}

static void PushLine(RenderState* pState, v2 pP1, v2 pP2, float pSize, v4 pColor)
{
	pP1.Y = g_state.form->height - pP1.Y;
	pP2.Y = g_state.form->height - pP2.Y;
	Renderable_Line* line = PushRenderGroupEntry(pState, Renderable_Line, RENDER_GROUP_ENTRY_TYPE_Line);
	line->first_vertex = pState->vertex_count;
	line->size = pSize;

	Vertex* v = PushArray(pState->vertices, Vertex, 2);
	*(v + 0) = { pP1, pColor, {0.0F, 0.0F} };
	*(v + 1) = { pP2, pColor, {0.0F, 0.0F} };

	pState->vertex_count += 2;
}

static void PushRightAlignedTextWithIcon(RenderState* pState, v2* pRight, BITMAPS pIcon, float pIconSize, FONTS pFont, const char* pText, v4 pTextColor = V4(0, 0, 0, 1))
{
	v2 text_size = MeasureString(pFont, pText);
	pRight->X -= text_size.Width;

	PushText(pState, pFont, pText, *pRight, pTextColor);
	if (pIcon != BITMAP_None)
	{
		pRight->X -= pIconSize;
		PushSizedQuad(pState, V2(pRight->X, pRight->Y + text_size.Height - pIconSize + 2), V2(pIconSize), V4(1, 1, 1, pTextColor.A), GetBitmap(g_assets, pIcon));
	}

	pRight->X -= UI_MARGIN;
}

static void DisposeRenderState(RenderState* pState)
{
	glDeleteProgram(pState->program.id);
	glDeleteBuffers(1, &pState->EBO);
	glDeleteBuffers(1, &pState->VBO);
	glDeleteVertexArrays(1, &pState->VAO);
}