class YaffeSettingsModal : public ModalContent
{
	const float WIDTH = 720;

	void Render(RenderState* pState, v2 pPosition)
	{
		float size = GetFontSize(FONT_Normal);
		PushText(pState, FONT_Normal, "Run At Startup", pPosition, TEXT_FOCUSED);
		RenderCheckbox(pState, &run_at_startup, pPosition + V2(LABEL_WIDTH, 0));
	}

public:
	Checkbox run_at_startup;

	YaffeSettingsModal()
	{
		run_at_startup = CreateCheckbox();
		run_at_startup.checked = RunAtStartUp(STARTUP_INFO_Get, false);
		size = V2(WIDTH, 100);
	}
};

static MODAL_CLOSE(YaffeSettingsModalClose)
{
	if (pResult == MODAL_RESULT_Ok)
	{
		YaffeSettingsModal* content = (YaffeSettingsModal*)pContent;
		Verify(RunAtStartUp(STARTUP_INFO_Set, content->run_at_startup.checked), "Unable to set run at startup", ERROR_TYPE_Warning);
	}
}