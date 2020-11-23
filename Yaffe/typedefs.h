#pragma once
#include "stdint.h"
#include <cctype>
#include <locale>

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath/HandmadeMath.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef unsigned char u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef signed char s8;
typedef hmm_vec4 v4;
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
static v4 V4(float x, float y, float z, float w)
{
	return { x, y, z, w };
}
static v4 V4(v4 xyz, float w)
{
	return { xyz.X, xyz.Y, xyz.Z, w };
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

inline static bool IsZero(v2 a) { return a.X == 0 && a.Y == 0; }

static bool operator<(v2 l, v2 r)
{
	return l.X < r.X && l.Y < r.Y;
}
static bool operator>(v2 l, v2 r)
{
	return l.X > r.X && l.Y > r.Y;
}

static void GetTime(char* pBuffer, u32 pBufferSize, const char* pFormat = "%I:%M%p")
{
	time_t now = time(0);
	struct tm* timeinfo = localtime(&now);
	strftime(pBuffer, pBufferSize, pFormat, timeinfo);
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

	T* AddItem()
	{
		return items + count++;
	}

	void Initialize(u32 pCount)
	{
		if (items) delete[] items;
		items = new T[pCount];
		count = 0;
		ZeroMemory(items, sizeof(T) * pCount);
	}
};

template<typename T, unsigned N>
class StaticList
{
	T items[N];
public:
	volatile long count;

	void AddItem(T item) { items[count++] = item; }

	void Clear() { count = 0; }

	bool CanAdd() { return count < N; }

	T CurrentItem() { return items[count - 1]; }

	T& operator[](int index) { return items[index]; }
};