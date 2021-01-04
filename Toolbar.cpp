class Toolbar : public Widget
{
public:
	Toolbar(Interface* pInterface) : Widget(UI_None, pInterface) { }

private:
	void OnAdded()
	{
		SetSize(0.99F, 0.05F);
		SetPosition(V2(0, parent->size.Height - size.Height * 2));
	}

	void Render(RenderState* pState)
	{
		float title = GetFontSize(FONT_Title);

		v2 menu_position = GetPosition() + size;
		//Time
		char buffer[20];
		GetTime(buffer, 20);
		PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_None, FONT_Title, buffer, TEXT_FOCUSED); menu_position.X -= UI_MARGIN;

		//Action buttons
		menu_position.Y += (title - GetFontSize(FONT_Normal));
		switch (GetFocusedElement())
		{
		case UI_Roms:
			PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonY, FONT_Normal, "Filter"); menu_position.X -= UI_MARGIN;
			PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonB, FONT_Normal, "Back"); menu_position.X -= UI_MARGIN;
			break;

		case UI_Emulators:
			Platform* plat = GetSelectedPlatform();
			if (plat && plat->type != PLATFORM_Recents)
			{
				PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonX, FONT_Normal, "Info"); menu_position.X -= UI_MARGIN;
			}
			PushRightAlignedTextWithIcon(pState, &menu_position, BITMAP_ButtonA, FONT_Normal, "Select"); menu_position.X -= UI_MARGIN;
			break;
		}
	}

	void Update(YaffeState* pState, float pDeltaTime) { }
};