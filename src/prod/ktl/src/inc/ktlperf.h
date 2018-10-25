/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:

    RvdPerf.h

Abstract:

    This header file defines the global RVD performance counters

Author:

    Norbert Kusters (norbertk) 9-Feb-2012

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#pragma once

//
// The ID for the per-instance cached block file counters.  {F4AACC4B-660D-4f56-8694-5320E3B1E2B2}
//

static const GUID KtlPerInstanceCachedBlockFileCountersId = { 0xf4aacc4b, 0x660d, 0x4f56, { 0x86, 0x94, 0x53, 0x20, 0xe3, 0xb1, 0xe2, 0xb2 } };
static const WCHAR* KtlPerInstanceCachedBlockFileCountersIdString = L"Perf{F4AACC4B-660D-4f56-8694-5320E3B1E2B2}";
