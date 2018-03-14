// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <bool IsReaderLock>
    class FileLock : public TextTraceComponent<TraceTaskCodes::Common>
    {
    public:
        FileLock(std::wstring const & path);
        virtual ~FileLock();

        virtual ErrorCode Acquire(TimeSpan timeout = TimeSpan::Zero);
        virtual bool Release();

    protected:

#ifdef PLATFORM_UNIX

        std::wstring const id_;
        ErrorCode openStatus_;
        std::string path_;
        int file_;

#else

        static std::wstring GetReaderLockPath(std::wstring const & path);
        static std::wstring GetWriterLockPath(std::wstring const & path);        
        static const std::wstring ReaderLockExtension;
        static const std::wstring WriterLockExtension;

        ErrorCode AcquireImpl();

        std::unique_ptr<File> readFile_;
        std::unique_ptr<File> writeFile_;
        std::wstring path_;
        std::wstring operation_;

#endif
    };

}
