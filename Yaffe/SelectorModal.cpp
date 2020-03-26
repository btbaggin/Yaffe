#pragma once

class SelectorModal : public ModalContent
{
	s32 index;
	std::vector<std::string> items;

	MODAL_RESULTS Update(float pDeltaTime)
	{
		for (u32 i = 0; i < items.size(); i++)
		{
			v2 font_size = MeasureString(FONT_Normal, items.at(i).c_str());
			size = V2(max(font_size.Width + UI_MARGIN * 2, size.Width), items.size() * (font_size.Height + UI_MARGIN * 2));
		}

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
		float font_size = GetFontSize(FONT_Normal);
		for (u32 i = 0; i < items.size(); i++)
		{
			std::string text = items.at(i);
			v2 position = pPosition + V2(0, (font_size + UI_MARGIN * 2) * i);
			if (i == index)
			{
				PushQuad(pState, position, position + V2(size.Width, font_size + UI_MARGIN * 2), ACCENT_COLOR);
			}

			PushText(pState, FONT_Normal, text.c_str(), position, TEXT_FOCUSED);
		}
	}

public:
	void* data;
	SelectorModal(std::vector<std::string> pItems, void* pData)
	{
		items = pItems;
		index = 0;
		data = pData;
	}

	std::string GetSelected()
	{
		return items.at(index);
	}
};