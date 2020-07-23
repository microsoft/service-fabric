// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <windns.h>
#include "Common/Environment.h"
#include "Common/Path.h"

using namespace std;
using namespace Common;

wstring FileVersion::GetCurrentExeVersion()
{
    return NumberVersionToString( GetCurrentExeVersionNumber() );  
}

unsigned __int64 FileVersion::GetCurrentExeVersionNumber()
{
    WCHAR fileName[MAX_PATH];
    if (GetModuleFileName(NULL, fileName, MAX_PATH) == 0)
    {
        Trace.WriteError("FileVersion", "Unable to get filename for the current exe because {0}", GetLastError());
        return 0;
    }
    
    return FileVersion::GetFileVersionNumber(fileName);
}

wstring FileVersion::GetFileVersion(wstring const & fileName)
{
    return NumberVersionToString( GetFileVersionNumber(fileName) );    
}

unsigned __int64 FileVersion::GetFileVersionNumber(wstring const & fileName, bool traceOnError)
{
    UINT BufLen;
    LPTSTR lpData; 
    VS_FIXEDFILEINFO *pFileInfo;
    DWORD dwHandle;
    DWORD dwLen = GetFileVersionInfoSize(fileName.c_str(), &dwHandle); 
    if (!dwLen)  
    {
        if (traceOnError)
        {
            Trace.WriteError("FileVersion", "Unable to get version size of {0} because {1}", fileName, GetLastError());
        }
        return 0;
    }

    ScopedHeap heap;
    lpData = (LPTSTR) heap.Allocate(dwLen);
    if (!lpData)
    {
        if (traceOnError)
        {
            Trace.WriteError("FileVersion", "Unable to allocate memory for version of fabric setup in scoped heap because {0}", GetLastError());
        }
        return 0;
    }

    if( !GetFileVersionInfo(fileName.c_str(), dwHandle, dwLen, lpData) ) 
    {  
        if (traceOnError)
        {
            Trace.WriteError("FileVersion", "Unable to get file version of fabric setup in scoped heap because {0}", GetLastError());
        }
        return 0;
    }

    if(!VerQueryValue( lpData, L"\\", (LPVOID *) &pFileInfo, (PUINT)&BufLen ))  
    {
        if (traceOnError)
        {
            Trace.WriteError("FileVersion", "Unable to query version information of fabric setup in scoped heap because {0}", GetLastError());
        }
        return 0;
    }

    unsigned __int64 value = pFileInfo->dwFileVersionMS;
    value = value << 32;
    return value + pFileInfo->dwFileVersionLS;
}

wstring FileVersion::NumberVersionToString(unsigned __int64 numVer)
{
    if (numVer == 0)
    {
        return L"";
    }

    return wformatString("{0}.{1}.{2}.{3}", 
        HIWORD( HIDWORD(numVer) ),
        LOWORD( HIDWORD(numVer) ),
        HIWORD( LODWORD(numVer) ),
        LOWORD( LODWORD(numVer) ));
}

