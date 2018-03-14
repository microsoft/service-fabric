// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;

NTSTATUS NotificationKey::Create(
    __in LONG64 stateProviderId, 
    __in LONG64 visibilitySequencenNumber, 
    __in KAllocator& allocator, 
    __out SPtr& result) noexcept
{
    result = _new(NOTIFICATION_KEY_TAG, allocator) NotificationKey(stateProviderId, visibilitySequencenNumber);
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

ULONG NotificationKey::GetHashCode(NotificationKey::SPtr const& notificationKey)
{
    return static_cast<ULONG>(notificationKey->StateProviderId ^ notificationKey->VisibilitySeqeuenceNumber);
}

LONG64 NotificationKey::get_StateProviderId() const
{
    return stateProviderId_;
}

LONG64 NotificationKey::get_VisibilitySequenceNumber() const
{
    return visibilitySequenceNumber_;
}

NotificationKey::NotificationKey(
    __in LONG64 stateProviderId, 
    __in LONG64 visibilitySequencenNumber) noexcept
    : stateProviderId_(stateProviderId)
    , visibilitySequenceNumber_(visibilitySequencenNumber)
{
}

NotificationKey::~NotificationKey()
{
}

