static std::string GetDisplayString(PlatformInfo* pInfo)
{
	return pInfo->name;
}
static std::string GetDisplayString(GameInfo* pInfo)
{
	return pInfo->name;
}
static std::string GetDisplayString(std::string* pInfo)
{
	return *pInfo;
}

//We need to define GetDisplayString for each type that will be using this modal
template <typename T>
class ListModal : public ModalContent
{
	s32 index;
	std::vector<T> items;
	char title[200];
	bool has_title = false;

	MODAL_RESULTS Update(float pDeltaTime)
	{
		if (IsDownPressed())
		{
			index++;
			index = min(index, (s32)items.size() - 1);
		}
		else if (IsUpPressed())
		{
			index--;
			index = max(index, 0);
		}
		return ModalContent::Update(pDeltaTime);
	}

	void Render(RenderState* pState, v2 pPosition)
	{
		if (has_title)
		{
			float font_size = GetFontSize(FONT_Normal);
			PushText(pState, FONT_Normal, title, pPosition, TEXT_FOCUSED);
			pPosition.Y += GetFontSize(FONT_Normal) + UI_MARGIN;
		}

		float font_size = GetFontSize(FONT_Normal);
		for (u32 i = 0; i < items.size(); i++)
		{
			std::string text = GetDisplayString(items.data() + i);
			v2 position = pPosition + V2(0, (font_size + UI_MARGIN * 2) * i);
			if (i == index)
			{
				PushSizedQuad(pState, position, V2(size.Width, font_size + UI_MARGIN * 2), ACCENT_COLOR);
			}

			PushText(pState, FONT_Normal, text.c_str(), position, TEXT_FOCUSED);
		}
	}

	void CalculateSize()
	{
		size = {};
		v2 font_size = MeasureString(FONT_Normal, title);
		if (has_title)
		{
			size = font_size;
			size.Height += UI_MARGIN;
		}

		size.Height += items.size() * (font_size.Height + UI_MARGIN * 2);
		for (u32 i = 0; i < items.size(); i++)
		{
			std::string text = GetDisplayString(items.data() + i);
			v2 font_width = MeasureString(FONT_Normal, text.c_str());
			size.Width = max(font_width.Width + UI_MARGIN * 2, size.Width);
		}
	}

public:
	std::string old_value;
	ListModal(std::vector<T> pItems, std::string pData, const char* pTitle)
	{
		items = pItems;
		index = 0;
		old_value = pData;
		if (pTitle)
		{
			has_title = true;
			strcpy(title, pTitle);
		}
		CalculateSize();
	}

	T GetSelected()
	{
		return items.at(index);
	}
};