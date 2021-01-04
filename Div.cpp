class Div : public Widget
{
public:
	Div(v2 pPosition, float pWidth, float pHeight, Interface* pInterface) : Widget(UI_None, pInterface)
	{
		SetPosition(pPosition);
		SetSize(pWidth, pHeight);
	}

private:
	void Render(RenderState* pState) { }
	void Update(YaffeState* pState, float pDeltaTime) { }
};