// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class EnumerationCompletionResult final
        : public KObject<EnumerationCompletionResult>
        , public KShared<EnumerationCompletionResult>
    {
        K_FORCE_SHARED(EnumerationCompletionResult)

    public:
        static NTSTATUS Create(
            __in FABRIC_SEQUENCE_NUMBER visibilitySequenceNumber,
            __in ktl::AwaitableCompletionSource<LONG64> & notificationAcs,
            __in KAllocator & allocator,
            __out SPtr & result) noexcept;

        __declspec(property(get = get_VisibilitySequenceNumber)) FABRIC_SEQUENCE_NUMBER VisibilitySequenceNumber;
        FABRIC_SEQUENCE_NUMBER get_VisibilitySequenceNumber() const;

        __declspec(property(get = get_Notification)) ktl::Awaitable<LONG64> Notification;
        ktl::Awaitable<LONG64> get_Notification() const;

        __declspec(property(get = get_IsNotificationCompleted)) bool IsNotificationCompleted;
        bool get_IsNotificationCompleted() const;

        ULONG GetHashCode() noexcept;

        bool operator==(EnumerationCompletionResult &) const;

    private:
        NOFAIL EnumerationCompletionResult(
            __in FABRIC_SEQUENCE_NUMBER visibilitySequenceNumber,
            __in ktl::AwaitableCompletionSource<LONG64> & notificationAcs) noexcept;

        FABRIC_SEQUENCE_NUMBER visibilitySequenceNumber_;
        ktl::AwaitableCompletionSource<LONG64>::SPtr notificationAcsSPtr_;
    };
}
