// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncFile::WriteFileAsyncOperation
        : public TimedAsyncOperation
        , public TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(WriteFileAsyncOperation);
        friend class WriteFileAsyncOverlapped;

    public:

        WriteFileAsyncOperation(
            HANDLE const& fileHandle,
            ByteBuffer inputBuffer,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(timeout,callback, parent)
            , writeStarted_(FALSE)
            , writeCompleted_(FALSE)
            , fileHandle_(fileHandle)
            , body_(inputBuffer)
            , winError_(ERROR_SUCCESS)
        {
            overlapped_ = { 0 };
        }

        ~WriteFileAsyncOperation();
        static ErrorCode End(__in AsyncOperationSPtr const & asyncOperation, __out DWORD & winError);

    protected:

        void OnWriteFileCompleted(__in AsyncOperationSPtr const & asyncOperation, ErrorCode errorCode, BOOL isTimeout);
        void OnWriteFileCancelled();
        void OnStart(AsyncOperationSPtr const& thisSPtr);
        void OnTimeout(AsyncOperationSPtr const& thisSPtr);
        void WriteFileComplete(ErrorCode errorCode, ULONG_PTR bytesCopied);
        void WriteFileComplete(DWORD errorCode, ULONG_PTR bytesCopied);
        static void WINAPI CompletedWriteRoutine(DWORD dwError, DWORD cbBytesWrote, LPOVERLAPPED lpOverLap);
    private:

        ErrorCode StartWriteFile(AsyncOperationSPtr const& operation);
        AsyncOperationSPtr thisSPtr_;
        ByteBuffer body_;
        ErrorCode operationStatus_;
        HANDLE const& fileHandle_;
        BOOL writeStarted_;
        BOOL writeCompleted_;
        OVERLAPPED overlapped_;
        DWORD winError_;
    };
}
