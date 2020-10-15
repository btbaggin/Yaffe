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
			v2 position = pPosition + V2(0, (font_size + UI_MARGIN) * i);
			if (i == index)
			{
				PushSizedQuad(pState, position, V2(size.Width, font_size + UI_MARGIN), ACCENT_COLOR);
			}

			PushText(pState, FONT_Normal, text.c_str(), position, TEXT_FOCUSED);
		}
	}

public:
	std::string old_value;
	ListModal(std::vector<T> pItems, std::string pData, const char* pTitle, MODAL_SIZES pSize = MODAL_SIZE_Full)
	{
		items = pItems;
		index = 0;
		old_value = pData;
		if (pTitle)
		{
			has_title = true;
			strcpy(title, pTitle);
		}
		SetSize(pSize, ((u32)pItems.size() + (pTitle ? 1 : 0)));
	}

	T GetSelected()
	{
		return items.at(index);
	}
};