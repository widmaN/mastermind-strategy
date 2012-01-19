//#include <intrin.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>

#include "CpuInfo.h"

using namespace Utilities;

static CpuInfo::CpuIdentifier get_identifier()
{
	int info[4];
	CpuInfo::CpuIdentifier ret;
	assert(sizeof(ret) == 16);
	__cpuid(info, 0);
	memcpy(&ret, info, 16);
	return ret;
}

static CpuInfo::CpuFeatures get_features()
{
	int info[4];
	CpuInfo::CpuFeatures ret;
	assert(sizeof(ret) == 16);
	__cpuid(info, 1);
	memcpy(&ret, info, 16);
	return ret;
}

const CpuInfo::CpuIdentifier CpuInfo::Identifier = get_identifier();
const CpuInfo::CpuFeatures CpuInfo::Features = get_features();

//const char * CpuInfo::GetIdentifier()
//{
//	static char id[13];
//	memcpy(&id[0], Identifier.Id1, 4);
//	memcpy(&id[4], Identifier.Id2, 4);
//	memcpy(&id[8], Identifier.Id3, 4);
//	id[12] = '\0';
//	return id;
//}
