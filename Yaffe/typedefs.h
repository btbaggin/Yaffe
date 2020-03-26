#pragma once
#include "stdint.h"
#include <cctype>
#include <locale>

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath/HandmadeMath.h"
//#include "glm/glm.hpp"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef unsigned char u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef signed char s8;
typedef hmm_vec4 v4;
typedef hmm_vec3 v3;
typedef hmm_vec2 v2;
typedef hmm_mat4 mat4;

static v2 V2(float xy)
{
	return { xy, xy };
}

static v2 V2(float x, float y)
{
	return { x, y };
}

static v3 V3(float x, float y, float z)
{
	return { x, y, z };
}

static v4 V4(float x, float y, float z, float w)
{
	return { x, y, z, w };
}

static v4 V4(float xyzw)
{
	return { xyzw, xyzw, xyzw, xyzw };
}

static v2 Lerp(v2 a, float time, v2 b)
{
	v2 result;
	result.X = HMM_Lerp(a.X, time, b.X);
	result.Y = HMM_Lerp(a.Y, time, b.Y);
	return result;
}

static inline void ltrim(std::string* s) {
	s->erase(s->begin(), std::find_if(s->begin(), s->end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

#define ArrayCount(a) sizeof(a) / sizeof(a[0])
#define HasFlag(flag, value) (flag & value) != 0

template <typename T>
class List
{
public:
	T* items;
	u32 count;

	T* GetItem(u32 pIndex)
	{
		if (pIndex >= count) return nullptr;
		return items + pIndex;
	}

	void InitializeWithArray(T* pItems, u32 pCount)
	{
		items = pItems;
		count = 0;
		ZeroMemory(items, sizeof(T) * pCount);
	}
};

template <typename T>
class FreeList
{
private:
	struct FreeListNode
	{
		T item;
		FreeListNode* next;
	};

	FreeListNode* list = nullptr;

public:
	T Request()
	{
		T result = 0;
		if (list)
		{
			FreeListNode* node = list;
			result = list->item;
			list = list->next;
			delete node;
		}

		return result;
	}

	void Append(T pItem)
	{
		FreeListNode* node = new FreeListNode;
		node->item = pItem;

		node->next = list;
		list = node;
	}

	bool Contains(T pItem)
	{
		for (FreeListNode* node = list; node; node = node->next)
		{
			if (node->item == pItem) return true;
		}
		return false;
	}
};