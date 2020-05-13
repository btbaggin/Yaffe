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
		field_label[3] = application.checked ? "Image" : "Folder";

		for (u32 i = 0; i < ArrayCount(fields); i++)
		{
			float y = (size + UI_MARGIN) * (i + 1);
			RenderElementWithLabel(pState, Textbox, fields[i], pPosition + V2(0, y), field_label[i]);
		}
	}

public:
	Checkbox application;
	Textbox fields[4];
	const char* field_label[4] = { "Name", "Executable", "Args", "Folder" };

	AddApplicationModal()
	{
		float width = WIDTH - LABEL_WIDTH - UI_MARGIN * 2;
		for (u32 i = 0; i < ArrayCount(fields); i++) fields[i] = CreateTextbox(width, FONT_Normal);
		application = CreateCheckbox();
	}

	std::string GetName() { return std::string(fields[0].string, fields[0].stringlen); }
	std::string GetExecutable() { return std::string(fields[1].string, fields[1].stringlen); }
	std::string GetArgs() { return std::string(fields[2].string, fields[2].stringlen); }
	std::string GetFolder() { return std::string(fields[3].string, fields[3].stringlen); }

};