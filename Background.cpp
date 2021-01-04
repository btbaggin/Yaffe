#include "Interface.h"

class Background : public Widget
{
public:
	Background(Interface* pInterface) : Widget(UI_None, pInterface)
	{
		SetPosition(V2(0));
		SetSize(1, 1);
	}

private:
	void Render(RenderState* pState)
	{
		Bitmap* b = GetBitmap(g_assets, BITMAP_Background);
		PushQuad(pState, GetPosition(), size, b);
	}
	void Update(YaffeState* pState, float pDeltaTime) { }
};