// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class NotificationKey
            : public KObject<NotificationKey>
            , public KShared<NotificationKey>
        {
            K_FORCE_SHARED(NotificationKey)

        public:
            static NTSTATUS Create(
                __in LONG64 stateProviderId, 
                __in LONG64 visibilitySequencenNumber,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            static ULONG GetHashCode(NotificationKey::SPtr const & notificationKey);

        public:
            /// <summary>
            /// State Provider Id of the state provider for whom the notification is registered.
            /// </summary>
            __declspec(property(get = get_StateProviderId)) LONG64 StateProviderId;
            LONG64 get_StateProviderId() const;

            /// <summary>
            /// Snapshot sequence number of the notification.
            /// </summary>
            __declspec(property(get = get_VisibilitySequenceNumber)) LONG64 VisibilitySeqeuenceNumber;
            LONG64 get_VisibilitySequenceNumber() const;

        public:
            NotificationKey(
                __in LONG64 stateProviderId, 
                __in LONG64 visibilitySequencenNumber) noexcept;

        private:
            LONG64 stateProviderId_;
            LONG64 visibilitySequenceNumber_;
        };
    }
}
