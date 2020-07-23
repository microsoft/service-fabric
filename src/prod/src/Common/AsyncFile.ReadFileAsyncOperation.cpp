// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Common;
using namespace std;

Common::StringLiteral const TraceType("File.ReadFileAsyncOperation");

namespace Common
{
    AsyncFile::ReadFileAsyncOperation::~ReadFileAsyncOperation()
    {
    }
   
    void AsyncFile::ReadFileAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
    {
        ErrorCode error;

        error = CreateFileHandle();
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, "Could not open file {0}. Error:{1}", filePath_, error);
            this->OnReadFileCompleted(thisSPtr, error);
            return;
        }

        error = threadpoolIO_.Open(fileHandleUPtr_->get_Value());
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType,"Failed to open threadpool io for file :{0}. Error : {1}", filePath_, error);
            this->OnReadFileCompleted(thisSPtr, error);
            return;
        }

        if (TimedAsyncOperation::get_RemainingTime() <= TimeSpan::Zero)
        {
            this->OnReadFileCompleted(thisSPtr, ErrorCodeValue::Timeout);
        }
        else
        {
            TimedAsyncOperation::OnStart(thisSPtr);

            // Get File Size
            operationStatus_ = this->GetFileSize();

            if (operationStatus_.IsSuccess())
            {
                // Start To Read file
                operationStatus_ = this->StartReadFile(thisSPtr);

                if (!operationStatus_.IsSuccess())
                {
                    this->OnReadFileCompleted(thisSPtr, operationStatus_);
                }
            }
            else
            {
                this->OnReadFileCompleted(thisSPtr, operationStatus_);
            }
        } 
    }

    ErrorCode AsyncFile::ReadFileAsyncOperation::GetFileSize()
    {
        LARGE_INTEGER fileSize = { 0 };
        BOOL success = GetFileSizeEx(fileHandleUPtr_->get_Value(), &fileSize);

        if (!success)
        {
            ErrorCode error = ErrorCode::FromWin32Error();
            WriteWarning(TraceType, "Getting file size failed for {0} with error{1}.", filePath_, error);
            return error;
        }

        fileSize_ = fileSize.LowPart;
        
        // If file Size is greater than 1 GB  return with an error
        if (fileSize_ > maxFileSize_)
        {
            ErrorCode error = ErrorCodeValue::EntryTooLarge;
            WriteWarning(TraceType, "FileSize for {0} exceeds allowed limit of 1 GB .Failing with error{1}.", filePath_, error);
            return error;
        }

        return ErrorCode::Success();
    }

    ErrorCode AsyncFile::ReadFileAsyncOperation::CreateFileHandle()
    {
        HANDLE fileHandle = nullptr;

        // Get File Handle
        fileHandle = CreateFile(filePath_.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL);

        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            return ErrorCode::FromWin32Error();
        }

        fileHandleUPtr_ = make_unique<Handle>(fileHandle);
        return ErrorCode::Success();
    }

    ErrorCode AsyncFile::ReadFileAsyncOperation::StartReadFile(AsyncOperationSPtr const&)
    {
        BOOL success = FALSE;
        DWORD bytesRead = 0;
        body_.resize(fileSize_);

        //Capturing the thisSPtr to keep the AsyncOperation object alive until the async I/O completes.
        thisSPtr_ = shared_from_this();

        // Start Thread Pool IO
        threadpoolIO_.StartIo();
        readStarted_ = TRUE;

        // Start Read file with overlapped I/O
        success = ReadFile(
            fileHandleUPtr_->get_Value(),
            body_.data(),
            fileSize_,
            &bytesRead,
            readFileAsyncOverlapped_.OverlappedPtr());
        
        if (!success)
        {
            DWORD lastError = GetLastError();
            if (lastError != ERROR_IO_PENDING)
            {
                WriteWarning(TraceType, "Reading file failed for {0} with error {1}.Cancelling Thread Pool IO.", filePath_, lastError);
                threadpoolIO_.CancelIo();
                return ErrorCode::FromWin32Error(lastError);
            }
        }
        return ErrorCode::Success();
    }

    void AsyncFile::ReadFileAsyncOperation::ReadFileComplete(PTP_CALLBACK_INSTANCE pci,ErrorCode errorCode, ULONG_PTR)
    {
        // We're adding this call to prevent the deadlock situation where the same thread that calls waitforthreadpools is awaited. 
        DisassociateCurrentThreadFromCallback(pci);

        if (errorCode.IsSuccess())
        {
            WriteInfo(TraceType, "Read File Async completed for {0}.", filePath_);
            readCompleted_ = TRUE;

            // Call TryComplete with Success Error code 
            this->OnReadFileCompleted(thisSPtr_, ErrorCode::Success());
        }
        else
        {
            this->OnReadFileCompleted(thisSPtr_, ErrorCode::FromWin32Error());
        }
    }

    void AsyncFile::ReadFileAsyncOperation::OnReadFileCompleted(AsyncOperationSPtr const& thisSPtr, ErrorCode errorCode)
    {
        threadpoolIO_.Close();
        fileHandleUPtr_ = nullptr;
        WriteInfo(TraceType, "Calling Try Complete with error code {0} in reading file {1}", errorCode, filePath_);
        TimedAsyncOperation::OnCompleted();
        this->TryComplete(thisSPtr, errorCode);
        thisSPtr_ = nullptr;
    }

    void AsyncFile::ReadFileAsyncOperation::OnReadFileCancelled()
    {
        BOOL success = ::CancelIoEx(fileHandleUPtr_->get_Value(), NULL);
        if (success)
        {
            WriteInfo(TraceType, "Read File Async cancelled successfully for {0}.", filePath_);
        }
        else
        {
            WriteWarning(TraceType, "Error {0} in cancelling Read file I/O call for {1}", ErrorCode::FromWin32Error(), filePath_);
        }
    }

    void AsyncFile::ReadFileAsyncOperation::OnTimeout(AsyncOperationSPtr const& thisSPtr)
    {
        if (readStarted_ && !readCompleted_)
        {
            this->OnReadFileCancelled();
            this->threadpoolIO_.CancelIo();
            this->OnReadFileCompleted(thisSPtr, ErrorCodeValue::Timeout);
        }
        TimedAsyncOperation::OnTimeout(thisSPtr);
    }

    ErrorCode AsyncFile::ReadFileAsyncOperation::End(__in Common::AsyncOperationSPtr const & asyncOperation, __out ByteBuffer &buffer)
    {
        auto operation = AsyncOperation::End<ReadFileAsyncOperation>(asyncOperation);

        if (operation->Error.IsSuccess())
        {
            buffer = move(operation->body_);
        }
        return operation->Error;
    }
}
