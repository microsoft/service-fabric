// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// Provider & Counter Set GUIDs
//
// Because of manifest registration conflicts, user & kernel cannot have the same GUIDs
//
// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa373092(v=vs.85).aspx for
// details on the manifest scheme
//

#pragma once

#if KTL_USER_MODE

// {A1E3BBB1-1E5D-4e95-9E79-952AEF1206C3}
static const GUID GUID_KTLLOGGER_COUNTER_PROVIDER = 
{ 0xa1e3bbb1, 0x1e5d, 0x4e95, { 0x9e, 0x79, 0x95, 0x2a, 0xef, 0x12, 0x6, 0xc3 } };

// {CA572CEF-72B1-4933-8449-F51FB094BC3B}
static const GUID GUID_KTLLOGGER_COUNTER_LOGSTREAM = 
{ 0xca572cef, 0x72b1, 0x4933, { 0x84, 0x49, 0xf5, 0x1f, 0xb0, 0x94, 0xbc, 0x3b } };

// {CB2A261E-CDB7-413e-A5DA-1BE3C3DFFF0C}
static const GUID GUID_KTLLOGGER_COUNTER_LOGCONTAINER = 
{ 0xcb2a261e, 0xcdb7, 0x413e, { 0xa5, 0xda, 0x1b, 0xe3, 0xc3, 0xdf, 0xff, 0xc } };

// {FF633C70-DF1B-4c22-BB64-4EBC0568BC38}
static const GUID GUID_KTLLOGGER_COUNTER_LOGMANAGER = 
{ 0xff633c70, 0xdf1b, 0x4c22, { 0xbb, 0x64, 0x4e, 0xbc, 0x5, 0x68, 0xbc, 0x38 } };
#else

// {11477D28-DCF0-442e-978C-BCBCB71F4AD1}
static const GUID GUID_KTLLOGGER_COUNTER_PROVIDER = 
{ 0x11477d28, 0xdcf0, 0x442e, { 0x97, 0x8c, 0xbc, 0xbc, 0xb7, 0x1f, 0x4a, 0xd1 } };


// {CAA1AE36-014A-4bbb-B6CB-00D57F60D8D3}
static const GUID GUID_KTLLOGGER_COUNTER_LOGSTREAM = 
{ 0xcaa1ae36, 0x14a, 0x4bbb, { 0xb6, 0xcb, 0x0, 0xd5, 0x7f, 0x60, 0xd8, 0xd3 } };

// {1441D310-9058-444c-AEF5-0E52A6248589}
static const GUID GUID_KTLLOGGER_COUNTER_LOGCONTAINER = 
{ 0x1441d310, 0x9058, 0x444c, { 0xae, 0xf5, 0xe, 0x52, 0xa6, 0x24, 0x85, 0x89 } };

// {263ACA85-798B-4de1-8B21-42F2607FFB5A}
static const GUID GUID_KTLLOGGER_COUNTER_LOGMANAGER = 
{ 0x263aca85, 0x798b, 0x4de1, { 0x8b, 0x21, 0x42, 0xf2, 0x60, 0x7f, 0xfb, 0x5a } };

#endif

#if KTL_USER_MODE
#define KTLLOGGER_COUNTER_LOGMANAGER_NAME L"Ktl Log Manager UM"
#define KTLLOGGER_COUNTER_LOGCONTAINER_NAME L"Ktl Log Container UM"
#define KTLLOGGER_COUNTER_LOGSTREAM_NAME L"KTL Log Stream UM"
#else
#define KTLLOGGER_COUNTER_LOGMANAGER_NAME L"Ktl Log Manager"
#define KTLLOGGER_COUNTER_LOGCONTAINER_NAME L"Ktl Log Container"
#define KTLLOGGER_COUNTER_LOGSTREAM_NAME L"Ktl Log Stream"
#endif


typedef struct 
{
    ULONGLONG ApplicationBytesWritten;
    ULONGLONG DedicatedBytesWritten;
    ULONGLONG SharedBytesWritten;
    ULONGLONG DedicatedLogBacklogLimitBytes;
    ULONGLONG DedicatedLogBacklogBytes;
    ULONGLONG SharedLogQuota;
    ULONGLONG SharedLogQuotaInUse;
} LogStreamPerfCounterData;

typedef struct 
{
    LONGLONG DedicatedBytesWritten;
    LONGLONG SharedBytesWritten;
} LogContainerPerfCounterData;

typedef struct
{
    ULONGLONG WriteBufferMemoryPoolLimit;
    ULONGLONG WriteBufferMemoryPoolInUse;
} LogManagerPerfCounterData;
