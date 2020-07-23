// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef _CIRCULARLOG_H__

#define _CIRCULARLOG_H__

#include "sstring.h"

class CircularLog
{
public:
    CircularLog();
    ~CircularLog();
    
    bool Init(const WCHAR* logname, const WCHAR* logHeader, DWORD maxSize = 1024*1024);
    void Shutdown();
    void Log(const WCHAR* string);
  
protected:

    void   CheckForLogReset(BOOL fOverflow);
    BOOL   CheckLogHeader();
    HANDLE OpenFile();    
    void   CloseFile();

    bool            m_bInit;
    SString         m_LogFilename;
    SString         m_LogHeader;
    SString         m_OldLogFilename;
    SString         m_LockFilename;
    DWORD           m_MaxSize;
    unsigned        m_uLogCount;
};

#endif
