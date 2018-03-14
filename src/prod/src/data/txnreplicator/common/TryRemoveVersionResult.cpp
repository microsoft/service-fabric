// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;

NTSTATUS TryRemoveVersionResult::Create(
    __in bool canBeRemoved,
    __in KAllocator& allocator,
    __out SPtr& result) noexcept
{
    return Create(canBeRemoved, nullptr, nullptr, allocator, result);
}

NTSTATUS TryRemoveVersionResult::Create(
    __in bool canBeRemoved, 
    __in Data::Utilities::KHashSet<LONG64> const * const enumerationSet, 
    __in Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr> const * const enumerationCompletionNotifications, 
    __in KAllocator & allocator, 
    __out SPtr & result) noexcept
{
    result = _new(TRY_REMOVE_VERSION_RESULT_TAG, allocator) TryRemoveVersionResult(canBeRemoved, enumerationSet, enumerationCompletionNotifications);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

bool TryRemoveVersionResult::get_CanBeRemoved() const
{
    return canBeRemoved_;
}

Data::Utilities::KHashSet<FABRIC_SEQUENCE_NUMBER>::CSPtr TryRemoveVersionResult::get_EnumerationSet() const
{
    return enumerationSetSPtr_;
}

Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr>::CSPtr TryRemoveVersionResult::get_EnumerationCompletionNotifications() const
{
    return enumerationCompletionNotificationSetSPtr_;
}

bool TryRemoveVersionResult::operator==(const TryRemoveVersionResult& other) const
{
    if (canBeRemoved_ != other.CanBeRemoved)
    {
        return false;
    }

    if (enumerationSetSPtr_ == nullptr)
    {
        if (other.EnumerationSet != nullptr)
        {
            return false;
        }
    }
    else
    {
        if (other.EnumerationSet == nullptr)
        {
            return false;
        }
        
        bool areEqual = enumerationSetSPtr_->Equals(*other.EnumerationSet);
        if (areEqual == false)
        {
            return false;
        }
    }

    if (enumerationCompletionNotificationSetSPtr_ == nullptr)
    {
        if (other.EnumerationCompletionNotifications != nullptr)
        {
            return false;
        }
    }
    else
    {
        if (other.EnumerationCompletionNotifications == nullptr)
        {
            return false;
        }

        bool areEqual = enumerationCompletionNotificationSetSPtr_->Equals(*other.EnumerationCompletionNotifications);
        if (areEqual == false)
        {
            return false;
        }
    }

    return true;
}

TryRemoveVersionResult::TryRemoveVersionResult(
    __in bool canBeRemoved,
    __in Data::Utilities::KHashSet<FABRIC_SEQUENCE_NUMBER> const * const enumerationSet,
    __in Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr> const * const enumerationCompletionNotifications) noexcept
    : KObject<TryRemoveVersionResult>()
    , KShared<TryRemoveVersionResult>()
    , canBeRemoved_(canBeRemoved)
    , enumerationSetSPtr_(enumerationSet)
    , enumerationCompletionNotificationSetSPtr_(enumerationCompletionNotifications) 
{
}

TryRemoveVersionResult::~TryRemoveVersionResult()
{
}


