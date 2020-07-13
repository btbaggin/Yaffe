class PlatformDetailModal : public ModalContent
{
	Textbox fields[4];
	Checkbox application;
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
	s32 id;
	const char* field_label[4] = { "Name", "Executable", "Args", "Folder" };

	PlatformDetailModal(Platform* pPlatform)
	{
		float width = WIDTH - LABEL_WIDTH - UI_MARGIN * 2;
		for (u32 i = 0; i < ArrayCount(fields); i++) fields[i] = CreateTextbox(width, FONT_Normal);
		application = CreateCheckbox();

		if (pPlatform)
		{
			char path[MAX_PATH], args[100], roms[MAX_PATH];
			GetPlatformInfo(pPlatform->id, path, args, roms);
			SetTextboxText(&fields[0], pPlatform->name);
			SetTextboxText(&fields[1], path);
			SetTextboxText(&fields[2], args);
			SetTextboxText(&fields[3], roms);
			id = pPlatform->id;
			application.checked = (pPlatform->type == PLATFORM_App);
			application.enabled = false;
			fields[0].enabled = false;
		}

		char path[MAX_PATH];
		OpenFileSelector(path, false);
		SetTextboxText(&fields[0], path);
	}

	char* GetName() { return GetTextboxText(&fields[0]); }
	char* GetExecutable() { return GetTextboxText(&fields[1]); }
	char* GetArgs() { return GetTextboxText(&fields[2]); }
	char* GetFolder() { return GetTextboxText(&fields[3]); }
	bool GetIsApplication() { return application.checked; }
};