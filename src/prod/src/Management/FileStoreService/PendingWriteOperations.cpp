// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace FileStoreService;

ErrorCode PendingWriteOperations::TryAdd(wstring const & key, bool const isFolder)
{
    Common::AcquireWriteLock lock(this->setLock_);
    if (closed_) return ErrorCodeValue::ObjectClosed;
    
    wstring path = key;
    while(!path.empty())
    {
        auto iter = this->set_.find(path);
        if (iter != this->set_.end())
        {
            return ErrorCodeValue::FileUpdateInProgress;
        }

        path = Path::GetDirectoryName(path);
    }

    if(isFolder)
    {
        bool pendingWrite = std::any_of(
            set_.cbegin(),
            set_.cend(),
            [&key](wstring const & path) { return StringUtility::StartsWithCaseInsensitive<wstring>(path, key); });

        if(pendingWrite)
        {
            return ErrorCodeValue::FileUpdateInProgress;
        }
    }

    this->set_.insert(key);

    return ErrorCodeValue::Success;
}

ErrorCode PendingWriteOperations::Contains(wstring const & key, __out bool & contains)
{
    Common::AcquireReadLock lock(this->setLock_);
    if (closed_) return ErrorCodeValue::ObjectClosed;

    auto iter = this->set_.find(key);

    contains = (iter != this->set_.end());

    return ErrorCodeValue::Success;
}

bool PendingWriteOperations::Remove(wstring const & key)
{
    Common::AcquireWriteLock lock(this->setLock_);
    if (closed_) { return false; }

    auto iter = this->set_.find(key);
    if (iter != this->set_.end())
    {
        this->set_.erase(iter);

        return true;
    }

    return false;
}

size_t PendingWriteOperations::Count()
{
    Common::AcquireReadLock lock(this->setLock_);
    return this->set_.size();
}

void PendingWriteOperations::Clear()
{
    Common::AcquireWriteLock lock(this->setLock_);
    this->set_.clear();
}

std::set<wstring, IsLessCaseInsensitiveComparer<wstring>> PendingWriteOperations::Close()
{
    Common::AcquireWriteLock lock(this->setLock_);

    closed_ = true;

    auto removedSet(std::move(this->set_));

    return removedSet;
}
