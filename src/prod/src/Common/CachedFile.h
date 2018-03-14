// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class CachedFile;
    typedef std::shared_ptr<CachedFile> CachedFileSPtr;
    typedef std::weak_ptr<CachedFile> CachedFileWPtr;

    typedef ErrorCode (*CachedFileReadCallback)(__in std::wstring const & filePath, __out std::wstring & content);

    class CachedFile: 
        protected TextTraceComponent<TraceTaskCodes::Common>,
        public std::enable_shared_from_this<CachedFile>
    {
        using TextTraceComponent<TraceTaskCodes::Common>::WriteInfo;
        DENY_COPY(CachedFile)

    public:
        static Common::ErrorCode Create(__in std::wstring const & filePath, __in CachedFileReadCallback const & readCallback, __out CachedFileSPtr & cachedFileSPtr);

        Common::ErrorCode ReadFileContent(__out std::wstring & content);

        virtual ~CachedFile(void);

    private:
        CachedFile(std::wstring const & filePath, CachedFileReadCallback const & readCallback);

        Common::ErrorCode EnableChangeMonitoring();

        // Callback for processing change notification
        void ProcessChangeNotification();
        Common::ErrorCode ReadFile_WithRetry();

        static const int MaxRetryIntervalInMillis;
        static const int MaxRetryCount;

    private:
        class ChangeMonitor;
        friend class ChangeMonitor;
        CachedFileReadCallback readCallback_;
        std::wstring filePath_;
        std::wstring content_;
        std::shared_ptr<ChangeMonitor> changeMonitor_;
        bool isContentValid_; 
        RwLock lock_; 
    };
}
