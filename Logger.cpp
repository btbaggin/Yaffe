#define DEBUG_LEVEL_None 0
#define DEBUG_LEVEL_Error 1
#define DEBUG_LEVEL_All 2

//Modify this define to control the level of logging
#define DEBUG_LEVEL DEBUG_LEVEL_All

#if DEBUG_LEVEL > DEBUG_LEVEL_Error
#define YaffeLogInfo Log
#else
#define LogInfo(message, ...)
#endif

#if DEBUG_LEVEL != DEBUG_LEVEL_None
#include <stdarg.h>
#define YaffeLogError Log
#define InitializeLogger() log_handle = fopen("log.txt", "w")
#define CloseLogger() fclose(log_handle)
#else
#define LogError(message, ...)
#define InitializeLogger()
#define CloseLogger()
#endif

FILE* log_handle = nullptr;
static void Log(const char* pMessage, ...)
{
	va_list args;
	va_start(args, pMessage);

	char buffer[1000];
	vsprintf(buffer, pMessage, args);

	char time[100];
	GetTime(time, 100, "{%x %X} ");
	fputs(time, log_handle);

	fputs(buffer, log_handle);
	fputc('\n', log_handle);

	fflush(log_handle);

	va_end(args);
}