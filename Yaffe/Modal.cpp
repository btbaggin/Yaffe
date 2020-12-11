#pragma once
enum MODAL_SIZES
{
	MODAL_SIZE_Full,
	MODAL_SIZE_Half,
	MODAL_SIZE_Third,
};

class ModalContent
{
public:
	v2 size;
	virtual MODAL_RESULTS Update(float pDeltaTime)
	{
		if (IsEnterPressed()) return MODAL_RESULT_Ok;
		else if (IsEscPressed()) return MODAL_RESULT_Cancel;
		
		return MODAL_RESULT_None;
	}
	virtual void Render(RenderState* pState, v2 pPosition) = 0;

	void SetSize(MODAL_SIZES pSize, int pRows)
	{
		float width = g_state.form->width;
		if (pSize == MODAL_SIZE_Half) width /= 2.0F;
		else if (pSize == MODAL_SIZE_Third) width /= 3.0F;

		float height = GetFontSize(FONT_Normal) + UI_MARGIN;
		size = V2(width, height * pRows);
	}
};

class StringModal : public ModalContent
{
	std::string message;
	MODAL_RESULTS Update(float pDeltaTime)
	{
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
		size = MeasureString(FONT_Normal, message.c_str());
	}
};
