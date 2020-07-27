class PlatformDetailModal : public ModalContent
{
	Textbox name;
	FilePathBox path;
	Textbox args;
	FilePathBox roms;
	Checkbox application;

	void Render(RenderState* pState, v2 pPosition)
	{
		float size = GetFontSize(FONT_Normal);
		PushText(pState, FONT_Normal, "Application", pPosition, TEXT_FOCUSED);
		RenderCheckbox(pState, &application, pPosition + V2(LABEL_WIDTH, 0));

		pPosition.Y += (size + UI_MARGIN);
		PushText(pState, FONT_Normal, "Name", pPosition, TEXT_FOCUSED);
		RenderTextbox(pState, &name, pPosition + V2(LABEL_WIDTH, 0));

		pPosition.Y += (size + UI_MARGIN);
		PushText(pState, FONT_Normal, "Executable", pPosition, TEXT_FOCUSED);
		RenderFilePathBox(pState, &path, pPosition + V2(LABEL_WIDTH, 0));

		pPosition.Y += (size + UI_MARGIN);
		PushText(pState, FONT_Normal, "Args", pPosition, TEXT_FOCUSED);
		RenderTextbox(pState, &args, pPosition + V2(LABEL_WIDTH, 0));

		pPosition.Y += (size + UI_MARGIN);
		PushText(pState, FONT_Normal, application.checked ? "Image" : "Folder", pPosition, TEXT_FOCUSED);
		RenderFilePathBox(pState, &roms, pPosition + V2(LABEL_WIDTH, 0));
	}

public:
	s32 id;

	PlatformDetailModal(Platform* pPlatform)
	{
		SetSize(MODAL_SIZE_Full, 5);

		float width = size.Width - LABEL_WIDTH - UI_MARGIN * 2;
		name = CreateTextbox(width, FONT_Normal);
		path = CreateFilePathBox(width, FONT_Normal, true);
		args = CreateTextbox(width, FONT_Normal);
		roms = CreateFilePathBox(width, FONT_Normal, false);
		application = CreateCheckbox();

		if (pPlatform)
		{
			id = pPlatform->id;

			char path_text[MAX_PATH], args_text[100], roms_text[MAX_PATH];
			GetPlatformInfo(id, path_text, args_text, roms_text);
			SetText(&name, pPlatform->name);
			SetText(&path, path_text);
			SetText(&args, args_text);
			SetText(&roms, roms_text);

			application.checked = (pPlatform->type == PLATFORM_App);
			application.enabled = false;
			name.enabled = false;
		}

	}

	char* GetName() { return name.string; }
	char* GetExecutable() { return path.string; }
	char* GetArgs() { return args.string; }
	char* GetFolder() { return roms.string; }
	bool GetIsApplication() { return application.checked; }
};