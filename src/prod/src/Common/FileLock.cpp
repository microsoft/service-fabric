// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    using namespace std;

    static Common::StringLiteral const FileLockSource = "FileLock";

    template <bool IsReaderLock>
    const std::wstring FileLock<IsReaderLock>::ReaderLockExtension = L".ReaderLock";
    template <bool IsReaderLock>
    const std::wstring FileLock<IsReaderLock>::WriterLockExtension = L".WriterLock";

    template <bool IsReaderLock>
    FileLock<IsReaderLock>::FileLock(std::wstring const & path)
        : readFile_(), 
          writeFile_(),
          path_(path),
          operation_(IsReaderLock ? L"reader" : L"writer")
    {
    }

    template <bool IsReaderLock>
    FileLock<IsReaderLock>::~FileLock()
    {
        Release();
    }

    template <bool IsReaderLock>
    std::wstring FileLock<IsReaderLock>::GetReaderLockPath(std::wstring const & path)
    {
        return path + FileLock::ReaderLockExtension;
    }

    template <bool IsReaderLock>
    std::wstring FileLock<IsReaderLock>::GetWriterLockPath(std::wstring const & path)
    {
        return path + FileLock::WriterLockExtension;
    }
    
    template <bool IsReaderLock>
    ErrorCode FileLock<IsReaderLock>::AcquireImpl()
    {   
        unique_ptr<File> localReadFile = make_unique<File>();
        unique_ptr<File> localWriteFile = make_unique<File>();

        wstring writerLockPath = FileLock::GetWriterLockPath(this->path_);
        wstring readerLockPath = FileLock::GetReaderLockPath(this->path_);

        ErrorCode errorCode = ErrorCode(ErrorCodeValue::Success);

        bool isReaderLock = IsReaderLock;
        
        if (isReaderLock)
        {
            errorCode = localReadFile->TryOpen(
                readerLockPath,
                FileMode::OpenOrCreate,
                FileAccess::Read,
                FileShare::Read,
                FileAttributes::None);
            if (!errorCode.IsSuccess())
            {
                Trace.WriteWarning(FileLockSource, "Failed to acquire ReaderLock for path {0}. LockType = {1} Error = {2}", path_, operation_, errorCode);

                if (errorCode.IsWin32Error(ERROR_SHARING_VIOLATION))
                {
                    return ErrorCodeValue::SharingAccessLockViolation;
                }

                return errorCode;
            }
            
            if (File::Exists(writerLockPath))
            {
                Trace.WriteWarning(FileLockSource, "CorruptedImageStoreObjectFound found for path {0}. LockType = {1}", path_, operation_);
                return ErrorCodeValue::CorruptedImageStoreObjectFound;
            }

            readFile_ = move(localReadFile);

        }
        else
        {
            errorCode = localReadFile->TryOpen(
                readerLockPath,
                FileMode::OpenOrCreate,
                FileAccess::Write,
                FileShare::None,
                FileAttributes::None);
            if (!errorCode.IsSuccess())
            {
                Trace.WriteInfo(FileLockSource, "Failed to acquire ReaderLock for path {0}. LockType = {1} Error = {2}", path_, operation_, errorCode);
                
                if (errorCode.IsWin32Error(ERROR_SHARING_VIOLATION))
                {
                    return ErrorCodeValue::SharingAccessLockViolation;
                }

                return errorCode;
            }

            errorCode = localWriteFile->TryOpen(
                writerLockPath,
                FileMode::OpenOrCreate,
                FileAccess::Write,
                FileShare::None,
                FileAttributes::None);
            if (!errorCode.IsSuccess())
            {
                Trace.WriteInfo(FileLockSource, "Failed to acquire WriterLock for path {0}. LockType = {1} Error = {2}", path_, operation_, errorCode);

                if (errorCode.IsWin32Error(ERROR_SHARING_VIOLATION))
                {
                    errorCode = ErrorCodeValue::SharingAccessLockViolation;
                }

                return errorCode;
            }

            readFile_ = move(localReadFile);
            writeFile_ = move(localWriteFile);
        }

        Trace.WriteInfo(FileLockSource, "Obtained {0} lock for {1}", operation_, path_);            

        return errorCode;
    }

    template <bool IsReaderLock>
    bool FileLock<IsReaderLock>::Release()
    {
        if(writeFile_)
        {
            writeFile_.reset();
            wstring writerLockPath = FileLock::GetWriterLockPath(this->path_);
            ErrorCode error = File::Delete2(writerLockPath);
            Trace.WriteTrace(
                error.ToLogLevel(),
                FileLockSource,
                "Delete lock file {0}. Error:{1}", writerLockPath, error);

            if(!error.IsSuccess())
            {
                return false;
            }
        }

        if(readFile_)
        {
            readFile_.reset();
            wstring readerLockPath = FileLock::GetReaderLockPath(this->path_);
            ErrorCode error = File::Delete2(readerLockPath);            
            Trace.WriteNoise(
                FileLockSource, 
                "Delete lock file {0}. Error:{1}", readerLockPath, error);
            
        }
       
        Trace.WriteInfo(FileLockSource, "Released {0} lock on {1}", operation_, path_);

        return true;
    }

    template <bool IsReaderLock>
    ErrorCode FileLock<IsReaderLock>::Acquire(TimeSpan timeout)
    {
        TimeoutHelper timeoutHelper(timeout);
        ErrorCode lastError;

        do
        {
            lastError = AcquireImpl();

            if (!timeoutHelper.IsExpired && !lastError.IsSuccess())
            {
                Sleep(100);
            }
        } while (!timeoutHelper.IsExpired && !lastError.IsSuccess());

        return lastError;
    }

    template class FileLock<true>;
    template class FileLock<false>;
}
