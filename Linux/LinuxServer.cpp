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

static void InitYaffeService()
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

static const char* PIPE_NAME = "./tmp/yaffe";
static bool SendServiceMessage(YaffeState* pState, YaffeMessage* pMessage, json* pResponse)
{
	const u32 size = Megabytes(2);
	char* buf = new char[size];
	ZeroMemory(buf, size);

	char message[4048];
	CreateServiceMessage(pMessage, message);
	YaffeLogInfo("Sending service message %s", message);

	{
		std::lock_guard<std::mutex> guard(pState->net_mutex);
		int write_handle;
		if (!OpenNamedPipe(&write_handle, PIPE_NAME, O_WRONLY | O_NONBLOCK)) return false;

		write(write_handle, message, strlen(message));
		close(write_handle);
	}
	

	std::lock_guard<std::mutex> guard(pState->net_mutex);
	int read_handle;
	if (!OpenNamedPipe(&read_handle, PIPE_NAME, O_RDONLY | O_NONBLOCK)) return false;

	bool success = false;
	if (read(read_handle, buf, size))
	{
		YaffeLogInfo("Received service reply");
		picojson::parse(*pResponse, buf);
		success = true;
		goto cleanup;
	}

	close(read_handle);
	free(buf);
	return success;
}

static void ShutdownYaffeService()
{
	char message[4048];
	YaffeMessage m;
	m.type = MESSAGE_TYPE_Quit;
	CreateServiceMessage(&m, message);

	{
		int write_handle;
		if (!OpenNamedPipe(&write_handle, PIPE_NAME, O_WRONLY | O_NONBLOCK)) return false;

		write(write_handle, message, strlen(message));
		close(write_handle);
	}
}

static void DownloadImage(const char* pUrl, AssetSlot* pSlot)
{
	//http://beej.us/guide/bgnet/html/
	//TODO shell out to wget?
}
