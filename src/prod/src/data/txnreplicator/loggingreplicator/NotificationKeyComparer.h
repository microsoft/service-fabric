// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class NotificationKeyComparer final
            : public IComparer<NotificationKey::SPtr>
            , public KObject<NotificationKeyComparer>
            , public KShared<NotificationKeyComparer>         
        {
            K_FORCE_SHARED(NotificationKeyComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(__in KAllocator & allocator, __out SPtr & result) noexcept;

        public:
            int Compare(__in const NotificationKey::SPtr & x, __in const NotificationKey::SPtr & y) const override;
        };
    }
}
