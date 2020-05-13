struct PlatformService
{
	HANDLE read;
	HANDLE write;
	std::mutex mutex;
};

enum MESSAGE_TYPES : u32
{
	MESSAGE_TYPE_Platform,
	MESSAGE_TYPE_Game,
	MESSAGE_TYPE_Quit,
};


#include <json.hpp>
using json = nlohmann::json;
static json CreateServiceMessage(MESSAGE_TYPES pType, const char* pName, s32 pPlatform)
{
	json j;
	j["type"] = pType;
	switch (pType)
	{
		case MESSAGE_TYPE_Game:
		{
			j["platform"] = pPlatform;
			j["name"] = pName;
		}
		break;

		case MESSAGE_TYPE_Platform:
		{
			j["name"] = pName;
		}
		break;

		case MESSAGE_TYPE_Quit:
			break;
	}

	return j;
}
#include <stdio.h>
static void InitYaffeService(PlatformService* pService)
{
	STARTUPINFOA si = {};
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_MINIMIZE;
	PROCESS_INFORMATION pi = {};
	CreateProcessA("YaffeService.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

static void OpenNamedPipe(HANDLE* pHandle, const char* pPath, DWORD pAccess)
{
	while (!*pHandle || *pHandle == INVALID_HANDLE_VALUE)
	{
		if (WaitNamedPipeA(pPath, 2000))
		{
			*pHandle = CreateFileA(pPath, pAccess, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		}
	}
}
static bool SendServiceMessage(PlatformService* pService, json pRequest, json* pResponse)
{
	std::lock_guard<std::mutex> guard(pService->mutex);
	OpenNamedPipe(&pService->read, "\\\\.\\pipe\\yaffe", GENERIC_READ);
	OpenNamedPipe(&pService->write, "\\\\.\\pipe\\yaffeServer", GENERIC_WRITE);

	const u32 size = Megabytes(2);
	char* buf = new char[size];
	ZeroMemory(buf, size);

	std::string message = pRequest.dump();
	WriteFile(pService->write, message.c_str(), message.length(), 0, NULL);

	do
	{
		if (ReadFile(pService->read, buf, size, 0, NULL))
		{
			*pResponse = json::parse(buf);
			free(buf);
			return true;
		}
		else if (GetLastError() != ERROR_MORE_DATA)
		{
			return false;
		}
	} while (GetLastError() == ERROR_MORE_DATA);

	return false;
}


static void ShutdownYaffeService(PlatformService* pServer)
{
	json j;
	j["type"] = MESSAGE_TYPE_Quit;
	std::string message = j.dump();
	WriteFile(pServer->write, message.c_str(), message.length(), 0, NULL);

	CloseHandle(pServer->read);
	CloseHandle(pServer->write);
}