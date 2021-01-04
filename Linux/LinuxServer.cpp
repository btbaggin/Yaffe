struct PlatformService
{
	int handle;
	std::mutex mutex;
};

int IsProcessRunning(std::string procName)
{
    int pid = -1;

    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL)
    {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp)))
        {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0)
            {
                // Read contents of virtual /proc/{pid}/cmdline file
                std::string cmdPath = std::string("/proc/") + dirp->d_name + "/cmdline";
                std::ifstream cmdFile(cmdPath.c_str());
                std::string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != std::string::npos) cmdLine = cmdLine.substr(0, pos);

                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != std::string::npos) cmdLine = cmdLine.substr(pos + 1);

                    // Compare against requested process name
                    if (procName == cmdLine) pid = id;
                }
            }
        }
    }

    closedir(dp);

    return pid;
}

static void InitYaffeService(PlatformService* pService)
{
	if (IsProcessRunning("YaffeService") == -1)
	{
		//TODO add back, async
		// switch(fork())
		// {
		// 	case -1:
		// 		YaffeLogError("Unable to start YaffeService");
		// 		break;

		// 	case 0:
		// 		execl("YaffeService", NULL);
		// 		break;
		// }
	}
}

static bool OpenNamedPipe(int* pHandle, const char* pPath, int pAccess)
{
	if (!mkfifo(pPath, 0666)) return false;
	*pHandle = open(pPath, pAccess);
	return *pHandle != -1;
}

static bool SendServiceMessage(PlatformService* pService, YaffeMessage* pMessage, json* pResponse)
{
	const u32 size = Megabytes(2);
	char* buf = new char[size];
	ZeroMemory(buf, size);

	static const char* path = "./tmp/yaffe";
	char message[4048];
	CreateServiceMessage(pMessage, message);
	YaffeLogInfo("Sending service message %s", message);

	{
		std::lock_guard<std::mutex> guard(pService->mutex);
		int write_handle;
		if (!OpenNamedPipe(&write_handle, path, O_WRONLY | O_NONBLOCK)) return false;

		write(pService->handle, message, strlen(message));
		close(write_handle);
	}
	

	std::lock_guard<std::mutex> guard(pService->mutex);
	int read_handle;
	if (!OpenNamedPipe(&read_handle, path, O_RDONLY | O_NONBLOCK)) return false;

	if (read(pService->handle, buf, size))
	{
		YaffeLogInfo("Received service reply");
		picojson::parse(*pResponse, buf);
		close(read_handle);
		free(buf);
		return true;
	}

	close(read_handle);
	free(buf);
	return false;
}

static void ShutdownYaffeService(PlatformService* pService)
{
	close(pService->handle);
}

static void DownloadImage(const char* pUrl, AssetSlot* pSlot)
{
	//TODO shell out to wget?
}
