enum MESSAGE_TYPES : u32
{
	MESSAGE_TYPE_Platform,
	MESSAGE_TYPE_Game,
	MESSAGE_TYPE_Quit,
};

struct YaffeMessage
{
	MESSAGE_TYPES type;
	s32 platform;
	const char* name;
};

static void CreateServiceMessage(YaffeMessage* pArgs, char* pBuffer)
{
	switch (pArgs->type)
	{
		case MESSAGE_TYPE_Game:
			sprintf(pBuffer, R"({"type":%d,"platform":%d,"name":"%s"})", pArgs->type, pArgs->platform, pArgs->name);
			break;

		case MESSAGE_TYPE_Platform:
			sprintf(pBuffer, R"({"type":%d,"name":"%s"})", pArgs->type, pArgs->name);
			break;

		case MESSAGE_TYPE_Quit:
			sprintf(pBuffer, R"({"type":%d})", pArgs->type);
			break;

		default:
			assert(false);
	}
}
