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


#include <stdio.h>
#include <json.h>
using json = picojson::value;
static json CreateServiceMessage(MESSAGE_TYPES pType, const char* pName, s32 pPlatform)
{
	picojson::object j;
	j["type"] = json((double)pType);
	switch (pType)
	{
		case MESSAGE_TYPE_Game:
		{
			j["platform"] = json((double)pPlatform);
			j["name"] = json(pName);
		}
		break;

		case MESSAGE_TYPE_Platform:
		{
			j["name"] = json(pName);
		}
		break;

		case MESSAGE_TYPE_Quit:
			break;
	}

	return json(j);
}

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
		CreateProcessA("YaffeService.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	}
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
	const u32 size = Megabytes(2);
	char* buf = new char[size];
	ZeroMemory(buf, size);

	{
		std::lock_guard<std::mutex> guard(pService->mutex);
		std::string message = pRequest.serialize();
		OpenNamedPipe(&pService->handle, "\\\\.\\pipe\\yaffe", GENERIC_READ | GENERIC_WRITE);
		WriteFile(pService->handle, message.c_str(), (DWORD)message.length(), 0, NULL);
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

	return false;
}

static void ShutdownYaffeService(PlatformService* pService)
{
	picojson::object j;
	j["type"] = json((double)MESSAGE_TYPE_Quit);
	std::string message = json(j).serialize();

	std::lock_guard<std::mutex> guard(pService->mutex);
	OpenNamedPipe(&pService->handle, "\\\\.\\pipe\\yaffe", GENERIC_WRITE);
	WriteFile(pService->handle, message.c_str(), (DWORD)message.length(), 0, NULL);

	CloseHandle(pService->handle);
}

#pragma comment(lib, "urlmon.lib")
#include <urlmon.h>
static void DownloadImage(const char* pUrl, std::string pSlot)
{
	IStream* stream;
	//Also works with https URL's - unsure about the extent of SSL support though.
	HRESULT result = URLOpenBlockingStreamA(0, pUrl, &stream, 0, 0);
	Verify(result == 0, "Unable to retrieve image", ERROR_TYPE_Warning);

	static const u32 READ_AMOUNT = 100;
	HANDLE file = CreateFileA(pSlot.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
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