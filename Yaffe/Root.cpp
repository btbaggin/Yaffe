class UiRoot : public UiElement
{
	void Render(RenderState* pState)
	{
		Bitmap* b = GetBitmap(g_assets, BITMAP_Background);
		PushQuad(pState, position, GetAbsoluteSize(), b);
	}

	void Update(float pDeltaTime) { }

public:
	UiRoot() : UiElement(V2(1)) { }
};