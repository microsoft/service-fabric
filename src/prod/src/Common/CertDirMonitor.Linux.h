// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class CertDirMonitor : TextTraceComponent<TraceTaskCodes::Common>
    {
    DENY_COPY(CertDirMonitor)

    public:
        static const char     *CertFileExtsA[];
        static const wchar_t  *CertFileExtsW[];
        static const int NrCertFileExts;

    public:
        static CertDirMonitor & GetSingleton();
        CertDirMonitor();

        ErrorCode StartWatch(const string &srcPath, const string &destPath);
        ErrorCode StopWatch();

    private:
        wstring srcPath_;
        wstring destPath_;
        int wd_;

    private:
        void WaitForNextChange();
        void RetryWithDelay();
        static void OnAioComplete(sigval_t sigval);
        void ProcessNotifications();
        bool IsCertFile(const string &fileName);

        int inotifyFd_;
        struct aiocb aiocb_;
        ByteBuffer eventBuffer_;

        static BOOL InitFunction(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE initOnce_;
        static Global<CertDirMonitor> singleton_;
    };
}
