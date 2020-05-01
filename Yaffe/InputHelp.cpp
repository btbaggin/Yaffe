struct KeyHelp
{
	BITMAPS bitmap;
	const char* message;
};

static void RenderInputHelp(RenderState* pState, UiRegion pRegion, KeyHelp* pHelp, u32 pHelpCount)
{
	v2 menu_position = pRegion.position + pRegion.size;
	menu_position -= V2(UI_MARGIN);
	for (u32 i = 0; i < pHelpCount; i++)
	{
		KeyHelp* kh = pHelp + i;
		menu_position = PushRightAlignedTextWithIcon(pState, menu_position, kh->bitmap, 24, FONT_Normal, kh->message);
	}
}

static void DisplayInputHelp(RenderState* pState, UiRegion pRegion)
{
	UI_ELEMENT_NAME focus = g_ui.focus[g_ui.focus_index - 1];
	switch (focus)
	{
		case UI_Roms:
		{
			KeyHelp keys[4] = {
				{BITMAP_ButtonY, "Filter"},
				{BITMAP_ButtonX, "Info"},
				{BITMAP_ButtonB, "Back"},
				{BITMAP_ButtonA, "Run"},
			};
			RenderInputHelp(pState, pRegion, keys, ArrayCount(keys));
		}
		break;

		case UI_Emulators:
		{
			KeyHelp keys[2] = {
				{BITMAP_ButtonX, "Add"},
				{BITMAP_ButtonA, "Select"},
			};
			RenderInputHelp(pState, pRegion, keys, ArrayCount(keys));
		}
		break;
	}

}
