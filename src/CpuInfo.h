#pragma once

#include <intrin.h>

namespace Utilities
{
	class CpuInfo
	{
	public:
		enum CpuFeature
		{
			CPU_FEATURE_SSE41 = (1 << 19),
			CPU_FEATURE_SSE42 = (1 << 20),
		};

	public:
		struct CpuIdentifier
		{
			int MaxInfo;
			char Id1[4];
			char Id3[4];
			char Id2[4];
		};

		static const CpuIdentifier Identifier;

		struct CpuFeatures
		{
			// byte 0-1
			int SteppingID		: 4;
			int Model			: 4;
			int Family			: 4;
			int ProcessorType	: 2;
			int _reserved1		: 2;

			// byte 2-3
			int ExtendedModel	: 4;
			int ExtendedFamily	: 8;
			int _reserved2		: 4;

			// byte 4-7
			int BrandIndex		: 8;
			int CacheLineSizeQW	: 8;
			int LogicalProcessorCount : 8;
			int InitialApicId	: 8;

			// byte 8
			bool WithSSE3		: 1;
			bool WithPCLMULQDQ	: 1;
			bool With64BitDS	: 1;
			bool WithMonitor	: 1;
			bool WithCplQualifiedDebugStore : 1;
			bool WithVirtualMachine : 1;
			bool WithSaferMode	: 1;
			bool WithSpeedStep	: 1;

			// byte 9
			bool WithThermalMonitor2 : 1;
			bool WithSSSE3		: 1;
			bool L1ContextId	: 1;
			bool _reserved4		: 1;
			bool _reserved5		: 1;
			bool WithCMPXCHG16B	: 1;
			bool WithXtprUpdateControl : 1;
			bool WithPerfMonAndDebug : 1;

			// byte 10
			bool _reserved6		: 1;
			bool WithProcessContextId : 1;
			bool WithMemoryMapPrefetch : 1;
			bool WithSSE41		: 1;
			bool WithSSE42		: 1;
			bool WithX2APIC		: 1;
			bool WithMOVBE		: 1;
			bool WithPOPCNT		: 1;

			// byte 11
			bool _reserved7		: 1;
			bool WithAESNI		: 1;
			bool WithXSAVE		: 1;
			bool WithOSXSAVE	: 1;
			bool WithAVX		: 1;
			bool _reserved8		: 1;
			bool _reserved9		: 1;
			bool _reserved10	: 1;

			// byte 12-15
			char _padding[4];
		};

		static const CpuFeatures Features;

	public:

		// static const char *GetIdentifier();

	};
}