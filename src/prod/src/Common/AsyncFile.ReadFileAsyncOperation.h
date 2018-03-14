// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncFile::ReadFileAsyncOperation
        : public TimedAsyncOperation
        , public TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(ReadFileAsyncOperation);
        friend class ReadFileAsyncOverlapped;

    public:

        ReadFileAsyncOperation(
            std::wstring const &filePath,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(timeout,callback, parent)
            , filePath_(filePath)
            , readStarted_(FALSE)
            , readCompleted_(FALSE)
            , readFileAsyncOverlapped_(*this)
            , fileHandleUPtr_(nullptr)
            , maxFileSize_(FabricConstants::MaxFileSize)
        {
        }

        ~ReadFileAsyncOperation();
        static ErrorCode End(__in AsyncOperationSPtr const & asyncOperation, __out Common::ByteBuffer &buffer);  

    protected:

        void OnReadFileCompleted(__in AsyncOperationSPtr const & asyncOperation, ErrorCode errorCode);
        void OnReadFileCancelled();
        void OnStart(AsyncOperationSPtr const& thisSPtr);
        void OnTimeout(AsyncOperationSPtr const& thisSPtr);
        void ReadFileComplete(PTP_CALLBACK_INSTANCE pci,ErrorCode errorCode, ULONG_PTR bytesCopied);

    private:

        ErrorCode StartReadFile(AsyncOperationSPtr const& operation);
        ErrorCode GetFileSize();
        ErrorCode CreateFileHandle();
        AsyncOperationSPtr thisSPtr_;
        ByteBuffer body_;
        ErrorCode operationStatus_;
        std::wstring filePath_;
        HandleUPtr fileHandleUPtr_;
        DWORD fileSize_;
        BOOL readStarted_;
        BOOL readCompleted_;
        ThreadpoolIo threadpoolIO_;
        ReadFileAsyncOverlapped readFileAsyncOverlapped_;
        DWORD maxFileSize_;
    };
}
