// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    using namespace std;

    StringLiteral const TraceAcquireExclusive("RwLock.AcquireExclusive");
    StringLiteral const TraceReleaseExclusive("RwLock.ReleaseExclusive");
    StringLiteral const TraceAcquireShared("RwLock.AcquireShared");
    StringLiteral const TraceReleaseShared("RwLock.ReleaseShared");

    volatile bool Inited = false;
    Global<RwLock> InitLock = make_global<RwLock>();
    Global<vector<string>> LockPrefixList = make_global<vector<string>>();

    void RwLockTracer::TraceLock(StringLiteral type, void* address, char const* lockName)
    {
        if (!Inited)
        {
            AcquireWriteLock grab(*InitLock);
            if (!Inited)
            {
                StringUtility::Split<string>(CommonConfig::GetConfig().LockTraceNamePrefix, *LockPrefixList, ",");
                Inited = true;
            }
        }

        if (LockPrefixList->size() == 0)
        {
            Trace.WriteInfo(type, "{0}:{1}", address, lockName);
            return;
        }

        for (string const & prefix : *LockPrefixList)
        {
            char const* p1 = lockName;
            char const* p2 = prefix.c_str();

            while (*p1++ == *p2++)
            {
                if (*p2 == 0)
                {
                    Trace.WriteInfo(type, "{0}:{1}", address, lockName);
                    return;
                }
            }
        }
    }

    _Acquires_exclusive_lock_(lock_)
    LOCKAPI RWLockBase::AcquireExclusive()
    {
        CheckOwners();
        AcquireSRWLockExclusive(&lock_);
        AddOwner();
    }

    _Releases_exclusive_lock_(lock_)
    LOCKAPI RWLockBase::ReleaseExclusive()
    {
        RemoveOwner();
        ReleaseSRWLockExclusive(&lock_);
    }

    _Acquires_shared_lock_(lock_)
    LOCKAPI RWLockBase::AcquireShared()
    {
        CheckOwners();
        AcquireSRWLockShared(&lock_);
        AddOwner();
    }

    _Releases_shared_lock_(lock_)
    LOCKAPI RWLockBase::ReleaseShared()
    {
        RemoveOwner();
        ReleaseSRWLockShared(&lock_);
    }

#ifdef DBG

    void RWLockBase::CheckOwners()
    {
        AcquireSRWLockShared(&ownerSetLock_);
        auto iter = owners_.find(GetCurrentThreadId());
        if (iter != owners_.end())
        {
            Common::Assert::CodingError("Trying to recursively acquire a RWLock");
        }
        ReleaseSRWLockShared(&ownerSetLock_);
    }

    void RWLockBase::AddOwner()
    {
        AcquireSRWLockExclusive(&ownerSetLock_);
        owners_.insert(GetCurrentThreadId());
        ReleaseSRWLockExclusive(&ownerSetLock_);
    }

    void RWLockBase::RemoveOwner()
    {
        AcquireSRWLockExclusive(&ownerSetLock_);
        auto count = owners_.erase(GetCurrentThreadId());
        if ( count != 1)
        {
            Common::Assert::CodingError("Trying to release a RWLock not acquired on this thread");
        }
        ReleaseSRWLockExclusive(&ownerSetLock_);
    }

#endif
}
