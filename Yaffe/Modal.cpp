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