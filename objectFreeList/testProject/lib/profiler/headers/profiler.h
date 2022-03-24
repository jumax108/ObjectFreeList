#pragma once

#include <stdio.h>
#include <Windows.h>

#include "dump/headers/dump.h"
#pragma comment(lib, "lib/dump/dump")

#include "log/headers/log.h"
#pragma comment(lib, "lib/log/log")

#include "common.h"

class CProfiler
{
public:

	struct Profile {
		char name[100] = {0,};
		LARGE_INTEGER start;
		__int64 sum = 0;
		__int64 max = 0;
		__int64 min = 0x7FFFFFFFFFFFFFFF;
		unsigned int callCnt = 0;
	};

	LARGE_INTEGER freq;

	CLog logger;

public:
	
	CProfiler();

	void begin(const char name[100]);
	void end(const char name[100]);

	void printToFile();
	
private:

	unsigned int tlsIndex = 0;
	unsigned short allocIndex = 0;

	Profile profile[profiler::MAX_THREAD_NUM][profiler::MAX_PROFILE_NUM]; 

	Profile* getTlsProfileData();
	int findIdx(const char* name, Profile* profile);
};