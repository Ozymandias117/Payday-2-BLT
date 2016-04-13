#include "util/util.h"
#include <ctime>
#include <fstream>
#include <iostream>

void Logging::Log(std::string msg, LogType mType)
{
	std::time_t currentTime = time(0);
	struct std::tm now = *localtime(&currentTime);

	char datestring[100];
	std::strftime(datestring, sizeof(datestring), "%F", &now);


	std::string fPath = "mods/logs/" + std::string(datestring) + "_log.txt";
	std::ofstream mFile;
	mFile.open(fPath.c_str(), std::ios::out | std::ios::app);

	std::strftime(datestring, sizeof(datestring), "%X", &now);

	mFile << datestring;
	mFile << " ";

	switch (mType)
	{
	case LOGGING_LOG:
		mFile << "Log: ";
		break;
	case LOGGING_LUA:
		mFile << "Lua: ";
		break;
	case LOGGING_WARN:
		mFile << "WARNING: ";
		break;
	case LOGGING_ERROR:
		mFile << "FATAL ERROR: ";
		break;
	default:
		mFile << "Message: ";
		break;
	}

	mFile << msg.c_str();
	mFile << "\n";
	mFile.close();
	printf("%s\n", msg.c_str());
}
