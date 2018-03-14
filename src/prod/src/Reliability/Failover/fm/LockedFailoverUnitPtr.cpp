// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;

LockedFailoverUnitPtr::LockedFailoverUnitPtr()
    : entry_(nullptr)
    , old_(nullptr)
    , isUpdating_(false)
    , skipPersistence_(false)
{
}

LockedFailoverUnitPtr::LockedFailoverUnitPtr(FailoverUnitCacheEntrySPtr const & entry)
    : entry_(entry)
    , old_(nullptr)
    , isUpdating_(false)
    , skipPersistence_(false)
{
    if (entry_)
    {
        ASSERT_IF(entry_->FailoverUnit->PersistenceState != PersistenceState::NoChange,
            "Invalid persistence state: {0}", *(entry_->FailoverUnit));
    }
}

LockedFailoverUnitPtr::LockedFailoverUnitPtr(LockedFailoverUnitPtr && other)
    : entry_(std::move(other.entry_))
    , old_(move(other.old_))
    , isUpdating_(other.isUpdating_)
    , skipPersistence_(other.skipPersistence_)
{
}

LockedFailoverUnitPtr::LockedFailoverUnitPtr(LockedFailoverUnitPtr const & other)
    : entry_(std::move(other.entry_))
    , old_(move(other.old_))
    , isUpdating_(other.isUpdating_)
    , skipPersistence_(other.skipPersistence_)
{
}

LockedFailoverUnitPtr & LockedFailoverUnitPtr::operator=(LockedFailoverUnitPtr && other)
{
    if (this != &other)
    {
        entry_ = move(other.entry_);
        old_ = move(other.old_);
        isUpdating_ = other.isUpdating_;
        skipPersistence_ = other.skipPersistence_;
    }

    return *this;
}

LockedFailoverUnitPtr::~LockedFailoverUnitPtr()
{
    Release(false, false);
}

bool LockedFailoverUnitPtr::Release(bool restoreExecutingTask, bool processPendingTask)
{
    if (entry_)
    {
        if (isUpdating_)
        {
            Revert();
        }

        if (!entry_->Release(restoreExecutingTask, processPendingTask))
        {
            return false;
        }

        entry_ = nullptr;
    }

    return true;
}

void LockedFailoverUnitPtr::EnableUpdate(bool skipPersistence)
{
    if (!isUpdating_)
    {
        ASSERT_IFNOT(entry_->FailoverUnit->PersistenceState == PersistenceState::NoChange, 
            "FailoverUnit already changed before old copy is created");

        old_ = make_unique<FailoverUnit>(*(entry_->FailoverUnit));
        isUpdating_ = true;
        skipPersistence_ = skipPersistence;
    }
    else
    {
        skipPersistence_ &= skipPersistence;
    }
}

void LockedFailoverUnitPtr::Revert()
{
    ASSERT_IFNOT(isUpdating_, "FailoverUnit doesn't change when calling revert");
    old_->ReplicaDifference = 0;
    entry_->FailoverUnit = move(old_);
    old_ = nullptr;
    isUpdating_ = false;
}

void LockedFailoverUnitPtr::Submit()
{
    ASSERT_IFNOT(isUpdating_, "FailoverUnit doesn't change when calling submit");
    isUpdating_ = false;
}

FailoverUnit const* LockedFailoverUnitPtr::operator->() const
{
    return get();
}

FailoverUnit const& LockedFailoverUnitPtr::operator*() const
{
    return *get();
}

FailoverUnit * LockedFailoverUnitPtr::operator->()
{
    return get();
}

FailoverUnit & LockedFailoverUnitPtr::operator*()
{
    return *get();
}

LockedFailoverUnitPtr::operator bool() const
{
    return (entry_ != nullptr);
}

FailoverUnit* LockedFailoverUnitPtr::get() const
{
    return entry_->FailoverUnit.get();
}
