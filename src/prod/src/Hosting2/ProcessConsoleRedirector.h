// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ProcessConsoleRedirector:
         public std::enable_shared_from_this<ProcessConsoleRedirector>,
         protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>,
         public Common::FabricComponent
    {
    public:        
        ProcessConsoleRedirector(
            Common::HandleUPtr & pipeHandle, 
            std::wstring const & redirectedFileLocation,
            std::wstring const & redirectedFileNamePrefix,
            bool isOutFile,
            int maxRetentionCount,
            LONG maxFileSizeInKb);                
        ~ProcessConsoleRedirector();

        static Common::ErrorCode CreateOutputRedirector(
            std::wstring const & redirectedFileLocation, 
            std::wstring const & redirectedFileNamePrefix, 
            int maxRetentionCount,
            LONG maxFileSizeInKb,
            __out Common::HandleUPtr & stdoutPipeHandle, 
            __out Hosting2::ProcessConsoleRedirectorSPtr & stdoutProcessRedirector);
        static Common::ErrorCode CreateErrorRedirector(
            std::wstring const & redirectedFileLocation, 
            std::wstring const & redirectedFileNamePrefix, 
            int maxRetentionCount,
            LONG maxFileSizeInKb,
            __out Common::HandleUPtr & stderrPipeHandle, 
            __out Hosting2::ProcessConsoleRedirectorSPtr & stderrProcessRedirector);
        
        typedef std::function<void(void)> WaitCallback;

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:        
        class ThreadpoolWaitImpl;
        typedef std::shared_ptr<ThreadpoolWaitImpl> ThreadpoolWaitImpSPtr;

        static Common::ErrorCode Create(
            std::wstring const & redirectedFileLocation, 
            std::wstring const & redirectedFileNamePrefix, 
            bool isOutFile, 
            int maxRetentionCount,
            LONG maxFileSizeInKb,
            __out Common::HandleUPtr & pipeHandle, 
            __out Hosting2::ProcessConsoleRedirectorSPtr & processRedirector);

        static std::wstring GetPipeName(std::wstring const & redirectedFileNamePrefix, bool isOutFile);
        bool ReRegisterThreadpoolWait(Common::HandleUPtr const & eventHandle, ThreadpoolWaitImpSPtr const & threadPoolWait);
        
        Common::ErrorCode SetRedirectionFile(bool isOpening);
        Common::ErrorCode CreateFileForRedirection(bool isOpening);
        Common::ErrorCode DeleteRedirectionFile(std::wstring const & fileName, int deleteTrialCount, bool isOpening);
        void OnFileWriteComplete();
        void OnPipeReadComplete();
        Common::ErrorCode ReadPipe();
        Common::ErrorCode WriteFile();

        static const size_t BufferSize = 2048;
        static const LONG DeleteFileBackoffIntervalInMs = 500;
        static const LONG DeleteFileRetryMaxCount = 5;
        const int MaxFileRetentionCount;
        const size_t MaxFileSize;        

        std::wstring redirectedFileLocation_;
        std::wstring fileName_;
        std::wstring redirectedFileNamePrefix_;
        bool isOutFile_;
        std::unique_ptr<Common::File> redirectedFile_;
        Common::HandleUPtr pipeHandle_;
        Common::HandleUPtr pipeReadWaitHandle_;
        Common::HandleUPtr fileWriteWaitHandle_;        
        ThreadpoolWaitImpSPtr pipeReadWait_;
        ThreadpoolWaitImpSPtr fileWriteWait_;        
        OVERLAPPED readOverlapped_;
        OVERLAPPED writeOverlapped_;
        char buffer_[BufferSize];        
        ULONG bytesRead_;
        ULONG totalBytesWritten_; 
        Common::TimerSPtr deleteTimer_;
    };
}
