// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType = "CachedFile";

const int CachedFile::MaxRetryIntervalInMillis = 1000;
const int CachedFile::MaxRetryCount = 10;

// monitors the changes to a file
class CachedFile::ChangeMonitor : 
    public FabricComponent,
    public std::enable_shared_from_this<ChangeMonitor>
{
    DENY_COPY(ChangeMonitor);

public:
    ChangeMonitor::ChangeMonitor(CachedFileSPtr const & owner, wstring const & filePath_)
        : weak_owner_(owner),
        fileMonitor_(FileChangeMonitor2::Create(filePath_))
    {
    }

    virtual ChangeMonitor::~ChangeMonitor()
    {
    }

protected:
    ErrorCode OnOpen()
    {
        fileMonitor_->RegisterFailedEventHandler([this](ErrorCode const & error) { this->OnMonitorFailure(error); });
        // Passing a shared pointer to lamda to ensure that a reference to the current object is hold as long as the lambda is active.
        // This will prevent the parent from being freed before the lambda. 
        auto thisSPtr = shared_from_this();
        auto error = fileMonitor_->Open([thisSPtr](FileChangeMonitor2SPtr const & monitor) { thisSPtr->OnChange(monitor); } );
        if (!error.IsSuccess())
        {
            fileMonitor_->Abort();
        }

        return error;
    }

    ErrorCode OnClose()
    {
        fileMonitor_->Close();
        return ErrorCode(ErrorCodeValue::Success);
    }

    void OnAbort()
    {
        fileMonitor_->Abort();
    }

private:
    void OnChange(FileChangeMonitor2SPtr const &)
    {
        auto owner = weak_owner_.lock();
        if (owner)
        {
            auto cachedFileObj = static_cast<CachedFile*>(owner.get());
            cachedFileObj->ProcessChangeNotification();
        }
    }

    void OnMonitorFailure(ErrorCode const & error)
    {
        fileMonitor_->Abort();
        Assert::CodingError("FileChangeMonitor failed with {0}", error);
    }

private:
    CachedFileWPtr const weak_owner_;
    FileChangeMonitor2SPtr const fileMonitor_;
};

ErrorCode CachedFile::Create(
    __in wstring const & filePath,
    __in CachedFileReadCallback const & readCallback,
    __out CachedFileSPtr & cachedFileSPtr
    )
{
    if(readCallback == nullptr)
    {
        WriteInfo(TraceType, "Value of readCallback cannot be null for Create");
        return ErrorCodeValue::InvalidArgument;
    }
    auto filePtr = std::shared_ptr<CachedFile>(new CachedFile(filePath, readCallback));
    auto error = filePtr->EnableChangeMonitoring();
    if (!error.IsSuccess())
    {
        WriteInfo(TraceType, "Failed to enable change monitoring on file {0}. ErrorCode={1}", filePath, error);
        return error;
    }

    cachedFileSPtr = std::move(filePtr);
    return ErrorCodeValue::Success;
}

CachedFile::CachedFile (wstring const & filePath, CachedFileReadCallback const & readCallback)
    : filePath_(filePath),
    readCallback_(readCallback),
    changeMonitor_(),
    lock_(),
    isContentValid_(false),
    content_()
{
}

CachedFile::~CachedFile ()
{
    this->changeMonitor_->Close().ReadValue();
}

ErrorCode CachedFile::ReadFileContent(__out std::wstring & content)
{
    {
        // Directly read from cache if it is valid
        AcquireReadLock readLock(lock_);
        if (isContentValid_)
        {
            content = content_;
            return ErrorCodeValue::Success;
        }
    }

    {
        AcquireWriteLock writeLock(lock_);
        // If concurrent threads try to read an invalid cache, One of the threads will go in and read from the disk.
        // The other threads will wait for the lock and once released, will simply read from the cache.
        if (isContentValid_)
        {
            content = content_;
            return ErrorCodeValue::Success;
        }

        // Read the file from disk to the cache.
        ErrorCode error = this->ReadFile_WithRetry();
        if (!error.IsSuccess())
        {
            return error;
        }
        content = content_;
    }

    return ErrorCodeValue::Success;
}

void CachedFile::ProcessChangeNotification()
{
    AcquireWriteLock writeLock(lock_);
    if (isContentValid_)
    {
        isContentValid_ = false;
    }
}

ErrorCode CachedFile::ReadFile_WithRetry()
{
    Random rand;
    ErrorCode lastError(ErrorCodeValue::OperationFailed);

    for(int i = 0; i < CachedFile::MaxRetryCount; i++)
    {
        ASSERT_IF(this->readCallback_ == nullptr, "read callback must not be null.");
        auto error = readCallback_(filePath_, content_);
        
        if(error.IsSuccess())
        {
            isContentValid_ = true;
            return error;
        }
        else
        {
            // Retry if last operation resulted in an error.
                lastError.Overwrite(error);
            ::Sleep(rand.Next(CachedFile::MaxRetryIntervalInMillis));
        }
    }

    return lastError;
}

ErrorCode CachedFile::EnableChangeMonitoring()
{
    ASSERT_IFNOT(this->changeMonitor_ == nullptr, "ChangeMonitor must be null.");
    this->changeMonitor_ = make_shared<ChangeMonitor>(this->shared_from_this(), this->filePath_);
    return this->changeMonitor_->Open();
}
