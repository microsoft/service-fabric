// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2 {
    class ProcessConsoleRedirector :
            public std::enable_shared_from_this<ProcessConsoleRedirector>,
            protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>,
            public Common::FabricComponent {
    public:
        ProcessConsoleRedirector(
                int &pipeHandle,
                std::string const &redirectedFileLocation,
                std::string const &redirectedFileNamePrefix,
                bool isOutFile,
                int maxRetentionCount,
                LONG maxFileSizeInKb);

        ~ProcessConsoleRedirector();

        static Common::ErrorCode CreateOutputRedirector(
                std::string const &redirectedFileLocation,
                std::string const &redirectedFileNamePrefix,
                int maxRetentionCount,
                LONG maxFileSizeInKb,
                int &stdoutPipeHandle,
                __out Hosting2::ProcessConsoleRedirectorSPtr &stdoutProcessRedirector);

        static Common::ErrorCode CreateErrorRedirector(
                std::string const &redirectedFileLocation,
                std::string const &redirectedFileNamePrefix,
                int maxRetentionCount,
                LONG maxFileSizeInKb,
                int &stderrPipeHandle,
                __out Hosting2::ProcessConsoleRedirectorSPtr &stderrProcessRedirector);

        static Common::ErrorCode ProcessConsoleRedirector::Create(
                std::string const & redirectedFileLocation,
                std::string const & redirectedFileNamePrefix,
                bool isOutFile,
                int maxRetentionCount,
                LONG maxFileSizeInKb,
                int & pipeHandle,
                __out ProcessConsoleRedirectorSPtr & processRedirector);

        typedef std::function<void(void)> WaitCallback;

        static const size_t BufferSize = 2048;
        static const LONG DeleteFileBackoffIntervalInMs = 500;
        static const LONG DeleteFileRetryMaxCount = 5;
        const int MaxFileRetentionCount;
        const size_t MaxFileSize;

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        bool isDisabled_;
        std::string redirectedFileLocation_;
        std::string fileName_;
        std::string redirectedFileNamePrefix_;
        bool isOutFile_;
        int redirectedFile_;
        int pipeHandle_;
        char buffer_[BufferSize];
        ULONG bytesRead_;
        ULONG totalBytesWritten_;

    };
}
