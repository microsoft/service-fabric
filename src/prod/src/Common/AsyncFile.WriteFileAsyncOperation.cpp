// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Common;
using namespace std;

Common::StringLiteral const TraceType("File.WriteFileAsyncOperation");

namespace Common
{
    AsyncFile::WriteFileAsyncOperation::~WriteFileAsyncOperation()
    {
    }
   
    void AsyncFile::WriteFileAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
    {
        if (TimedAsyncOperation::get_RemainingTime() <= TimeSpan::Zero)
        {
            this->OnWriteFileCompleted(thisSPtr, ErrorCodeValue::Timeout, TRUE);
        }
        else
        {
            TimedAsyncOperation::OnStart(thisSPtr);

            // Start To Write file
            operationStatus_ = this->StartWriteFile(thisSPtr);
            if (!operationStatus_.IsSuccess())
            {
                this->OnWriteFileCompleted(thisSPtr, operationStatus_, FALSE);
            }
        } 
    }

    void WINAPI AsyncFile::WriteFileAsyncOperation::CompletedWriteRoutine(DWORD errorCode, DWORD u32_BytesTransfered, LPOVERLAPPED overlapped)
    {
        AsyncFile::WriteFileAsyncOperation* pThis = (AsyncFile::WriteFileAsyncOperation*)overlapped->hEvent;
        pThis->WriteFileComplete(errorCode, u32_BytesTransfered);
    }

    ErrorCode AsyncFile::WriteFileAsyncOperation::StartWriteFile(AsyncOperationSPtr const&)
    {
        BOOL success = FALSE;

        //Capturing the thisSPtr to keep the AsyncOperation object alive until the async I/O completes.
        thisSPtr_ = shared_from_this();

        writeStarted_ = TRUE;
        overlapped_.hEvent = this;
        // Start Write file with overlapped I/O
        success = WriteFileEx(
            fileHandle_,
            body_.data(),
            static_cast<DWORD>(body_.size()),
            &overlapped_,
            (LPOVERLAPPED_COMPLETION_ROUTINE)CompletedWriteRoutine);
        
        if (!success)
        {
            DWORD lastError = GetLastError();
            if (lastError != ERROR_IO_PENDING)
            {
                WriteWarning(TraceType, "Writing file failed with error {1}.Cancelling Thread Pool IO.", lastError);
                winError_ = lastError;
                return ErrorCode::FromWin32Error(lastError);
            }
        }
        return ErrorCode::Success();
    }

    void AsyncFile::WriteFileAsyncOperation::WriteFileComplete(DWORD errorCode, ULONG_PTR bytesCopied)
    {
        winError_ = errorCode;
        WriteFileComplete(ErrorCode::FromWin32Error(errorCode), bytesCopied);
    }

    void AsyncFile::WriteFileAsyncOperation::WriteFileComplete(ErrorCode errorCode, ULONG_PTR)
    {
        if (errorCode.IsSuccess())
        {
            WriteInfo(TraceType, "Write File Async completed for.");
            writeCompleted_ = TRUE;

            // Call TryComplete with Success Error code 
            this->OnWriteFileCompleted(thisSPtr_, ErrorCode::Success(), FALSE);
        }
        else
        {
            this->OnWriteFileCompleted(thisSPtr_, ErrorCode::FromWin32Error(), FALSE);
        }
    }

    void AsyncFile::WriteFileAsyncOperation::OnWriteFileCompleted(AsyncOperationSPtr const& thisSPtr, ErrorCode errorCode, BOOL isTimeout)
    {
        TimedAsyncOperation::OnCompleted();
        WriteInfo(TraceType, "Calling Try Complete with error code {0}.", errorCode);
        this->TryComplete(thisSPtr, errorCode);
        if (!isTimeout){
          thisSPtr_ = nullptr;    
        }        
    }

    void AsyncFile::WriteFileAsyncOperation::OnWriteFileCancelled()
    {
        BOOL success = ::CancelIoEx(fileHandle_, &overlapped_);
        if (success)
        {
            WriteInfo(TraceType, "Write File Async cancelled successfully.");
        }
        else
        {
            WriteWarning(TraceType, "Error {0} in cancelling Write file I/O call.", ErrorCode::FromWin32Error());
        }
    }

    void AsyncFile::WriteFileAsyncOperation::OnTimeout(AsyncOperationSPtr const& thisSPtr)
    {
        if (writeStarted_ && !writeCompleted_)
        {
            this->OnWriteFileCancelled();
            this->OnWriteFileCompleted(thisSPtr, ErrorCodeValue::Timeout, TRUE);
        }
        TimedAsyncOperation::OnTimeout(thisSPtr);
    }

    ErrorCode AsyncFile::WriteFileAsyncOperation::End(__in Common::AsyncOperationSPtr const & asyncOperation, __out DWORD & winError)
    {
        auto operation = AsyncOperation::End<WriteFileAsyncOperation>(asyncOperation);
        winError = operation->winError_;
        return operation->Error;
    }
}
