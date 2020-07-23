// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef _APITHREADSTRESS_H_
#define _APITHREADSTRESS_H_

#include "utilcode.h"

class APIThreadStress
{
 public:
    APIThreadStress();
    ~APIThreadStress();

    BOOL DoThreadStress();
    static void SyncThreadStress();

    static void SetThreadStressCount(int count);

 protected:
    virtual void Invoke() {LIMITED_METHOD_CONTRACT;};

 private:
    static DWORD WINAPI StartThread(void *arg);

    static int s_threadStressCount;     

    int       m_threadCount;
    HANDLE    *m_hThreadArray;
    BOOL      m_setupOK;
    LONG      m_runCount;
    HANDLE    m_syncEvent;

};

#endif  // _APITHREADSTRESS_H_
