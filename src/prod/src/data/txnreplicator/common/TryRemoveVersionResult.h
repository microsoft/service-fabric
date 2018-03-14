// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class TryRemoveVersionResult final
        : public KObject<TryRemoveVersionResult>
        , public KShared<TryRemoveVersionResult>
    {
        K_FORCE_SHARED(TryRemoveVersionResult)

    public:
        static NTSTATUS Create(
            __in bool canBeRemoved,
            __in KAllocator & allocator,
            __out SPtr & result) noexcept;

        static NTSTATUS Create(
            __in bool canBeRemoved,
            __in_opt Data::Utilities::KHashSet<LONG64> const * const enumerationSet,
            __in_opt Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr> const * const enumerationCompletionNotifications,
            __in KAllocator & allocator,
            __out SPtr & result) noexcept;

        __declspec(property(get = get_CanBeRemoved)) bool CanBeRemoved;
        bool get_CanBeRemoved() const;

        __declspec(property(get = get_EnumerationSet)) Data::Utilities::KHashSet<FABRIC_SEQUENCE_NUMBER>::CSPtr EnumerationSet;
        Data::Utilities::KHashSet<FABRIC_SEQUENCE_NUMBER>::CSPtr get_EnumerationSet() const;

        __declspec(property(get = get_EnumerationCompletionNotifications)) Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr>::CSPtr EnumerationCompletionNotifications;
        Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr>::CSPtr get_EnumerationCompletionNotifications() const;

        bool operator==(const TryRemoveVersionResult & other) const;

    private:
        NOFAIL TryRemoveVersionResult(
            __in bool canBeRemoved,
            __in Data::Utilities::KHashSet<FABRIC_SEQUENCE_NUMBER> const * const enumerationSet,
            __in Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr> const * const enumerationCompletionNotifications) noexcept;

        bool canBeRemoved_;
        Data::Utilities::KHashSet<LONG64>::CSPtr enumerationSetSPtr_;
        Data::Utilities::KHashSet<EnumerationCompletionResult::SPtr>::CSPtr enumerationCompletionNotificationSetSPtr_;
    };
}
