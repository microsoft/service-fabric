// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;

NTSTATUS EnumerationCompletionResult::Create(
    __in FABRIC_SEQUENCE_NUMBER visibilitySequenceNumber,
    __in ktl::AwaitableCompletionSource<LONG64> & notificationAcs,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(ENUMERATION_COMPLETION_RESULT_TAG, allocator) EnumerationCompletionResult(visibilitySequenceNumber, notificationAcs);
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

FABRIC_SEQUENCE_NUMBER EnumerationCompletionResult::get_VisibilitySequenceNumber() const
{
    return visibilitySequenceNumber_;
}

ktl::Awaitable<LONG64> EnumerationCompletionResult::get_Notification() const
{
    return notificationAcsSPtr_->GetAwaitable();
}

bool EnumerationCompletionResult::get_IsNotificationCompleted() const
{
    return notificationAcsSPtr_->IsCompleted();
}

ULONG EnumerationCompletionResult::GetHashCode() noexcept
{
    return static_cast<ULONG>(visibilitySequenceNumber_);
}

bool EnumerationCompletionResult::operator==(
    __in EnumerationCompletionResult& other) const
{
    if (visibilitySequenceNumber_ != other.VisibilitySequenceNumber)
    {
        return false;
    }

    return true;
}

EnumerationCompletionResult::EnumerationCompletionResult(
    __in FABRIC_SEQUENCE_NUMBER visibilitySequenceNumber,
    __in ktl::AwaitableCompletionSource<LONG64> & notificationAcs) noexcept
    : KObject<EnumerationCompletionResult>()
    , KShared<EnumerationCompletionResult>()
    , visibilitySequenceNumber_(visibilitySequenceNumber)
    , notificationAcsSPtr_(&notificationAcs)
{
}

EnumerationCompletionResult::EnumerationCompletionResult()
    : KObject<EnumerationCompletionResult>()
    , KShared<EnumerationCompletionResult>()
    , visibilitySequenceNumber_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , notificationAcsSPtr_(nullptr)
{
}

EnumerationCompletionResult::~EnumerationCompletionResult()
{
}
