struct PlatformService
{
	HANDLE handle;
	std::mutex mutex;
};

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

#include <stdio.h>
#include <tchar.h>
#include <TlHelp32.h>
bool IsProcessRunning(const TCHAR* pExe) 
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(snapshot, &entry))
	{
		do
		{
			if (_tcsicmp(entry.szExeFile, pExe) == 0) {
				CloseHandle(snapshot);
				return true;
			}
		} while (Process32Next(snapshot, &entry));
	}
	
	CloseHandle(snapshot);
	return false;
}

static void InitYaffeService(PlatformService* pService)
{
	if (!IsProcessRunning(L"YaffeService.exe"))
	{
		STARTUPINFOA si = {};
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_MINIMIZE;
		PROCESS_INFORMATION pi = {};
		CreateProcessA("YaffeService.exe", NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	}
}

static bool OpenNamedPipe(HANDLE* pHandle, const char* pPath, DWORD pAccess)
{
	for (u32 i = 0; i < 3; i++)
	{
		if (WaitNamedPipeA(pPath, 2000))
		{
			*pHandle = CreateFileA(pPath, pAccess, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (*pHandle != INVALID_HANDLE_VALUE) return true;
		}
	}

	return false;
}

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

static bool SendServiceMessage(PlatformService* pService, YaffeMessage* pMessage, json* pResponse)
{
	if (!g_state.has_connection) return false;

	const u32 size = Megabytes(2);
	char* buf = new char[size];
	ZeroMemory(buf, size);

	{
		char message[4048];
		CreateServiceMessage(pMessage, message);

		std::lock_guard<std::mutex> guard(pService->mutex);
		if (!OpenNamedPipe(&pService->handle, "\\\\.\\pipe\\yaffe", GENERIC_READ | GENERIC_WRITE)) return false;

		WriteFile(pService->handle, message, (DWORD)strlen(message), 0, NULL);
	}

	do
	{
		std::lock_guard<std::mutex> guard(pService->mutex);
		if (ReadFile(pService->handle, buf, size, 0, NULL))
		{
			picojson::parse(*pResponse, buf);
			free(buf);
			return true;
		}
	} while (GetLastError() == ERROR_MORE_DATA);

	free(buf);
	return false;
}

static void ShutdownYaffeService(PlatformService* pService)
{
	char message[4048];
	YaffeMessage m;
	m.type = MESSAGE_TYPE_Quit;
	CreateServiceMessage(&m, message);

	if (OpenNamedPipe(&pService->handle, "\\\\.\\pipe\\yaffe", GENERIC_WRITE))
	{
		WriteFile(pService->handle, message, (DWORD)strlen(message), 0, NULL);
	}

	CloseHandle(pService->handle);
}

#pragma comment(lib, "urlmon.lib")
#include <urlmon.h>
static void DownloadImage(const char* pUrl, AssetSlot* pSlot)
{
	IStream* stream;
	//Also works with https URL's - unsure about the extent of SSL support though.
	HRESULT result = URLOpenBlockingStreamA(0, pUrl, &stream, 0, 0);
	Verify(result == 0, "Unable to retrieve image", ERROR_TYPE_Warning);

	static const u32 READ_AMOUNT = 100;
	HANDLE file = CreateFileA(pSlot->load_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	Verify(file, "Unable to download image", ERROR_TYPE_Warning);

	char buffer[READ_AMOUNT];
	unsigned long bytesRead;
	stream->Read(buffer, READ_AMOUNT, &bytesRead);
	while (bytesRead > 0U)
	{
		WriteFile(file, buffer, READ_AMOUNT, NULL, NULL);
		stream->Read(buffer, READ_AMOUNT, &bytesRead);
	}
	stream->Release();
	CloseHandle(file);
}