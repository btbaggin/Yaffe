#pragma once
class ModalContent
{
public:
	v2 size;
	virtual MODAL_RESULTS Update(float pDeltaTime)
	{
		if (IsEnterPressed())
		{
			return MODAL_RESULT_Ok;
		}
		else if (IsEscPressed())
		{
			return MODAL_RESULT_Cancel;
		}
		return MODAL_RESULT_None;
	}
	virtual void Render(RenderState* pState, v2 pPosition) = 0;
};

class StringModal : public ModalContent
{
	std::string message;
	MODAL_RESULTS Update(float pDeltaTime)
	{
		size = MeasureString(FONT_Normal, message.c_str());
		return ModalContent::Update(pDeltaTime);
	}

	void Render(RenderState* pState, v2 pPosition)
	{
		PushText(pState, FONT_Normal, message.c_str(), pPosition, TEXT_FOCUSED);
	}

public:
	StringModal(std::string pMessage)
	{
		message = pMessage;
	}
};


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

class AddApplicationModal : public ModalContent
{	
	const float WIDTH = 720;

	MODAL_RESULTS Update(float pDeltaTime)
	{
		size = V2(WIDTH, 480);
		return ModalContent::Update(pDeltaTime);
	}

	void Render(RenderState* pState, v2 pPosition)
	{
		float size = GetFontSize(FONT_Normal);
		RenderElementWithLabel(pState, Checkbox, application, pPosition, "Application");
		field_label[2] = application.checked ? "Image" : "Folder";

		for (u32 i = 0; i < ArrayCount(fields); i++)
		{
			float y = (size + UI_MARGIN) * (i + 1);
			RenderElementWithLabel(pState, Textbox, fields[i], pPosition + V2(0, y), field_label[i]);
		}
	}

public:
	Checkbox application;
	Textbox fields[3];
	const char* field_label[3] = { "Name", "Executable", "Folder" };

	AddApplicationModal()
	{
		float width = WIDTH - LABEL_WIDTH - UI_MARGIN * 2;
		for (u32 i = 0; i < ArrayCount(fields); i++) fields[i] = CreateTextbox(width, FONT_Normal);
		application = CreateCheckbox();
	}
};