#pragma once
#ifndef _HELPER_H_
#define _HELPER_H_

#include "stdafx.h"
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
//#include <filesystem>
using namespace std;

typedef enum 
{
	WriteBack = 0,
	WriteThroughAllocate,
	WriteThroughNonAllocate
}wstrategies;
extern wstrategies writeStrategies;

typedef enum
{
	LRUsed = 0,
	RoundRobin
}RStrategies;
extern RStrategies ReplaceStategies;
typedef struct 
{
	uint32_t ReadMemoryCnt;
	uint32_t ReadLineCnt;
	uint32_t ReadLineHitCnt;	
	uint32_t ReadLineDirtyCnt;
	uint32_t ReadLineReplaceCnt;

	uint32_t WriteMemoryCnt;
	uint32_t WriteLineCnt;
	uint32_t WriteLineHitCnt;
	uint32_t WriteLineDirtyCnt;
	uint32_t WriteLineReplaceCnt;
	uint32_t FlushDirtyCount;		
	uint32_t WriteThroughMemoryCnt;
}counts;
extern counts Counter;


#endif // !_HELPER_H_