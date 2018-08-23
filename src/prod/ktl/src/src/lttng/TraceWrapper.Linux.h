//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#define KTRACE_LEVEL_SILENT   0
#define KTRACE_LEVEL_CRITICAL 1
#define KTRACE_LEVEL_ERROR    2
#define KTRACE_LEVEL_WARNING  3
#define KTRACE_LEVEL_INFO     4
#define KTRACE_LEVEL_VERBOSE  5

void LttngTraceWrapper(
	const char * taskName,
	const char * eventName,
	int level,
	const char * id,
	const char * data);
#ifdef __cplusplus
}
#endif
