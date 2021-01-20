#include <stdio.h>
#include <tchar.h>
#include <TlHelp32.h>

static const char* PIPE_NAME = "\\\\.\\\\pipe\\\\yaffe";
static bool OpenNamedPipe(HANDLE* pHandle, DWORD pAccess, int pSleep = 1000)
{
	*pHandle = INVALID_HANDLE_VALUE;
	for (u32 i = 0; i < 3 && *pHandle == INVALID_HANDLE_VALUE; i++)
	{
		*pHandle = CreateFileA(PIPE_NAME, pAccess, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (*pHandle == INVALID_HANDLE_VALUE) Sleep(pSleep);
	}

	return *pHandle != INVALID_HANDLE_VALUE;
}

static void InitYaffeService()
{
	HANDLE handle;
	if (!OpenNamedPipe(&handle, GENERIC_READ, 10))
	{
		STARTUPINFOA si = {};
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_MINIMIZE;
		PROCESS_INFORMATION pi = {};
		if (!CreateProcessA("YaffeService.exe", NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			YaffeLogError("Unable to start YaffeService.exe");
		}
	}
	else
	{
		CloseHandle(handle);
	}
}

static bool SendServiceMessage(YaffeState* pState, YaffeMessage* pMessage, json* pResponse)
{
	if (!g_state.has_connection) return false;

	const u32 size = Megabytes(2);
	char* buf = new char[size];
	ZeroMemory(buf, size);

	HANDLE handle;
	{
		char message[4048];
		CreateServiceMessage(pMessage, message);

		std::lock_guard<std::mutex> guard(pState->net_mutex);
		if (!OpenNamedPipe(&handle, GENERIC_READ | GENERIC_WRITE))
		{
			YaffeLogError("Unable to open named pipe: %d", GetLastError());
			return false;
		}

		YaffeLogInfo("Sending service message %s", message);
		WriteFile(handle, message, (DWORD)strlen(message), 0, NULL);
	}

	bool success = false;
	do
	{
		std::lock_guard<std::mutex> guard(pState->net_mutex);
		if (ReadFile(handle, buf, size, 0, NULL))
		{
			YaffeLogInfo("Received service reply: %s", buf);
			picojson::parse(*pResponse, buf);
			success = true;
			goto cleanup;
		}
	} while (GetLastError() == ERROR_MORE_DATA);

cleanup:
	free(buf);
	CloseHandle(handle);
	return success;
}

static void ShutdownYaffeService()
{
	char message[4048];
	YaffeMessage m;
	m.type = MESSAGE_TYPE_Quit;
	CreateServiceMessage(&m, message);

	HANDLE handle;
	if (OpenNamedPipe(&handle, GENERIC_WRITE))
	{
		WriteFile(handle, message, (DWORD)strlen(message), 0, NULL);
	}

	CloseHandle(handle);
}

#pragma comment(lib, "urlmon.lib")
#include <urlmon.h>
static void DownloadImage(const char* pUrl, AssetSlot* pSlot)
{
	YaffeLogInfo("Downloading image from %s", pUrl);

	IStream* stream;
	//Also works with https URL's - unsure about the extent of SSL support though.
	HRESULT result = URLOpenBlockingStreamA(0, pUrl, &stream, 0, 0);
	Verify(result == 0, "Unable to retrieve image from URL", ERROR_TYPE_Warning);

	static const u32 READ_AMOUNT = 100;
	HANDLE file = CreateFileA(pSlot->load_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	Verify(file, "Unable to create image file", ERROR_TYPE_Warning);

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